//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/Pop3.cpp: POP3 helper functions
// Purpose:     collects POP3-specific utility functions used by MailFolderCC
// Author:      Vadim Zeitlin
// Modified by:
// Created:     15.10.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include  "Mpch.h"

#ifndef  USE_PCH
#  include "Mcommon.h"

#  include "Mcclient.h"
#endif // USE_PCH

extern "C"
{
   #undef LOCAL         // previously defined in other cclient headers
   #include <pop3.h>    // for pop3_xxx() functions
   #undef LOCAL         // prevent clashes with other headers
}

#include <wx/textfile.h>

#include "CacheFile.h"

#include "MailFolder.h" // for flags constants

// from MailFolderCC.cpp
extern int GetMsgStatus(const MESSAGECACHE *elt);

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// PopFlagsCacheFile: saves the UIDL <-> flags correspondence for POP3
// ----------------------------------------------------------------------------

class PopFlagsCacheFile : public CacheFile
{
public:
   PopFlagsCacheFile(const String& folderName,
                     MAILSTREAM *stream,
                     const wxArrayString *uidls);
   virtual ~PopFlagsCacheFile() { }

   void SaveFlags();
   void RestoreFlags();

   // get the name of the cache file to use for this folder
   static String GetCacheFileName(const String& folderName);

protected:
   // implement CacheFile pure virtuals

   virtual String GetFileName() const;
   virtual String GetFileHeader() const;
   virtual int GetFormatVersion() const;

   virtual bool DoLoad(const wxTextFile& file, int version);
   virtual bool DoSave(wxTempFile& file);

private:
   String         m_folderName;

   MAILSTREAM    *m_stream;

   const wxArrayString *m_uidls;

   DECLARE_NO_COPY_CLASS(PopFlagsCacheFile)
};

// ============================================================================
// PopFlagsCacheFile implementation
// ============================================================================

/* static */
String PopFlagsCacheFile::GetCacheFileName(const String& folderName)
{
   String folderNameFixed = folderName;
   folderNameFixed.Replace("/", "_");

   String filename;
   filename << GetCacheDirName() << DIR_SEPARATOR << folderNameFixed;

   return filename;
}

PopFlagsCacheFile::PopFlagsCacheFile(const String& folderName,
                                     MAILSTREAM *stream,
                                     const wxArrayString *uidls)
                 : m_folderName(folderName)
{
   m_stream = stream;
   m_uidls = uidls;
}

void PopFlagsCacheFile::SaveFlags()
{
   if ( !Save() )
   {
      wxLogWarning(_("Failed to save flags for POP3 folder '%s'"),
                   m_folderName.c_str());
   }
}

void PopFlagsCacheFile::RestoreFlags()
{
   (void)Load();
}

String PopFlagsCacheFile::GetFileName() const
{
   return GetCacheFileName(m_folderName);
}

String PopFlagsCacheFile::GetFileHeader() const
{
   return "Mahogany POP3 Flags Cache File (version %d.%d)";
}

int PopFlagsCacheFile::GetFormatVersion() const
{
   return BuildVersion(1, 0);
}

bool PopFlagsCacheFile::DoLoad(const wxTextFile& file, int /* version */)
{
   String uidl;

   size_t count = file.GetLineCount();
   for ( size_t n = 1; n < count; n++ )
   {
      int flags;
      bool ok = sscanf(file[n], "%s %d",
                       uidl.GetWriteBuf(file[n].length()), &flags) == 2;
      uidl.UngetWriteBuf();

      if ( !ok )
      {
         wxLogWarning(_("Incorrect format at line %d."), n + 1);

         return false;
      }

      // find the message with this UIDL in the folder
      int idx = m_uidls->Index(uidl);
      if ( idx != wxNOT_FOUND )
      {
         // +1 to make it a msgno from index
         MESSAGECACHE *elt = mail_elt(m_stream, idx + 1);
         if ( elt )
         {
            elt->recent = (flags & MailFolder::MSG_STAT_RECENT) != 0;
            elt->seen = (flags & MailFolder::MSG_STAT_SEEN) != 0;
            elt->flagged = (flags & MailFolder::MSG_STAT_FLAGGED) != 0;
            elt->answered = (flags & MailFolder::MSG_STAT_ANSWERED) != 0;
            elt->deleted = (flags & MailFolder::MSG_STAT_DELETED) != 0;
         }
         else
         {
            FAIL_MSG( _T("where is the cache element?") );
         }
      }
   }

   return true;
}

