///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   upgrade.cpp - functions to upgrade from previous version of M
// Purpose:     
// Author:      Vadim Zeitlin
// Modified by:
// Created:     20.08.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#include "Mpch.h"

#ifndef   USE_PCH
#  include "Mcommon.h"

#  include "PathFinder.h"
#  include "Profile.h"

#  include "MApplication.h"
#endif  //USE_PCH

#  include "Message.h"
#  include "MailFolder.h"
#  include "MailFolderCC.h"
#  include "SendMessageCC.h"

#ifdef USE_PYTHON
#  include <Python.h>
#  include "PythonHelp.h"
#  include "MScripts.h"
#endif

#include "Mdefaults.h"

#include <wx/log.h>

#include "gui/wxOptionsDlg.h"
#include "gui/wxMDialogs.h"   // for CloseSplash()
#include "Mupgrade.h"

// ----------------------------------------------------------------------------
// symbolic version constants
// ----------------------------------------------------------------------------

// this enum contains only versions with incompatible changes between them
enum MVersion
{
   Version_None,     // this is the first installation of M on this machine
   Version_Alpha001, // first public version
   Version_Alpha002, // some config strucutre changes (due to wxPTextEntry)
   Version_Unknown   // some unreckognized version
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------


static bool
UpgradeFromNone()
{
#if 0 // VZ: this code has no effect!
   // we do it here and only once because it takes a long time
   PathFinder pf(READ_APPCONFIG(MC_PATHLIST));
   pf.AddPaths(M_PREFIX,true);
   bool found;
   String strRootPath = pf.FindDir(READ_APPCONFIG(MC_ROOTDIRNAME), &found);
#endif // 0
   
   wxLog *log = wxLog::GetActiveTarget();
   if ( log ) {
      static const char *msg =
         "As it seems that you're running M for the first\n"
         "time, you should probably set up some of the options\n"
         "needed by the program (especially network parameters).";
      wxLogMessage(_(msg));
      log->Flush();
   }

   ShowOptionsDialog();

   SetupInitialConfig();

   return true;
}

static bool
UpgradeFrom001()
{
   // Config structure incompatible changes from 0.01a to 0.02a:
   // 1) last prompts stored as 0, 1, 2, ... subkeys of /Prompts/foo instead
   //    of a single value /Prompts/foo.
   // 2) last values of the adb editor search control stored as subkeys of
   //    /AdbEditor/LastSearch instead of a single value with this name.
   // 3) /AdbEditor/LastPage is /AdbEditor/Notebook/Page

   // TODO
   return true;
}

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

// upgrade from the specified version to the current one
extern bool
Upgrade(const String& fromVersion)
{
   // what is the old version?
   MVersion oldVersion;
   if ( fromVersion.IsEmpty() ) {
      oldVersion = Version_None;
   }
   else if ( fromVersion == "0.01a" ) {
      oldVersion = Version_Alpha001;
   }
   else if ( fromVersion == "0.02a" ) {
      oldVersion = Version_Alpha002;
   }
   else {
      oldVersion = Version_Unknown;
   }

   // otherwise it would hide our message box(es)
   CloseSplash();

   switch ( oldVersion ) {
      case Version_None:
         UpgradeFromNone();
         break;

      case Version_Alpha001:
         UpgradeFrom001();
         // fall through

      case Version_Alpha002:
         // nothing to do, this is the current version
         wxLogMessage(_("Configuration information and program files were "
                        "successfully upgraded from the version '%s'."),
                      fromVersion.c_str());
         break;

      case Version_Unknown:
         wxLogError(_("The previously installed version of M was probably "
                      "newer than this one. Can not upgrade."));
         return FALSE;

      default:
         FAIL_MSG("invalid version value");
   }

   return TRUE;
}

/** Make sure we have /Profiles/INBOX set up. */
extern bool
SetupInitialConfig(void)
{
   ProfileBase *parent = mApplication->GetProfile();

   ProfilePathChanger(parent,M_PROFILE_CONFIG_SECTION);
   // Do we need to create an INBOX profile?
   if(! parent->HasEntry("INBOX"))
   {
      ProfileBase *ibp = ProfileBase::CreateProfile("INBOX",parent);
      ibp->writeEntry(MP_PROFILE_TYPE,ProfileBase::PT_FolderProfile);
      ibp->writeEntry(MP_FOLDER_TYPE,MailFolder::MF_INBOX);
      ibp->writeEntry(MP_FOLDER_COMMENT,
                     _("Default system folder for incoming mail.")); 
      ibp->DecRef();
   }

#if 0
#if defined ( USE_PYTHON )
   // run a python script to set up things
   PyH_RunMScript(MSCRIPT_USER_SETUP);
#else
#   pragma warning "Missing functionality without Python!"
#endif
#endif

   return true;
}

extern bool
VerifyMailConfig(void)
{
   // we send a mail to ourself

   String me;
   me << READ_APPCONFIG(MP_USERNAME) << '@' << READ_APPCONFIG(MP_HOSTNAME);

   String nil;
   SendMessageCC  sm(mApplication->GetProfile(),
                     String(_("M test message")),
                     me,nil,nil);
   String msg = _("If you can read this, your M configuration works.");
   sm.AddPart(Message::MSG_TYPETEXT, msg.c_str(), msg.length());
   sm.Send();  

   msg.Empty();
   msg << _("Sent email message to:\n")
       << me
       << _("\nPlease check whether it arrives.");
   MDialog_Message(msg, NULL, _("Testing your configuration"), "TestMailSent");

   return true; // till we know something better
}
