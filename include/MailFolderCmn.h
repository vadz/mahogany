/*-*- c++ -*-********************************************************
 * MailFolderCmn class: common code for all MailFolder classes      *
 *                                                                  *
 * (C) 1998-2000 by Karsten Ballüder (karsten@phy.hw.ac.uk)      *
 *                                                                  *
 * $Id$
 *******************************************************************/


#ifndef MAILFOLDERCMN_H
#define MAILFOLDERCMN_H

#ifdef __GNUG__
#   pragma interface "MailFolderCmn.h"
#endif

#include "MailFolder.h" // the base class

#include "MEvent.h"     // for MEventOptionsChangeData

// define this for some additional checks of folder closing logic
#undef DEBUG_FOLDER_CLOSE

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
   virtual bool SaveMessages(const UIdArray *selections,
                             MFolder *folder,
                             bool updateCount = true);

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
   virtual bool SaveMessages(const UIdArray *selections,
                             String const & folderName,
                             bool isProfile,
                             bool updateCount = true);
   /** Save the messages to a folder.
       @param selections the message indices which will be converted using the current listing
       @param fileName the name of the folder to save to
       @return true on success
   */
   virtual bool SaveMessagesToFile(const UIdArray *selections,
                                   String const & fileName);

   /** Mark messages as deleted or move them to trash.
       @param messages pointer to an array holding the message numbers
       @return true on success
   */
   virtual bool DeleteOrTrashMessages(const UIdArray *messages);

   /** Mark messages as deleted.
       @param messages pointer to an array holding the message numbers
       @param expunge expunge messages after deletion
       @return true on success
   */
   virtual bool DeleteMessages(const UIdArray *messages, bool expunge=false);

   /** Mark messages as no longer deleted.
       @param messages pointer to an array holding the message numbers
       @return true on success
   */
   virtual bool UnDeleteMessages(const UIdArray *messages);

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
   virtual bool DeleteMessage(unsigned long uid);

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
   virtual bool SaveMessagesToFile(const UIdArray *messages, MWindow *parent = NULL);

   /** Save messages to a folder.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @param folder is the folder to save to, ask the user if NULL
       @return true if messages got saved
   */
   virtual bool SaveMessagesToFolder(const UIdArray *messages,
                                     MWindow *parent = NULL,
                                     MFolder *folder = NULL);

   /** Reply to selected messages.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @param profile pointer for environment
       @param flags 0, or REPLY_FOLLOWUP
   */
   virtual void ReplyMessages(const UIdArray *messages,
                              const Params& params,
                              MWindow *parent = NULL);

   /** Forward selected messages.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @param profile pointer for environment
   */
   virtual void ForwardMessages(const UIdArray *messages,
                                const Params& params,
                                MWindow *parent = NULL);

   virtual void ProcessHeaderListing(HeaderInfoList *hilp);
   //@}


   /// Count number of new messages.
   virtual unsigned long CountNewMessages(void) const
      {
         return
            CountMessages(MailFolder::MSG_STAT_RECENT|MailFolder::MSG_STAT_SEEN,
                          MailFolder::MSG_STAT_RECENT);
      }

   virtual inline void GetAuthInfo(String *login, String *password) const
      { *login = m_Login; *password = m_Password; }

   /** Toggle update behaviour flags.
       @param updateFlags the flags to set
   */
   //@{
   virtual void SetUpdateFlags(int updateFlags)
      {
         int oldFlags = m_UpdateFlags;
         m_UpdateFlags = updateFlags;
         if( !(oldFlags & UF_UpdateCount)
             && (m_UpdateFlags & UF_UpdateCount)
            )
         {
            UpdateStatus();
            RequestUpdate();// this should be done already...
         }
      }
   /// Get the current update flags
   virtual int  GetUpdateFlags(void) const
      { return m_UpdateFlags; }
   //@}
   /** Apply any filter rules to the folder.
       Applies the rule to all messages listed in msgs.
       @return -1 if no filter module exists, return code otherwise
   */
   virtual int ApplyFilterRules(UIdArray msgs);

   /** Update the folder to correspond to the new parameters: called from
       Options_Change MEvent handler.

       @param kind the change kind (ok/apply/cancel)
   */
   virtual void OnOptionsChange(MEventOptionsChangeData::ChangeKind kind);

   /// VZ: adding this decl as it doesn't compile otherwise
   virtual void Checkpoint(void) = 0;

