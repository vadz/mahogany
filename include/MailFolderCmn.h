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
   //@}
protected:
   /**@name Common variables might or might not be used */
   //@{
   /// Login for password protected mail boxes.
   String m_Login;
   /// Password for password protected mail boxes.
   String m_Password;
   /// a profile
   ProfileBase *m_Profile;
   //@}
};


#endif
