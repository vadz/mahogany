///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/MailMH.cpp - support functions for MH folders
// Purpose:     provide functions to init MH support, work with MH folder
//              names, import MH folders &c
// Author:      Vadim Zeitlin
// Modified by:
// Created:     03.05.00
// CVS-ID:      $Id$
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "Mcclient.h"

#  include "strutil.h"

#ifdef OS_WIN // cygwin and mingw
#  include "MApplication.h" // for mApplication
#endif
#endif // USE_PCH

#include "ASMailFolder.h"
#include "MFolder.h"
#include "MEvent.h"

#include <wx/dir.h>
#include <wx/filefn.h>

extern "C"
{
   // including mh.h doesn't seem to work...
   int mh_isvalid(char *name, char *tmp, long synonly);

   char *mh_getpath(void);
}

class MOption;

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_USERDIR;

// ----------------------------------------------------------------------------
// globals
// ----------------------------------------------------------------------------

// the root of MH folders
static String gs_MHRootDir;

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// a class which receives mm_list notifications for MH folders and imports
// them (i.e. creates the entries for them in the folder tree)
class MHFoldersImporter : public MEventReceiver
{
public:
   MHFoldersImporter();
   virtual ~MHFoldersImporter();

   bool IsOk() const { return m_ok; }

   // event processing function
   virtual bool OnMEvent(MEventData& event);

private:
   // called when a new folder must be added
   void OnNewFolder(String& name);

   // MEventReceiver cookie for the event manager
   void *m_regCookie;

