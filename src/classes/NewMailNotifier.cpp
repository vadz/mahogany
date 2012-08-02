///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   NewMailNotifier.cpp
// Purpose:     NewMailNotifier implementation.
// Author:      Vadim Zeitlin
// Created:     2012-07-31
// Copyright:   (C) 2012 Vadim Zeitlin <vadim@wxwindows.org>
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
#endif // USE_PCH

#include "NewMailNotifier.h"

#include "Address.h"

#include <wx/notifmsg.h>

#include <map>

// ----------------------------------------------------------------------------
// global variables
// ----------------------------------------------------------------------------

// Number of currently existing notifiers.
static unsigned gs_numNotifiers = 0;

// ============================================================================
// Notification implementation
// ============================================================================

namespace
{

// This struct contains information about new mail in a single folder.
struct FolderNewMailInfo
{
   FolderNewMailInfo(const String& folderName_,
         unsigned long countNew_,
         const NewMailNotifier::MsgInfos& infos_) :
      folderName(folderName_),
      countNew(countNew_),
      infos(infos_)
   {
   }

   String folderName;
   unsigned long countNew;
   NewMailNotifier::MsgInfos infos;
};

// All currently queued new mail notifications.
std::vector<FolderNewMailInfo> g_newMail;

// Function building detailed notification message for new email in the given
// folder. This is used when there is only one folder with new email right now
// so there is no need to mention the folder name in the main message itself.
String BuildFullNotificationMessage(const NewMailNotifier::MsgInfos& infos)
{
   // Maybe we should avoid repeating the sender name, i.e. instead of
   // showing "Sender: subject one", "Sender: subject two" show just
   // "Sender: subject one, subject two"?
   String message;
   for ( size_t i = 0; i < infos.size(); i++ )
   {
      const NewMailNotifier::MsgInfo& info = infos[i];

      if ( !message.empty() )
         message << '\n';

      String sender = Address::GetDisplayAddress(info.from);
      if ( !sender.empty() )
         message << sender << ": ";

      message << '"' << info.subject << '"';
   }

   return message;
}

// Function building brief notification message for new email in the given
// folder. This is used when there is more than one folder with the new mail.
String
BuildBriefNotificationMessage(
      const NewMailNotifier::MsgInfos& infos,
      const String& folderName
   )
{
   // Show only the sender names: this is shorter and usually more informative
   // than the subjects.
   //
   // Also count the number of emails from each sender to avoid repeating them
   // unnecessarily.
   typedef std::map<String, unsigned> StringToCount;
   StringToCount senders;
   for ( size_t i = 0; i < infos.size(); i++ )
   {
      String sender = Address::GetDisplayAddress(infos[i].from);
      if ( sender.empty() )
         sender = _("unknown sender");

      senders[sender]++;
   }

   // Now show the first few of them, with the repeat count if necessary.
   String allSenders;
   unsigned numSenders = 0;
   for ( StringToCount::const_iterator it = senders.begin();
         it != senders.end();
         ++it )
   {
      if ( numSenders )
         allSenders += ", ";

      if ( it->second == 1 )
         allSenders += it->first;
      else
         allSenders += String::Format("%s (%u)", it->first, it->second);

      // TODO: Don't hard code the maximal number of senders.
      if ( numSenders++ > 3 )
      {
         allSenders += ", ...";
         break;
      }
   }

   return wxString::Format(_("In %s: from %s"), folderName, allSenders);
}


// This function generates a notification about new mail in the given number of
// folders, using the information from the folders array of the corresponding
// size.
void DoNotify(unsigned long numFolders, const FolderNewMailInfo* folders)
{
   wxNotificationMessage notification;

   if ( numFolders == 1 )
   {
      // Simple case: just show all the information we have.
      notification.SetTitle(
         folders->countNew == 1
            ? String::Format(
                 _("New email in folder \"%s\""), folders->folderName
              )
            : String::Format(
                  _("%lu new messages in folder \"%s\""),
                  folders->countNew,
                  folders->folderName
               )
      );

      notification.SetMessage(BuildFullNotificationMessage(folders->infos));
   }
   else // New mail in more than one folder
   {
      unsigned long n;

      // Compute the total number of new messages and their common prefix, if
      // any.

      // The prefix will include the trailing slash at the end but while we're
      // determining it, it doesn't contain it, so take care to append to it
      // when checking if a folder name starts with it to avoid deciding that
      // a top level "Foobar" folder is under "Foo" parent.
      String commonParent;
      unsigned long totalNew = 0;
      for ( n = 0; n < numFolders; n++ )
      {
         totalNew += folders[n].countNew;

         // We try to show all the folder names under the common parent, if
         // possible.
         const String& folderName = folders[n].folderName;
         if ( n == 0 )
         {
            commonParent = folderName;
         }
         else if ( !commonParent.empty() )
         {
            // Find longest common prefix.
            while ( !folderName.StartsWith(commonParent + '/') )
            {
               commonParent = commonParent.BeforeLast('/');
               if ( commonParent.empty() )
                  break;
            }
         }
         //else: there is no common parent
      }

      if ( !commonParent.empty() )
         commonParent += '/';


      // Concatenate brief summaries for each folder to make the entire message.
      String message;
      for ( n = 0; n < numFolders; n++ )
      {
         if ( !message.empty() )
            message += '\n';

         // Show only the unique parts of the folder names.
         String folderName = folders[n].folderName;
         folderName.erase(0, commonParent.length());

         message += BuildBriefNotificationMessage(folders[n].infos, folderName);
      }

      notification.SetMessage(message);

      String title;
      if ( commonParent.empty() )
      {
         title.Printf(
            _("%lu new messages in %lu folders."),
            totalNew, numFolders
         );
      }
      else // Show the parent to make folder names in the message unambiguous.
      {
         title.Printf(
            _("%lu new messages in %lu folders under \"%s\"."),
            totalNew, numFolders, commonParent
         );
      }

      notification.SetTitle(title);
   }

#if defined(__WXGTK__) && wxUSE_LIBNOTIFY
   // Use a more appropriate stock icon under GTK.
   notification.GTKSetIconName("mail-message-new");
#endif

   notification.Show();
}

} // anonymous namespace

// ============================================================================
// NewMailNotifier implementation
// ============================================================================

NewMailNotifier::NewMailNotifier()
{
   gs_numNotifiers++;
}

NewMailNotifier::~NewMailNotifier()
{
   ASSERT_MSG( gs_numNotifiers > 0, "Notifier destruction mismatch" );

   if ( --gs_numNotifiers )
      return;

   if ( !g_newMail.empty() )
   {
      DoNotify(g_newMail.size(), &g_newMail[0]);
      g_newMail.clear();
   }
}

/* static */
void
NewMailNotifier::DoForFolder(const String& folderName,
      unsigned long countNew,
      const MsgInfos& infos)
{
   FolderNewMailInfo folderNewMailInfo(folderName, countNew, infos);

   if ( gs_numNotifiers )
      g_newMail.push_back(folderNewMailInfo);
   else
      DoNotify(1, &folderNewMailInfo);
}
