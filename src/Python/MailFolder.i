/*-*- c++ -*-********************************************************
 * MailFolder class: handling of Unix mail folders                  *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$             *
 *******************************************************************/

%module MailFolder

%{
#include   "Mcommon.h"

#include   <wx/config.h>

#include   "kbList.h"
#include   "Profile.h"
#include   "MailFolder.h"

// we don't want to export our functions as we don't build a shared library
#undef SWIGEXPORT
#define SWIGEXPORT(a,b) a b
%}

%import String.i

/**
   MailFolder base class, represents anything containig mails.

   This class defines the interface for all MailFolder classes. It can
   represent anything which contains messages, e.g. folder files, POP3
   or IMAP connections or even newsgroups.

   Before really using an object of this class, one must either
   connect it to a file or open a connection for it.
   */

class MailFolder
{
public:
   /// check wether object is initialised
   bool		IsInitialised(void) const;

   /** Register a FolderViewBase derived class to be notified when
       folder contents change.
       @param 	view the FolderView to register
       @param	reg if false, unregister it
   */
   void RegisterView(FolderViewBase *view, bool reg = true);
   
   /** is mailbox "ok", i.e. was there an error or not?
       @return  true if everything succeeded
   */
   bool	IsOk(void) const;
   
   /** get name of mailbox
       @return	the symbolic name of the mailbox
   */
   const String	& GetName(void) const;

   /** get number of messages
       @return number of messages
   */
   long	CountMessages(void) const;

   /** Check whether mailbox has changed. */
   void Ping(void);
   
   /** Get status of message.
       @param 	msgno	sequence no of message
       @param size if not NULL, size in bytes gets stored here
       @param day to store day (1..31)
       @param month to store month (1..12)
       @param year to store year (19xx)
       @return flags of message
   */
   int	GetMessageStatus(
      unsigned int msgno,
      unsigned long *size = NULL,
      unsigned int *day = NULL,
      unsigned int *month = NULL,
      unsigned int *year = NULL);
   
   /** get the message with number msgno
       @param msgno sequence number
       @return message handler
   */
   class Message *GetMessage(unsigned long msgno);

   /** Delete a message.
       @param index the sequence number
   */
   void DeleteMessage(unsigned long index);

   /** Expunge messages.
     */
   void ExpungeMessages(void);
   
   /** Get the profile.
       @return Pointer to the profile.
   */
   inline ProfileBase *GetProfile(void) { return profile; }
};
