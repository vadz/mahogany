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
#include <wx/utils.h>         // wxGetFullHostName()

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
   Version_Alpha020, // folder host name is now ServerName, not HostName
   Version_NoChange, // any version from which we don't need to upgrade
   Version_Unknown   // some unrecognized version
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
   wxLog *log = wxLog::GetActiveTarget();
   if ( log ) {
      static const char *msg =
         "As it seems that you are running Mahogany for the first\n"
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

#define COPYENTRY(type)  { type val; rc &= _this->Read(entry, &val); rc &= dest->Write(newentry,val); }

/** Copies all entries and optionally subgroups from path from to path
    to in the wxConfig.
    NOTE: Currently both from and to should be absolute paths!
    @param from  absolute group from where to copy entries
    @param to    absolute group where to copy to
    @param recursive if true, copy all subgroups, too
    @param dest      if non-NULL, use that config as the destination otherwise use this
    @return false on error
*/
static bool
CopyEntries(wxConfigBase *_this,
            const wxString &from, const wxString &to,
            bool recursive = TRUE,
            wxConfigBase *dest = NULL)
{
   wxString oldPath = _this->GetPath();
   bool rc = TRUE;

   if(dest == NULL) dest = _this;

   // Build a list of all entries to copy:
   _this->SetPath(from);

   long
      index = 0;
   wxString
      entry, newentry;
   bool ok;
   for ( ok = _this->GetFirstEntry(entry, index);
         ok ;
         ok = _this->GetNextEntry(entry, index))
   {
      newentry = to;
      newentry << '/' << entry;
      switch(_this->GetEntryType(entry))
      {
      case wxConfigBase::Type_String:
         COPYENTRY(wxString); break;
      case wxConfigBase::Type_Integer:
         COPYENTRY(long); break;
      case wxConfigBase::Type_Float:
         COPYENTRY(double); break;
      case wxConfigBase::Type_Boolean:
         COPYENTRY(bool); break;
      case wxConfigBase::Type_Unknown:
         wxFAIL_MSG("unexpected entry type");
      }
   }
   if(recursive)
   {
      wxString
         fromgroup, togroup;
      wxString
         *groups;

      size_t
         idx = 0,
         n = _this->GetNumberOfGroups(FALSE);
      if(n > 0)
      {
         groups = new wxString[n];
         index = 0;
         for ( ok = _this->GetFirstGroup(entry, index);
               ok ;
               ok = _this->GetNextGroup(entry, index))
         {
            wxASSERT(idx < n);
            groups[idx++] = entry;
         }
         for(idx = 0; idx < n; idx++)
         {
            fromgroup = from;
            fromgroup << '/' << groups[idx];
            togroup = to;
            togroup << '/' << groups[idx];
            rc &= CopyEntries(_this, fromgroup, togroup, recursive, dest);
         }
         delete [] groups;
      }
   }

   _this->SetPath(oldPath);
   return rc;
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

   bool rc = true;

   //FIXME paths need adjustment for windows?
   rc &= CopyEntries(mApplication->GetProfile()->GetConfig(),
                     "/M/Profiles/Folders","/M/Profiles", true);

   rc &= CopyEntries(mApplication->GetProfile()->GetConfig(),
                     "/AdbEditor","/M/Profiles/AdbEditor", true);

   ProfileBase
      *p = ProfileBase::CreateProfile(""),
      *p2;
   kbStringList
      folders;
   String
      group, pw, tmp;
   long
      index = 0;
   // We need to rename the old mainfolder, to remove its leading
   // slash:
   String
      mainFolder = p->readEntry(MP_MAINFOLDER,MP_MAINFOLDER_D);
   if(mainFolder.Length())
   {
      if(mainFolder[0u] == '/')
      {
         wxString tmp = mainFolder.Mid(1);
         mainFolder = tmp;
         p->writeEntry(MP_MAINFOLDER, mainFolder);
      }
   }

   //FIXME paths need adjustment for windows?
   wxConfigBase *c = mApplication->GetProfile()->GetConfig();
   // Delete obsolete groups:
   c->DeleteGroup("/M/Profiles/Folders");
   c->DeleteGroup("/AdbEditor");

   /* Encrypt passwords in new location and make sure we have no
      illegal old profiles around. */
   p->ResetPath(); // to be safe
   for ( bool ok = p->GetFirstGroup(group, index);
         ok ;
         ok = p->GetNextGroup(group, index))
   {
      tmp = group;
      tmp << '/' << MP_PROFILE_TYPE;
      if(p->readEntry(tmp, MP_PROFILE_TYPE_D) == ProfileBase::PT_FolderProfile)
         folders.push_back(new String(group));
   }
   for(kbStringList::iterator i = folders.begin(); i != folders.end();i++)
   {
      group = **i;
      p->SetPath(group);
      if(p->readEntry(MP_FOLDER_TYPE, MP_FOLDER_TYPE_D) != MP_FOLDER_TYPE_D)
      {
         p2 = ProfileBase::CreateProfile(group);
         pw = p2->readEntry(MP_FOLDER_PASSWORD, MP_FOLDER_PASSWORD_D);
         if(pw.Length()) // only if we have a password
            p2->writeEntry(MP_FOLDER_PASSWORD, strutil_encrypt(pw));
         p2->DecRef();
         p->ResetPath();
      }
      else
      {
         p->ResetPath();
         p->DeleteGroup(group);
         String msg = _("Deleted illegal folder profile: '");
         msg << p->GetName() << '/' << group << '\'';
         wxLogMessage(msg);
      }
   }
   p->DecRef();
   //FIXME check returncodes!


   return rc;

}



class UpgradeFolderTraversal : public MFolderTraversal
{
public:
   UpgradeFolderTraversal(MFolder* folder) : MFolderTraversal(*folder)
      { }

   virtual bool OnVisitFolder(const wxString& folderName)
      {
         Profile_obj profile(folderName);
         bool found;
         String hostname = profile->readEntry(MP_OLD_FOLDER_HOST, "", &found);
         if ( found )
         {
            // delete the old entry, create the new one
            wxConfigBase *config = profile->GetConfig();
            if ( config )
            {
               config->DeleteEntry(MP_OLD_FOLDER_HOST);

               wxLogTrace("Successfully converted folder '%s'",
                          folderName.c_str());
            }
            else
            {
               // what can we do? nothing...
               FAIL_MSG( "profile without config - can't delete entry" );
            }

            profile->writeEntry(MP_FOLDER_HOST, hostname);
         }

         return TRUE;
      }
};

static bool
UpgradeFrom020()
{
   /* Config structure incompatible changes from 0.20a to 0.21a:
      in the folder settings, HostName has become ServerName
    */

   // enumerate all folders recursively
   MFolder_obj folderRoot("");
   UpgradeFolderTraversal traverse(folderRoot);
   traverse.Traverse();

   // TODO it would be very nice to purge the redundant settings from config
   //      because the older versions wrote everything to it and it has only
   //      been fixed in 0.21a -- but this seems a bit too complicated

   return TRUE;
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
   else if ( fromVersion == "0.20a" )
      oldVersion = Version_Alpha020;
   else if ( fromVersion == "0.21a" )
      oldVersion = Version_NoChange;
   else
      oldVersion = Version_Unknown;

   // otherwise it would hide our message box(es)
   CloseSplash();

   bool success = TRUE;
   switch ( oldVersion )
   {
   case Version_None:
      UpgradeFromNone();
      break;

   case Version_Alpha001:
      if ( success )
         success = UpgradeFrom001();
      // fall through

   case Version_Alpha010:
      if ( success )
         success = UpgradeFrom010();
      // fall through

   case Version_Alpha020:
      if ( success && UpgradeFrom020() )
         wxLogMessage(_("Configuration information and program files were "
                        "successfully upgraded from the version '%s'."),
                      fromVersion.c_str());
      else
         wxLogError(_("Configuration information and program files "
                      "could not be upgraded from version '%s', some "
                      "settings might be lost.\n"
                      "\n"
                      "It is recommended that you uninstall and reinstall "
                      "the program before using it."),
                    fromVersion.c_str());
      break;
   case Version_NoChange:
      break;
   default:
      FAIL_MSG("invalid version value");
      // fall through

   case Version_Unknown:
      wxLogError(_("The previously installed version of Mahogany was "
                   "probably newer than this one. Cannot upgrade."));
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
   // INBOX has no meaning under Windows
#ifndef OS_WIN
   // Do we need to create the INBOX (special folder for incoming mail)?
   if ( parent->HasEntry("INBOX") )
      rc = TRUE;
   else
   {
      rc = FALSE;
      ProfileBase *ibp = ProfileBase::CreateProfile("INBOX");
      ibp->writeEntry(MP_PROFILE_TYPE, ProfileBase::PT_FolderProfile);
      if(READ_APPCONFIG(MP_NEWMAIL_FOLDER) != "INBOX"
         && MDialog_YesNoDialog(
            _("Normally Mahogany will automatically collect all mail\n"
              "from your system's default mail folder (INBOX,\n"
              "representing your mail spool entry),\n"
              "and move it to a special 'New Mail' folder.\n"
              "This is generally considered good practice as you\n"
              "should not leave your new mail in INBOX.\n"
              "\n"
              "Please confirm this. If you select No, your mail\n"
              "will remain in INBOX and you need to check that\n"
              "folder manually."
               ),
            NULL, _("Collect mail from INBOX?"), true))
         ibp->writeEntry(MP_FOLDER_TYPE, MF_INBOX|MF_FLAGS_INCOMING);
      else
      {
         ibp->writeEntry(MP_FOLDER_TYPE, MF_INBOX);
         MDialog_Message(_(
            "Mahogany will not automatically collect mail from your\n"
            "system's default INBOX.\n"
            "You can change this in the INBOX folder's preferences\n"
            "dialog at any time."));

      }
      ibp->writeEntry(MP_FOLDER_COMMENT, _("Default system folder for incoming mail."));
      ibp->DecRef();
   }
#endif

   // Is the newmail folder properly configured?
   String foldername = READ_APPCONFIG(MP_NEWMAIL_FOLDER);
   strutil_delwhitespace(foldername);
   if(foldername.IsEmpty()) // this must not be
      foldername = MP_NEWMAIL_FOLDER_D; // reset to default
   // Do we need to create the NewMailFolder?
   ProfileBase *ibp = ProfileBase::CreateProfile(foldername);
   if (!  parent->HasEntry(foldername) )
   {
      ibp->writeEntry(MP_PROFILE_TYPE, ProfileBase::PT_FolderProfile);
      ibp->writeEntry(MP_FOLDER_TYPE, MF_FILE);
      ibp->writeEntry(MP_FOLDER_PATH, strutil_expandfoldername(foldername));
      ibp->writeEntry(MP_FOLDER_COMMENT,
                      _("Default system folder for incoming mail."));
      rc = FALSE;
   }
   if(READ_CONFIG(ibp,MP_FOLDER_TYPE) & MF_FLAGS_INCOMING)
      ibp->writeEntry(MP_FOLDER_TYPE, (READ_CONFIG(ibp, MP_FOLDER_TYPE) ^ MF_FLAGS_INCOMING));
   ibp->DecRef();
   return rc;
}

/** Make sure we have all "vital" things set up. */
extern bool
SetupInitialConfig(void)
{
   String host = READ_APPCONFIG(MP_HOSTNAME);
   if(host.Length() == 0)
      mApplication->GetProfile()->writeEntry(MP_HOSTNAME,wxGetFullHostName());
   
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
   me = READ_APPCONFIG(MP_RETURN_ADDRESS);
   if(me.Length() == 0)
      me << READ_APPCONFIG(MP_USERNAME) << '@' << READ_APPCONFIG(MP_HOSTNAME);

// FIXME we should make sure that hostname can be resolved - otherwise this
// message will stay in sendmail queue _forever_ (at least it does happen
// here with sendmail 8.8.8 running under Solaris 2.6)
   String nil;
   SendMessageCC  sm(mApplication->GetProfile());
   sm.SetSubject(_("Mahogany Test Message"));
   sm.SetAddresses(me);
   String msg =
      _("If you have received this mail, your Mahogany configuration works.\n"
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
