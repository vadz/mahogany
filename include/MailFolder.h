/*-*- c++ -*-********************************************************
 * MailFolder class: handling of Unix mail folders                  *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$             *
 *******************************************************************/

#ifndef MAILFOLDER_H
#define MAILFOLDER_H

#ifndef	USE_PCH
#	include	"Profile.h"
#	include	"FolderView.h"
#endif

#define	MSG_STAT_UNREAD    1
#define	MSG_STAT_DELETED   2
#define	MSG_STAT_REPLIED   4
#define	MSG_STAT_RECENT    8
#define	MSG_STAT_SEARCHED 16

/**
   MailFolder base class, represents anything containig mails.

   This class defines the interface for all MailFolder classes. It can
   represent anything which contains messages, e.g. folder files, POP3
   or IMAP connections or even newsgroups.

   Before really using an object of this class, one must either
   connect it to a file or open a connection for it.
   */

class MailFolder : public CommonBase
{
public:   
   /** @name Constructors and destructor */
   //@{

   /// default destructor
   virtual ~MailFolder();

   //@}

   Profile 	*profile;
   
protected:
   /** open an existing mailbox:
       @param	filename the name of the "file" to open
       @return	true if successful
       */
   virtual bool	Open(String const & filename) = 0;

public:
   /// close the folder
   virtual void Close(void) = 0;
   
   /// check wether object is initialised
   virtual bool		IsInitialised(void) const = 0;

   /** Register a FolderViewBase derived class to be notified when
       folder contents change.
       @param 	view the FolderView to register
       @param	reg if false, unregister it
   */
   virtual void RegisterView(FolderViewBase *view, bool reg = true) = 0;
   
   /** is mailbox "ok", i.e. was there an error or not?
       @return  true if everything succeeded
   */
   virtual bool	IsOk(void) const = 0;

   /** get name of mailbox
       @return	the symbolic name of the mailbox
   */
   virtual const String	& GetName(void) const = 0;

   /** get number of messages
       @return number of messages
   */
   virtual	long	CountMessages(void) const = 0;

   /** Check whether mailbox has changed. */
   virtual void Ping(void) = 0;
   
   /** Get status of message.
       @param 	msgno	sequence no of message
       @param size if not NULL, size in bytes gets stored here
       @param day to store day (1..31)
       @param month to store month (1..12)
       @param year to store year (19xx)
       @return flags of message
   */
   virtual int	GetMessageStatus(
      unsigned int msgno,
      unsigned long *size = NULL,
      unsigned int *day = NULL,
      unsigned int *month = NULL,
      unsigned int *year = NULL) = 0;
   
   /** get the message with number msgno
       @param msgno sequence number
       @return message handler
   */
   virtual class Message *GetMessage(unsigned long msgno) = 0;

   /** Delete a message.
       @param index the sequence number
   */
   virtual void DeleteMessage(unsigned long index) = 0;

   /** Expunge messages.
     */
   virtual void ExpungeMessages(void) = 0;
   
   /** Get the profile.
       @return Pointer to the profile.
   */
   inline Profile *GetProfile(void) { return profile; }

   /// return class name
   const char *GetClassName(void) const
      { return "MailFolder"; }
   
   CB_DECLARE_CLASS(MailFolder, CommonBase);
};

class MailFolderPOP : public MailFolder
{
public:
   virtual void SetParameters(String const &host,
		      String const &login,
		      String const &password) = 0; 
   ~MailFolderPOP() {};
};

class MailFolderIMAP : public MailFolder
{
public:
   ~MailFolderIMAP() {};
};

class MailFolderFile : public MailFolder
{
public:
   ~MailFolderFile() {};
};

class MailFolderINBOX : public MailFolder
{
public:
   ~MailFolderINBOX() {};
};
#endif
