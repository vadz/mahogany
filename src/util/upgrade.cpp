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
#  include "strutil.h"
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
#include <wx/confbase.h>

#include "gui/wxOptionsDlg.h" // for ShowOptionsDialog()
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
   Version_Alpha010, // some config strucutre changes (due to wxPTextEntry)
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
   PathFinder pf(READ_APPCONFIG(MP_PATHLIST));
   pf.AddPaths(M_PREFIX,true);
   bool found;
   String strRootPath = pf.FindDir(READ_APPCONFIG(MP_ROOTDIRNAME), &found);
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
   // 4) /AdbEditor/LastAdb{Dir|File} => AdbFilePrompt/AdbFilePromptPath

   // TODO
   return true;
}

static bool
UpgradeFrom010()
{
   /* Config structure incompatible changes from 0.10a to 0.20a:
      New Profile system. Visible change is that the
      M/Profile/Folders/ hierarchy no longer exists, entries from
      M/Profile/Folders/xxx must be merged into M/Profile/xxx.

      Passwords are now encrypted using an extremly simple encryption
      algorithms. Totally unsafe but better than cleartext.
    */

   // The old Folders section appears as a profile now:
   ProfileBase *p = ProfileBase::CreateProfile("Folders");
   ProfileBase *p2;

   long
      index = 0, index2 = 0;
   String
      group;
   bool
      ok, ok2;
   String
      entry, value, pw;
   kbStringList
      folders;
   
   for ( ok = p->GetFirstGroup(group, index);
         ok ;
         ok = p->GetNextGroup(group, index))
      folders.push_back(new String(group));
      
   for(kbStringList::iterator i = folders.begin(); i != folders.end();i++)
      
   {
      group = **i;
      wxLogMessage(_("Converting information for folder '%s'."),
                   group.c_str());
      p->SetPath(group);
      p2 = ProfileBase::CreateProfile(group);
      index2 = 0;
      for ( ok2 = p->GetFirstEntry(entry, index2);
            ok2 ;
            ok2 = p->GetNextEntry(entry, index2))
      {
         value = p->readEntry(entry, NULL);
         p2->writeEntry(entry, value);
         wxLogDebug("converted entry '%s'='%s'", entry.c_str(), value.c_str());
      }
      pw = p2->readEntry(MP_POP_PASSWORD, MP_POP_PASSWORD_D);
      p2->writeEntry(MP_POP_PASSWORD, strutil_encrypt(pw));
      p2->DecRef();
      p->ResetPath();
      //FIXME causes assert in wxConfig code p->DeleteGroup(group);
   }
   p->DecRef();

   p = ProfileBase::CreateProfile("");
   // We need to rename the old mainfolder, to remove its leading
   // slash:
   String mainFolder = p->readEntry(MP_MAINFOLDER,MP_MAINFOLDER_D);
   if(mainFolder.Length())
   {
      if(mainFolder[0u] == '/')
      {
         mainFolder = mainFolder.Mid(1);
         p->writeEntry(MP_MAINFOLDER, mainFolder);
      }
   }
   // And now we delete the old Folders section altogether
   //FIXME: disabled due to wxFileConfig bug
   //p->DeleteGroup("Folders");

   // Write our new version number:
   p->writeEntry(MP_VERSION, M_VERSION);
   p->DecRef();

   // The old [AdbEditor] section has to be moved to [M/Profiles/AdbEditor].
   wxConfigBase *cf = mApplication->GetProfile()->GetConfig();
   String path = cf->GetPath();
   const char *adbGroup = "/AdbEditor";
   cf->SetPath(adbGroup);
   kbStringList
      groups, entries;

   index=0;
   for ( ok = cf->GetFirstGroup(entry, index);
         ok ;
         ok = cf->GetNextGroup(entry, index))
      groups.push_back(new String(entry));
   index=0;
   for ( ok = cf->GetFirstEntry(entry, index);
         ok ;
         ok = cf->GetNextEntry(entry, index))
      entries.push_back(new String(entry));

   p =  ProfileBase::CreateProfile("AdbEditor");

   // copy all top level entries:
   String *name;
   while((name = entries.pop_front()) != NULL)
   {
      p->writeEntry(*name, cf->Read(*name,String("")));
      delete name;
   }
   String *grp;
   index=0;
   while((grp = groups.pop_front()) != NULL)
   {
      cf->SetPath(*grp);
      for ( ok = cf->GetFirstEntry(entry, index);
            ok ;
            ok = cf->GetNextEntry(entry, index))
         
         p->writeEntry(entry, cf->Read(entry,String("")));
      cf->SetPath("..");
      delete grp;
   }
   cf->SetPath(path);
   //FIXME: broken!! cf->DeleteGroup(adbGroup);
   p->DecRef();
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
   if ( fromVersion.IsEmpty() )
      oldVersion = Version_None;
   else if ( fromVersion == "0.01a" )
      oldVersion = Version_Alpha001;
   else if ( fromVersion == "0.02a" || fromVersion == "0.10a")
      oldVersion = Version_Alpha010;
   else
      oldVersion = Version_Unknown;

   // otherwise it would hide our message box(es)
   CloseSplash();

   switch ( oldVersion )
   {
      case Version_None:
         UpgradeFromNone();
         break;

      case Version_Alpha001:
         UpgradeFrom001();
         // fall through

      case Version_Alpha010:
         if ( UpgradeFrom010() )
         {
            wxLogMessage(_("Configuration information and program files were "
                           "successfully upgraded from the version '%s'."),
                         fromVersion.c_str());
         }
         break;

      default:
         FAIL_MSG("invalid version value");
         // fall through

      case Version_Unknown:
         wxLogError(_("The previously installed version of M was probably "
                      "newer than this one. Can not upgrade."));
         return FALSE;
   }

   return TRUE;
}

