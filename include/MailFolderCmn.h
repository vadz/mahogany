/*-*- c++ -*-********************************************************
 * MailFolderCmn class: common code for all MailFolder classes      *
 *                                                                  *
 * (C) 1998-1999 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
 *                                                                  *
 * $Id$
 *******************************************************************/


#ifndef MAILFOLDERCMN_H
#define MAILFOLDERCMN_H

//#ifdef __GNUG__
//#   pragma interface "MailFolderCMN.h"
//#endif

#include "MailFolder.h"

/**
   MailFolderCmn  class, common code shared by all implementations of
   the MailFolder ABC.
*/
class MailFolderCmn : public MailFolder
{
public:
   /**@name Some higher level functionality implemented by the
      MailFolder class on top of the other functions.
      These functions are not used by anything else in the MailFolder
      class and can easily be removed if needed.
   */
   //@{
   /** Save the messages to a folder.
       @param selections the message indices which will be converted using the current listing
       @param folderName the name of the folder to save to
       @param isProfile if true, the folderName will be interpreted as
       a symbolic folder name, otherwise as a filename
       @param updateCount If true, the number of messages in the
       folder is updated. If false, they will be detected as new
       messages.
       @return true on success
   */
   virtual bool SaveMessages(const INTARRAY *selections,
                             String const & folderName,
                             bool isProfile,
                             bool updateCount = true);
   /** Save the messages to a folder.
       @param selections the message indices which will be converted using the current listing
       @param fileName the name of the folder to save to
       @return true on success
   */
   virtual bool SaveMessagesToFile(const INTARRAY *selections,
                                   String const & fileName);

   /** Mark messages as deleted.
       @param messages pointer to an array holding the message numbers
       @return true on success
   */
   virtual bool DeleteMessages(const INTARRAY *messages);

   /** Mark messages as no longer deleted.
       @param messages pointer to an array holding the message numbers
       @return true on success
   */
   virtual bool UnDeleteMessages(const INTARRAY *messages);

   /**@name Old-style functions, try to avoid. */
   //@{
   
   /** Set flags on a messages. Possible flag values are MSG_STAT_xxx
       @param uid mesage uid
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
       @return always true UNSUPPORTED!
   */
  virtual bool SetMessageFlag(unsigned long uid,
                              int flag,
                              bool set = true)
      {
         return SetSequenceFlag(strutil_ultoa(uid),flag,set);
      }

   /** Delete a message.
       @param uid mesage uid
       @return always true UNSUPPORTED!
   */
   virtual inline bool DeleteMessage(unsigned long uid)
   {
     SetMessageFlag(uid,MSG_STAT_DELETED);
     return true;
   }

   /** UnDelete a message.
       @param uid mesage uid
       @return always true UNSUPPORTED!
   */
   virtual inline bool UnDeleteMessage(unsigned long uid)
      { SetMessageFlag(uid,MSG_STAT_DELETED, false); return true; }

   //@}   
   /** Save messages to a file.
       @param messages pointer to an array holding the message numbers
       @parent parent window for dialog
       @return true if messages got saved
   */
   virtual bool SaveMessagesToFile(const INTARRAY *messages, MWindow *parent = NULL);

   /** Save messages to a folder.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @return true if messages got saved
   */
   virtual bool SaveMessagesToFolder(const INTARRAY *messages, MWindow *parent = NULL);

   /** Reply to selected messages.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @param profile pointer for environment
       @param flags 0, or REPLY_FOLLOWUP
   */
   virtual void ReplyMessages(const INTARRAY *messages,
                              MWindow *parent = NULL,
                              int flags = 0);

   /** Forward selected messages.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @param profile pointer for environment
   */
   virtual void ForwardMessages(const INTARRAY *messages,
                                MWindow *parent = NULL);

   //@}


   virtual inline void GetAuthInfo(String *login, String *password) const
      { *login = m_Login; *password = m_Password; }

   /** Get the profile.
       @return Pointer to the profile.
   */
   virtual inline ProfileBase *GetProfile(void) { return m_Profile; }
   /** Toggle sending of new mail events.
       @param send if true, send them
       @param update if true, update internal message count
   */
   virtual void EnableNewMailEvents(bool send = true, bool update = true)
      {
         m_GenerateNewMailEvents = send;
         m_UpdateMsgCount = update;
      }

   /** Query whether foldre is sending new mail events.
       @return if true, folder sends them
   */
   virtual bool SendsNewMailEvents(void) const
      { return m_GenerateNewMailEvents; }

   //@}
protected:
   /// Constructor
   MailFolderCmn(class ProfileBase *profile);

   /// Destructor
   ~MailFolderCmn();
   /**@name All used to build listings */
   //@{
   /** This function is called to update the folder listing. */
   void UpdateListing(void);

   /** This function must be implemented by the driver and return a
       newly built listing of all messages in the folder.
   */
   virtual HeaderInfoList *BuildListing(void) = 0;

   /// To display progress while reading message headers:
   class MProgressDialog *m_ProgressDialog;

   /// Have we not build a listing before?
   bool m_FirstListing;
   /// Number of messages in last listing:
   UIdType m_OldMessageCount;
   /** Do we want to generate new mail events?
       Used to supporess new mail events when first opening the folder
       and when copying to it. */
   bool m_GenerateNewMailEvents;
   /** Do we want to update the message count? */
   bool m_UpdateMsgCount;
   //@}
   /**@name Common variables might or might not be used */
   //@{
   /// Login for password protected mail boxes.
   String m_Login;
   /// Password for password protected mail boxes.
   String m_Password;
   /// a profile
   ProfileBase *m_Profile;
   //@}

   /**@name Config information used */
   //@{
   struct MFCmnOptions
   {
      long m_ListingSortOrder;
      bool m_ReSortOnChange;
   } m_Config;
   //@}

private:
   /// Update Config info from profile.
   void UpdateConfig(void);
   friend class MFCmnEventReceiver;
   /// We react to config change events.
   class MEventReceiver *m_MEventReceiver;
};


#endif
