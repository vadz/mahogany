///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   src/modules/spam/DspamFilter.cpp
// Purpose:     SpamFilter implementation using dspam
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-08-04
// CVS-ID:      $Id$
// Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #include "MApplication.h"
#endif //USE_PCH

#include "Message.h"

#include "SpamFilter.h"

extern "C"
{
   #include <libdspam.h>

   extern void dspam_set_home(const char *home);
}

// ----------------------------------------------------------------------------
// DspamCtx simple wrapper around DSPAM_CTX
// ----------------------------------------------------------------------------

class DspamCtx
{
public:
   // take ownership of the specified context
   DspamCtx(DSPAM_CTX *ctx) : m_ctx(ctx) { }

   // destroy the context
   ~DspamCtx()
   {
      _ds_destroy_message(m_ctx->message);
      if ( dspam_destroy(m_ctx) != 0 )
      {
         wxLogDebug(_T("dspam_destroy() failed"));
      }
   }

   operator DSPAM_CTX *() const { return m_ctx; }
   DSPAM_CTX *operator->() const { return m_ctx; }

private:
   DSPAM_CTX *m_ctx;

   DECLARE_NO_COPY_CLASS(DspamCtx);
};

// ----------------------------------------------------------------------------
// DspamFilter class
// ----------------------------------------------------------------------------

class DspamFilter : public SpamFilter
{
public:
   DspamFilter();

protected:
   virtual void DoReclassify(const Message& msg, bool isSpam);
   virtual void DoTrain(const Message& msg, bool isSpam);
   virtual bool DoCheckIfSpam(const Message& msg,
                              const String& param,
                              String *result);

private:
   // helper: used as DoProcess() argument to initialize the context or to get
   // result from it
   class ContextHandler
   {
   public:
      virtual void OnInit(DSPAM_CTX *ctx) { }
      virtual void OnDone(DSPAM_CTX *ctx) { }
      virtual ~ContextHandler() { }
   };

   // ContextHandler used by DoReclassify() and DoTrain()
   class ClassifyContextHandler : public ContextHandler
   {
   public:
      enum Operation
      {
         Reclassify,
         Train
      };

      ClassifyContextHandler(Operation operation, bool isSpam)
      {
         m_operation = operation;
         m_isSpam = isSpam;
      }

      virtual void OnInit(DSPAM_CTX *ctx)
      {
         ctx->classification = m_isSpam ? DSR_ISSPAM : DSR_ISINNOCENT;
         ctx->source = m_operation == Reclassify ? DSS_ERROR : DSS_CORPUS;
      }

   private:
      Operation m_operation;
      bool m_isSpam;
   };


   // common part of all DoXXX() functions: processes the message and returns
   // false if we failed, use ContextHandler to customize processing
   bool DoProcess(const Message& msg, ContextHandler& handler);


   DECLARE_SPAM_FILTER("dspam", 100);
};

IMPLEMENT_SPAM_FILTER(DspamFilter,
                      gettext_noop("DSPAM Statistical Spam Filter"),
                      _T("(c) 2004 Vadim Zeitlin <vadim@wxwindows.org>"));

// ============================================================================
// DspamFilter implementation
// ============================================================================

DspamFilter::DspamFilter()
{
#ifdef __WINDOWS__
   dspam_set_home(mApplication->GetLocalDir());
#endif // __WINDOWS__

   dspam_init_driver();
}

bool DspamFilter::DoProcess(const Message& msg, ContextHandler& handler)
{
   DspamCtx ctx(dspam_init
                (
                  "mahogany",
                  NULL,
                  DSM_PROCESS,
                  DSF_CHAINED | DSF_NOISE
                ));
   if ( !ctx )
   {
      ERRORMESSAGE((_("DSPAM: library initialization failed.")));

      return false;
   }

   String str;
   if ( !msg.WriteToString(str) )
   {
      ERRORMESSAGE((_("Failed to get the message text.")));

      return false;
   }

   handler.OnInit(ctx);

   if ( dspam_process(ctx, str) != 0 )
   {
      ERRORMESSAGE((_("DSPAM: processing message failed.")));

      return false;
   }

   handler.OnDone(ctx);

   return true;
}

void DspamFilter::DoReclassify(const Message& msg, bool isSpam)
{
   ClassifyContextHandler handler(ClassifyContextHandler::Reclassify, isSpam);

   DoProcess(msg, handler);
}

void DspamFilter::DoTrain(const Message& msg, bool isSpam)
{
   ClassifyContextHandler handler(ClassifyContextHandler::Train, isSpam);

   DoProcess(msg, handler);
}

bool
DspamFilter::DoCheckIfSpam(const Message& msg,
                           const String& param,
                           String *result)
{
   class CheckContextHandler : public ContextHandler
   {
   public:
      CheckContextHandler(bool *rc, float *probability)
      {
         m_rc = rc;
         m_probability = probability;
      }

      virtual void OnDone(DSPAM_CTX *ctx)
      {
         *m_probability = ctx->probability;
         *m_rc = ctx->result == DSR_ISSPAM;
      }

   private:
      bool *m_rc;
      float *m_probability;
   };

   ASSERT_MSG( param.empty(), _T("DspamFilter has no parameters") );

   bool rc;
   float probability;
   CheckContextHandler handler(&rc, &probability);
   if ( !DoProcess(msg, handler) || !rc )
      return false;

   if ( result )
   {
      result->Printf(_T("probability = %0.3f"), probability);
   }

   return true;
}