/** Make sure we have /Profiles/INBOX set up and the global
    NewMailFolder folder.
    Returns TRUE if the profile already existed, FALSE if it was just created
 */
extern bool
VerifyInbox(void)
{
   bool rc;
   ProfileBase *parent = mApplication->GetProfile();
   // Do we need to create the INBOX (special folder for incoming mail)?
   if ( parent->HasEntry("INBOX") )
      rc = TRUE;
   else
   {
      rc = FALSE;
      ProfileBase *ibp = ProfileBase::CreateProfile("INBOX");
      ibp->writeEntry(MP_PROFILE_TYPE, ProfileBase::PT_FolderProfile);
      ibp->writeEntry(MP_FOLDER_TYPE, MF_INBOX);
      ibp->writeEntry(MP_FOLDER_COMMENT, _("Default system folder for incoming mail."));
      ibp->DecRef();
   }
   
   // Is the newmail folder properly configured?
   String foldername = READ_APPCONFIG(MP_NEWMAIL_FOLDER);
   strutil_delwhitespace(foldername);
   if(foldername.IsEmpty()) // this must not be
      foldername = MP_NEWMAIL_FOLDER_D; // reset to default
   // Do we need to create the NewMailFolder?
   if (!  parent->HasEntry(foldername) )
   {
      ProfileBase *ibp = ProfileBase::CreateProfile(foldername);
      ibp->writeEntry(MP_PROFILE_TYPE, ProfileBase::PT_FolderProfile);
      ibp->writeEntry(MP_FOLDER_TYPE, MF_FILE);
      ibp->writeEntry(MP_FOLDER_PATH, strutil_expandfoldername(foldername));
      ibp->writeEntry(MP_FOLDER_COMMENT,
                      _("Default system folder for incoming mail."));
      ibp->DecRef();
      rc = FALSE;
   }
   return rc;
}

/** Make sure we have all "vital" things set up. */
extern bool
SetupInitialConfig(void)
{
   (void)VerifyInbox();

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

// FIXME we should make sure that hostname can be resolved - otherwise this
// message will stay in sendmail queue _forever_ (at least it does happen
// here with sendmail 8.8.8 running under Solaris 2.6)
   String nil;
   SendMessageCC  sm(mApplication->GetProfile());
   sm.SetSubject(_("M test message"));
   sm.SetAddresses(me);
   String msg =
      _("If you can read this, your M configuration works.\n"
        "You should also try to reply to this mail and check that your reply arrives.");
   sm.AddPart(Message::MSG_TYPETEXT, msg.c_str(), msg.length());
   sm.Send();

   msg.Empty();
   msg << _("Sent email message to:\n")
       << me
       << _("\n\nPlease check whether it arrives.");
   MDialog_Message(msg, NULL, _("Testing your configuration"), "TestMailSent");

   return true; // till we know something better
}
