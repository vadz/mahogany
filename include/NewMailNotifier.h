///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   NewMailNotifier.h
// Purpose:     Declares class used for generating new mail notifications.
// Author:      Vadim Zeitlin
// Created:     2012-07-31
// Copyright:   (C) 2012 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef M_NEWMAILNOTIFIER_H
#define M_NEWMAILNOTIFIER_H

#include <vector>

/**
    New mail notifier collects information about new mail messages and then
    reports them all at once to the user.

    This class is used to avoid giving many different new mail notifications
    when opening a folder containing a lot of new messages that are then
    filtered into other folders.

    It doesn't decide for which folders are the notifications generated, this
    depends on the options used by MailFolderCmn::ReportNewMail(), but only
    postpones showing them to the user slightly.

    To bundle notifications together, simply create an object of this class,
    then run the code resulting in the calls to static methods adding
    notifications and they will all be shown when the object is destroyed.

    If no active notifier exists, the notifications are shown immediately.
 */
class NewMailNotifier
{
public:
   /**
       Struct containing the information about a single new message.
    */
   struct MsgInfo
   {
      MsgInfo(const String& from_, const String& subject_) :
         from(from_), subject(subject_)
      {
      }

      String
         from,
         subject;
   };

   typedef std::vector<MsgInfo> MsgInfos;


   /**
       Constructor creates a global new notifier.

       If a notifier already exists, constructing a new doesn't do anything and
       the notification will only be shown when the existing notifier is
       destroyed.
    */
   NewMailNotifier();

   /**
       Destructor shows all the notifications accumulated during the life-time
       of this notifier object.

       If this is the last existing notifier object, all notifications are
       shown, otherwise they will be shown later when the last notifier is
       destroyed.

       Dtor is not virtual as this class is not supposed to be used
       polymorphically.
    */
   ~NewMailNotifier();

   /**
       Show possibly delayed notification about new mail in the given folder.

       If a NewMailNotifier object currently exists, the notification will be
       shown (combined with the notifications about email in any other folders)
       when it is destroyed. Otherwise the notification is shown immediately.

       @param folderName The name of the folder in which new mail arrived.
       @param countNew The number of new messages, always > 0.
       @param infos Detailed information about the new messages, may contain
         less elements than @a countNew, e.g. can be empty if the folder wasn't
         opened.
    */
   static void
   DoForFolder(const String& folderName,
               unsigned long countNew,
               const MsgInfos& infos);

private:
   wxDECLARE_NO_COPY_CLASS(NewMailNotifier);
};

#endif // M_NEWMAILNOTIFIER_H
