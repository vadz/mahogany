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

#  include "Mdefaults.h"
#ifdef USE_PYTHON
#  include "Python.h"
#  include "PythonHelp.h"
#endif
#endif  //USE_PCH

#include <wx/log.h>

#include "gui/wxOptionsDlg.h"
#include "gui/wxMDialogs.h"   // for CloseSplash()

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

bool UpgradeFromNone()
{
   // FIXME I don't remember any more what this is supposed to do. Karsten?
#  if 0 //def OS_UNIX
      // we do it here and only once because it takes a long time
      PathFinder pf(READ_APPCONFIG(MC_PATHLIST));
      pf.AddPaths(M_PREFIX,true);
      bool found;
      String strRootPath = pf.FindDir(READ_APPCONFIG(MC_ROOTDIRNAME), &found);
#  endif // Unix
   
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
#if defined ( USE_PYTHON )
   // run a python script to set up things
   PyH_RunScript(MSCRIPT_USER_SETUP);
#else
#   pragma warning "Missing functionality without Python!"
#endif
   return true;
}

bool UpgradeFrom001()
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
bool Upgrade(const String& fromVersion)
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

