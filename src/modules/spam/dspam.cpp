///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   src/modules/spam/dspam.cpp
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

   virtual void Reclassify(const Message& msg, bool isSpam);
   virtual bool Process(const Message& msg, float *probability);

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

   // common part of Reclassify() and Process(): processes the message and
   // returns false if we failed
   bool DoProcess(const Message& msg, ContextHandler& handler);
};

IMPLEMENT_SPAM_FILTER(DspamFilter,
                      gettext_noop("DSPAM Statistical Spam Filter"),
                      _T("(c) 2004 Vadim Zeitlin <vadim@wxwindows.org>"));

// ============================================================================
// DspamFilter implementation
// ============================================================================

DspamFilter::DspamFilter()
{
   dspam_set_home(mApplication->GetLocalDir());

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

void DspamFilter::Reclassify(const Message& msg, bool isSpam)
{
   class ReclassifyContextHandler : public ContextHandler
   {
   public:
      ReclassifyContextHandler(bool isSpam) { m_isSpam = isSpam; }

      virtual void OnInit(DSPAM_CTX *ctx)
      {
         ctx->classification = m_isSpam ? DSR_ISSPAM : DSR_ISINNOCENT;
         ctx->source = DSS_ERROR;
      }

   private:
      bool m_isSpam;
   };

   ReclassifyContextHandler handler(isSpam);
   DoProcess(msg, handler);
}

bool DspamFilter::Process(const Message& msg, float *probability)
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
         if ( m_probability )
            *m_probability = ctx->probability;

         *m_rc = ctx->result == DSR_ISSPAM;
      }

   private:
      bool *m_rc;
      float *m_probability;
   };

   bool rc;
   CheckContextHandler handler(&rc, probability);
   if ( !DoProcess(msg, handler) )
      return false;

   return rc;
}