bool PopFlagsCacheFile::DoSave(wxTempFile& file)
{
   CHECK( m_uidls, false, _T("must have UIDL array for saving") );

   wxString str;
   str.reserve(1024);

   for ( unsigned long msgno = 1; msgno <= m_stream->nmsgs; msgno++ )
   {
      MESSAGECACHE *elt = mail_elt(m_stream, msgno);

      int flags;
      if ( elt )
      {
         // the message won't be recent the next time the folder is opened
         flags = GetMsgStatus(elt) & ~MailFolder::MSG_STAT_RECENT;
      }
      else
      {
         FAIL_MSG( _T("where is the cache element?") );

         flags = 0;
      }

      str.Printf("%s %d\n", m_uidls->Item(msgno - 1).c_str(), flags);

      if ( !file.Write(str) )
      {
         return false;
      }
   }

   return true;
}

// ============================================================================
// private helpers
// ============================================================================

static bool Pop3_GetUIDLs(MAILSTREAM *stream, wxArrayString& uidls)
{
   if ( !pop3_send(stream, "UIDL", NIL) )
   {
      // TODO: don't use it the next time
      return false;
   }

   String uidl;
   uidls.Alloc(stream->nmsgs);

   NETSTREAM *netstream = ((POP3LOCAL *)stream->local)->netstream;
   for ( ;; )
   {
      char *s = net_getline(netstream);
      if ( !s )
      {
         // net read error?
         break;
      }

      // check for end of text
      char *t;
      if ( *s == '.' )
      {
         if ( s[1] )
         {
            // dot stuffed line
            t = s + 1;
         }
         else
         {
            // real EOT
            t = NULL;
         }
      }
      else // normal line
      {
         t = s;
      }

      // process it: the reply format is "msgno <uidl>"
      if ( t )
      {
         // RFC says 70 characters max, but avoid buffer overflows just in case
         unsigned long msgno;
         bool ok =
            sscanf(t, "%lu %s", &msgno, uidl.GetWriteBuf(strlen(t) + 1)) == 2;
         uidl.UngetWriteBuf();

         if ( ok )
         {
            // check that the msgno returned is correct too - they should be
            // consecutive
            ok = msgno == uidls.GetCount() + 1;
         }

         if ( !ok )
         {
            wxLogDebug(_T("Unexpected line in UIDL response (%s) skipped."), t);

            uidl.clear();
         }

         uidls.Add(uidl);
      }

      fs_give ((void **) &s);

      if ( !t )
      {
         // test for EOT above succeeded, this is the end
         break;
      }
   }

   return true;
}

// ============================================================================
// global API implementation
// ============================================================================

extern void Pop3_SaveFlags(const String& folderName, MAILSTREAM *stream)
{
   CHECK_RET( stream, _T("Pop3_SaveFlags(): folder is closed") );

   if ( !stream->nmsgs )
   {
      // folder is empty, almost nothing to do: just remove the old cache file
      // as it is not useful any more
      String filename = PopFlagsCacheFile::GetCacheFileName(folderName);
      if ( wxFile::Exists(filename) )
      {
         if ( !wxRemoveFile(filename) )
         {
            wxLogWarning(_("Stale cache file '%s' left."), filename.c_str());
         }
      }

      return;
   }

   wxArrayString uidls;
   if ( Pop3_GetUIDLs(stream, uidls) )
   {
      PopFlagsCacheFile cacheFile(folderName, stream, &uidls);
      cacheFile.SaveFlags();
   }
}

extern void Pop3_RestoreFlags(const String& folderName, MAILSTREAM *stream)
{
   CHECK_RET( stream, _T("Pop3_RestoreFlags(): folder is closed") );

   if ( !stream->nmsgs )
   {
      // nothing to do: no messages - no flags
      return;
   }

   if ( !wxFile::Exists(PopFlagsCacheFile::GetCacheFileName(folderName)) )
   {
      // no cache file - no flags to restore
      return;
   }

   wxArrayString uidls;
   if ( Pop3_GetUIDLs(stream, uidls) )
   {
      PopFlagsCacheFile cacheFile(folderName, stream, &uidls);
      cacheFile.RestoreFlags();
   }
}