protected:
   /** Checks for new mail and filters if necessary.
       @return FilterRule flags combination
   */
   virtual int FilterNewMail(HeaderInfoList *);

   /// Is the mailfolder still connected to a server or file?
   virtual bool IsAlive(void) const = 0;

   /**@name Config information used */
   //@{
   struct MFCmnOptions
   {
      MFCmnOptions()
      {
         memset(this, sizeof(*this), 0);
      }

      bool operator!=(const MFCmnOptions& other) const
      {
         return m_ListingSortOrder != other.m_ListingSortOrder ||
                m_ReSortOnChange != other.m_ReSortOnChange ||
                m_UpdateInterval != other.m_UpdateInterval ||
                m_UseThreading != other.m_UseThreading;
      }

      /// how to sort the list of messages
      long m_ListingSortOrder;
      /// do we want to re-sort it on a status change?
      bool m_ReSortOnChange;
      /// Timer update interval for checking folder content
      int m_UpdateInterval;
      /// do we want to thread messages?
      bool m_UseThreading;
   } m_Config;
   //@}

   /// Use the new options from m_Config
   virtual void DoUpdate();

   /// Update m_Config info from profile.
   void UpdateConfig(void);

   /// Read options from profile into the options struct
   virtual void ReadConfig(MailFolderCmn::MFCmnOptions& config);

   /// Update the folder status, number of messages, etc
   virtual void UpdateStatus(void) = 0;
   /// Constructor
   MailFolderCmn();

   /// Destructor
   ~MailFolderCmn();
   /**@name All used to build listings */
   //@{

   /// generate NewMail messages if needed
   void CheckForNewMail(HeaderInfoList *hilp);

   /** Check if this message is a "New Message" for generating new
       mail event. */
   virtual bool IsNewMessage(const HeaderInfo * hi) = 0;

   /// Call this before actually closing the folder.
   void PreClose(void);

   /// To display progress while reading message headers:
   class MProgressDialog *m_ProgressDialog;

   /// Have we not build a listing before?
   bool m_FirstListing;
   /// Number of messages in last listing:
   UIdType m_OldMessageCount;
   /// The last seen new UID, to check for new mails:
   UIdType  m_LastNewMsgUId;
   /// The current update flags
   int     m_UpdateFlags;
   //@}
   /**@name Common variables might or might not be used */
   //@{
   /// Login for password protected mail boxes.
   String m_Login;
   /// Password for password protected mail boxes.
   String m_Password;
   //@}

   /// a timer to update information
   class MailFolderTimer *m_Timer;

#ifdef DEBUG_FOLDER_CLOSE
   virtual void IncRef(void);
#endif // DEBUG_FOLDER_CLOSE

   /// decrement and delete if reached 0, return TRUE if item wasn't deleted
   virtual bool DecRef();
   virtual bool RealDecRef();

private:
   /// We react to config change events.
   class MEventReceiver *m_MEventReceiver;

   /** gcc 2.7.2.1 on FreeBSD 2.8.8-stable is reported to need this to
       link correctly: */
   MailFolderCmn(const MailFolderCmn &) { ASSERT(0); }

#ifdef DEBUG_FOLDER_CLOSE
   bool m_PreCloseCalled;
#endif // DEBUG_FOLDER_CLOSE

   friend class MfCloseEntry;
   friend class MFCmnEventReceiver;
};

#endif // MAILFOLDERCMN_H

