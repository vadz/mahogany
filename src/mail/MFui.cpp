///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/MFui.cpp
// Purpose:     implements functions from MFui.h
// Author:      Vadim Zeitlin
// Modified by:
// Created:     04.07.02 (extracted from MailFolder.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #include "strutil.h"    // for strutil_ultoa()
#endif // USE_PCH

#include "MFui.h"
#include "MFStatus.h"
#include "MailFolder.h"
#include "MFolder.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// convert the message size to a user-readable string
// ----------------------------------------------------------------------------

String
SizeToString(unsigned long sizeBytes,
             unsigned long sizeLines,
             MessageSizeShow show,
             int flags)
{
   const bool verbose = (flags & SizeToString_Verbose) != 0;

   String s;

   switch ( show )
   {
      case MessageSize_Max: // to silence gcc warning
         FAIL_MSG( _T("unexpected message size format") );
         // fall through

      case MessageSize_Automatic:
         if ( sizeLines > 0 )
         {
            s.Printf(_("%lu lines"),  sizeLines);
            break;
         }
         // fall through

      case MessageSize_AutoBytes:
         if ( sizeBytes == 0 )
         {
            s = _("empty");
         }
         else if ( sizeBytes < 10*1024 )
         {
            s = SizeToString(sizeBytes, 0, MessageSize_Bytes);
         }
         else if ( sizeBytes < 10*1024*1024 )
         {
            s = SizeToString(sizeBytes, 0, MessageSize_KBytes);
         }
         else // Mb
         {
            s = SizeToString(sizeBytes, 0, MessageSize_MBytes);
         }
         break;

      case MessageSize_Bytes:
         s.Printf("%lu%s", sizeBytes, verbose ? _(" bytes") : "");
         break;

      case MessageSize_KBytes:
         s.Printf(_("%lu%s"), sizeBytes / 1024,
                              verbose ? _(" kilobytes") : _("Kb"));
         break;

      case MessageSize_MBytes:
         s.Printf(_("%lu%s"), sizeBytes / (1024*1024),
                              verbose ? _(" megabytes") : _("Mb"));
         break;
   }

   return s;
}

// ----------------------------------------------------------------------------
// convert the message status to a user-readable string
// ----------------------------------------------------------------------------

String
ConvertMessageStatusToString(int status)
{
   String strstatus;

   // in IMAP/cclient the NEW == RECENT && !SEEN while for most people it is
   // just NEW == !SEEN
   if ( status & MailFolder::MSG_STAT_RECENT )
   {
      // 'R' == RECENT && SEEN (doesn't make much sense if you ask me)
      strstatus << ((status & MailFolder::MSG_STAT_SEEN) ? 'R' : 'N');
   }
   else // !recent
   {
      // we're interested in showing the UNSEEN messages
      strstatus << ((status & MailFolder::MSG_STAT_SEEN) ? ' ' : 'U');
   }

   strstatus << ((status & MailFolder::MSG_STAT_FLAGGED) ? '*' : ' ')
             << ((status & MailFolder::MSG_STAT_ANSWERED) ? 'A' : ' ')
             << ((status & MailFolder::MSG_STAT_DELETED) ? 'D' : ' ');

   return strstatus;
}

// ----------------------------------------------------------------------------
// format the folder status using the specified format string
// ----------------------------------------------------------------------------

// from MFStatus.h
String FormatFolderStatusString(const String& format,
                                const String& name,
                                MailFolderStatus *status,
                                const MailFolder *mf)
{
   // this is a wrapper class which catches accesses to MailFolderStatus and
   // fills it with the info from the real folder if it is not yet initialized
   class StatusInit
   {
   public:
      StatusInit(MailFolderStatus *status,
                 const String& name,
                 const MailFolder *mf)
         : m_name(name)
      {
         m_status = status;
         m_mf = (MailFolder *)mf; // cast away const for IncRef()
         SafeIncRef(m_mf);

         // if we already have status, nothing to do
         m_init = status->IsValid();
      }

      MailFolderStatus *operator->() const
      {
         if ( !m_init )
         {
            ((StatusInit *)this)->m_init = true;

            // query the mail folder for info
            MailFolder *mf;
            if ( !m_mf )
            {
               MFolder_obj mfolder(m_name);
               mf = MailFolder::OpenFolder(mfolder);
            }
            else // already have the folder
            {
               mf = m_mf;
               mf->IncRef();
            }

            if ( mf )
            {
               mf->CountAllMessages(m_status);
               mf->DecRef();
            }
         }

         return m_status;
      }

      ~StatusInit()
      {
         SafeDecRef(m_mf);
      }

   private:
      MailFolderStatus *m_status;
      MailFolder *m_mf;
      String m_name;
      bool m_init;
   } stat(status, name, mf);

   String result;
   const char *start = format.c_str();
   for ( const char *p = start; *p; p++ )
   {
      if ( *p == '%' )
      {
         switch ( *++p )
         {
            case '\0':
               wxLogWarning(_("Unexpected end of string in the status format "
                              "string '%s'."), start);
               p--; // avoid going beyond the end of string
               break;

            case 'f':
               result += name;
               break;

            case 'i':               // flagged == important
               result += strutil_ultoa(stat->flagged);
               break;


            case 't':               // total
               result += strutil_ultoa(stat->total);
               break;

            case 'r':               // recent
               result += strutil_ultoa(stat->recent);
               break;

            case 'n':               // new
               result += strutil_ultoa(stat->newmsgs);
               break;

            case 's':               // search result
               result += strutil_ultoa(stat->searched);
               break;

            case 'u':               // unread
               result += strutil_ultoa(stat->unread);
               break;

            case '%':
               result += '%';
               break;

            default:
               wxLogWarning(_("Unknown macro '%c%c' in the status format "
                              "string '%s'."), *(p-1), *p, start);
         }
      }
      else // not a format spec
      {
         result += *p;
      }
   }

   return result;
}

