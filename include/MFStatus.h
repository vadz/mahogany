///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MFStatus.h
// Purpose:     declares MailFolderStatus struct
// Author:      Vadim Zeitlin
// Modified by:
// Created:     04.07.02 (extracted from MailFolder.h)
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MFSTATUS_H_
#define _MFSTATUS_H_

class MailFolder;

// ----------------------------------------------------------------------------
// MailFolderStatus
// ----------------------------------------------------------------------------

/**
  MailFolderStatus contains the "interesting" and often changing information
  about the folder such as the number of new/unread messages in it
*/

struct MailFolderStatus
{
   explicit MailFolderStatus(unsigned long total = UID_ILLEGAL) : total(total)
   {
   }

   MailFolderStatus(const MailFolderStatus& status) = default;
   MailFolderStatus& operator=(const MailFolderStatus& status) = default;

   bool IsValid() const { return total != UID_ILLEGAL; }

   // do we have any "interesting" messages at all?
   bool HasSomething() const
   {
      return newmsgs || recent || unread || flagged || searched;
   }

   bool operator==(const MailFolderStatus& status)
   {
      return memcmp(this, &status, sizeof(MailFolderStatus)) == 0;
   }

   // the total number of messages and the number of new, recent, unread,
   // important (== flagged) and searched (== results of search) messages in
   // this folder
   //
   // note that unread is the total number of unread messages, i.e. it
   // includes some which are just unseen and the others which are new (i.e.
   // unseen and recent)
   unsigned long total = UID_ILLEGAL,
                 newmsgs = 0,
                 recent = 0,
                 unread = 0,
                 flagged = 0,
                 searched = 0;
};

/**
   Formats a message replacing the occurrences of the format specifiers with the
   data. The format specifiers (listed with their meanings) are:

      %f          folder name
      %t          total number of messages in the folder
      %n          number of new messages in the folder
      %r          number of recent messages in the folder
      %u          number of unseen/unread messages in the folder
      %%          percent sign

   The function uses MailFolderStatus passed to it and will fill it if it is in
   unfilled state and if it needs any of the data in it - so you should reuse
   the same struct when calling this function several times if possible.

   @param format the format string
   @param folderName the full folder name
   @param status the status struct to use and fill
   @param mf the mail folder to use, may be NULL (then folderName is used)
   @return the formatted string
 */
extern String FormatFolderStatusString(const String& format,
                                       const String& folderName,
                                       MailFolderStatus *status,
                                       const MailFolder *mf = NULL);


#endif // _MFSTATUS_H_