   // did we finish importing folders successfully?
   bool m_ok;
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MHFoldersImporter
// ----------------------------------------------------------------------------

MHFoldersImporter::MHFoldersImporter()
{
   m_ok = false;

   m_regCookie = MEventManager::Register(*this, MEventId_ASFolderResult);
   ASSERT_MSG( m_regCookie, _T("can't register with event manager"));
}

MHFoldersImporter::~MHFoldersImporter()
{
   MEventManager::Deregister(m_regCookie);
}

// needed to be able to use DECLARE_AUTOREF() macro
typedef ASMailFolder::ResultFolderExists ASFolderExistsResult;
DECLARE_AUTOPTR(ASFolderExistsResult);

bool MHFoldersImporter::OnMEvent(MEventData& event)
{
   // we're only subscribed to the ASFolder events
   CHECK( event.GetId() == MEventId_ASFolderResult, FALSE,
          _T("unexpected event type") );

   MEventASFolderResultData &data = (MEventASFolderResultData &)event;

   ASFolderExistsResult_obj result((ASFolderExistsResult *)data.GetResult());

   // is this message really for us?
   if ( result->GetUserData() != this )
   {
      // no: continue with other event handlers
      return TRUE;
   }

   if ( result->GetOperation() != ASMailFolder::Op_ListFolders )
   {
      FAIL_MSG( _T("unexpected operation notification") );

      // eat the event - it was for us but we didn't process it...
      return FALSE;
   }

   // is it the special event which signals that there will be no more of
   // folders?
   if ( !result->GetDelimiter() )
   {
      m_ok = TRUE;
   }
   else
   {
      // we're passed a folder specification - extract the folder name from it
      // (it's better to show this to the user rather than cryptic cclient
      // string)
      wxString name,
               spec = result->GetName();
      if ( MailFolder::SpecToFolderName(spec, MF_MH, &name) )
      {
         OnNewFolder(name);
      }
      else
      {
         wxLogDebug(_T("Folder specification '%s' unexpected."), spec.c_str());
      }
   }

   // we don't want anyone else to receive this message - it was for us only
   return FALSE;
}

void MHFoldersImporter::OnNewFolder(String& name)
{
   wxLogMessage(_T("Found MH folder %s"), name.c_str());
}

// ----------------------------------------------------------------------------
// implement MailFolder API
// ----------------------------------------------------------------------------

const String&
MailFolder::InitializeMH()
{
   if ( !gs_MHRootDir )
   {
      // first, init cclient
      MailFolder::Init();

      // normally, the MH path is read by MH cclient driver from the MHPROFIle
      // file (~/.mh_profile under Unix), but we can't rely on this because if
      // this file is not found, it results in an error and the MH driver is
      // disabled - though it's perfectly ok for this file to be empty... So
      // we can use cclient MH logic if this file exists - but not if it
      // doesn't (and never under Windows)
#ifdef OS_UNIX
      String home = wxGetenv(_T("HOME"));
      String filenameMHProfile = home + _T("/.mh_profile");
      FILE *fp = wxFopen(filenameMHProfile, _T("r"));
      if ( fp )
      {
         fclose(fp);
      }
      else
#endif // OS_UNIX
      {
         // need to find MH path ourself
#ifdef OS_UNIX
         // the standard location under Unix
         String pathMH = home + _T("/Mail");
#else // !Unix
         // use the user directory by default
         String pathMH = READ_APPCONFIG(MP_USERDIR);
#endif // Unix/!Unix

         // const_cast is harmless
         mail_parameters(NULL, SET_MHPATH, (char *)pathMH.c_str());
      }

      // force cclient to init the MH driver
      char tmp[MAILTMPLEN];
      if ( !mh_isvalid("#MHINBOX", tmp, TRUE /* syn only check */) )
      {
         wxLogError(_("Sorry, support for MH folders is disabled."));
      }
      else
      {
         // retrieve the MH path (notice that we don't always find it ourself
         // as sometimes it's found only by the call to mh_isvalid)
         gs_MHRootDir = (wxChar *)mail_parameters(NULL, GET_MHPATH, &tmp);

         // the path should have a trailing [back]slash
         if ( !!gs_MHRootDir && !wxIsPathSeparator(gs_MHRootDir.Last()) )
         {
            gs_MHRootDir << wxFILE_SEP_PATH;
         }
      }
   }

   return gs_MHRootDir;
}

bool
MailFolder::GetMHFolderName(String *path)
{
   String& name = *path;

   if ( wxIsAbsolutePath(name) )
   {
      if ( !InitializeMH() ) // it's harmless to call it more than once
      {
         // no MH support
         return FALSE;
      }

      wxString pathFolder(name, gs_MHRootDir.length());
      if ( strutil_compare_filenames(pathFolder, gs_MHRootDir) )
      {
         // skip MH path (and trailing slash which follows it)
         name = name.c_str() + gs_MHRootDir.length();
      }
      else
      {
         wxLogError(_("Invalid MH folder name '%s' - all MH folders should "
                      "be under '%s' directory."),
                    name.c_str(),
                    gs_MHRootDir.c_str());

         return FALSE;
      }
   }
   //else: relative path - leave as is

   return TRUE;
}

bool MailFolder::ExistsMH()
{
   // check if any MH folders exist
   wxString rootMH = InitializeMH();
   if ( !rootMH )
   {
      // no MH support
      return FALSE;
   }

   // is the dir empty?
   wxDir dir(rootMH);
   wxString dummy;
   if ( dir.GetFirst(&dummy) )
   {
      // we have something, assume it's a valid MH folder
      return TRUE;
   }

   // empty directory
   return FALSE;
}

bool MailFolder::ImportFoldersMH(const String& root, bool allUnder)
{
   bool ok = TRUE;

   // change the MH path if it's different from the default one
   if ( root != InitializeMH() )
   {
      // const_cast is harmless
      mail_parameters(NULL, SET_MHPATH, (char *)root.c_str());
   }

   // first create the root MH folder
   MFolder *folderMH = CreateFolderTreeEntry(NULL,    // top level folder
                                             _("MH folders"),
                                             MF_MH,
                                             0,       // flags
                                             _T(""),
                                             FALSE);  // don't notify
   if ( !folderMH )
   {
      wxLogError(_("Failed to create root MH folder at '%s'."),
                 root.c_str());

      ok = FALSE;
   }

   if ( ok && allUnder )
   {
      // enum all MH folders and import them
      ASMailFolder *asmf = ASMailFolder::HalfOpenFolder(folderMH);
      if ( !asmf )
      {
         ok = FALSE;
      }
      else
      {
         MHFoldersImporter importer;
         asmf->ListFolders(_T("*"), FALSE, _T(""), &importer);

         ok = importer.IsOk();

         asmf->DecRef();
      }

      if ( !ok )
      {
         wxLogError(_("Failed to import MH subfolders under '%s'."),
                    root.c_str());
      }
   }

   SafeDecRef(folderMH);

   return ok;
}
