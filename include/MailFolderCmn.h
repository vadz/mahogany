//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MailFolderCmn.h: provide some meat on MailFolder bones
// Purpose:     MailFolderCmn is a normal base class implementing the methods
//              of MailFolder ABC for which we can write a reasonable generic
//              implementation
// Author:      Karsten Ballüder, Vadim Zeitlin
// Modified by:
// Created:     1998
// CVS-ID:      $Id$
// Copyright:   (C) 1998-2000 by Karsten Ballüder (ballueder@gmx.net)
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef MAILFOLDERCMN_H
#define MAILFOLDERCMN_H

#ifdef __GNUG__
#   pragma interface "MailFolderCmn.h"
#endif

#include "MailFolder.h" // the base class

#ifndef USE_PCH
#  include "MEvent.h"     // for MEventOptionsChangeData

#  include "Sorting.h"
#  include "Threading.h"
#endif // USE_PCH

// define this for some additional checks of folder closing logic
#undef DEBUG_FOLDER_CLOSE

/**
   MailFolderCmn  class, common code shared by all implementations of
   the MailFolder ABC.
*/
class MailFolderCmn : public MailFolder
{
public:
   /// do status caching and call DoCountMessages() to do the real work
   virtual bool CountAllMessages(MailFolderStatus *status) const;

   /**@name Some higher level functionality implemented by the
      MailFolderCmn class on top of the other functions.
   */
   //@{
   /** Save messages to a folder identified by MFolder

       @param selections pointer to an array holding the message UIDs
       @param folder is the folder to save to, can't be NULL
       @return true if messages got saved
   */
   virtual bool SaveMessages(const UIdArray *selections, MFolder *folder);

   /** Save the messages to a folder.
       @param selections the message indices which will be converted using the current listing
       @param folderName the name of the folder to save to
       @return true on success
   */
   virtual bool SaveMessages(const UIdArray *selections,
                             const String& folderName);

   /** Save the messages to a folder.
       @param selections the message indices which will be converted using the current listing
       @param fileName the name of the folder to save to
       @return true on success
   */
   virtual bool SaveMessagesToFile(const UIdArray *selections,
                                   const String& fileName,
                                   MWindow *parent = NULL);

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
   /** Reply to selected messages.
       @param messages pointer to an array holding the message numbers
       @param params reply parameters
       @param parent window for dialog
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

   virtual bool ThreadMessages(const ThreadParams& thrParams,
                               ThreadData *thrData);

   virtual bool SortMessages(MsgnoType *msgnos, const SortParams& sortParams);
   //@}


   virtual HeaderInfoList *GetHeaders(void) const;

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

   virtual MFrame *SetInteractiveFrame(MFrame *frame);
   virtual MFrame *GetInteractiveFrame() const;

   virtual void SuspendUpdates() { m_suspendUpdates++; }
   virtual void ResumeUpdates() { if ( !--m_suspendUpdates ) RequestUpdate(); }

   /** @name Delayed folder closing

       We may want to keep alive the folder for a while instead of closing it
       immediately, so we artificially bump up its reference count and
       decrement it back later when the keep alive timeout expires
    */
   //@{
   /// decrement and delete if reached 0, return TRUE if item wasn't deleted
   virtual void IncRef();
   virtual bool DecRef();
private:
   virtual bool RealDecRef();
   //@}

protected:
   /// remove the folder from our "closer" list
   virtual void Close(void);

   /// is updating currently suspended?
   bool IsUpdateSuspended() const { return m_suspendUpdates != 0; }

   /// apply filters to all new mail messages
   virtual bool FilterNewMail();

   /// move new mail to the incoming folder if necessary
   virtual bool CollectNewMail();

   /// really count messages
   virtual bool DoCountMessages(MailFolderStatus *status) const = 0;

   /** @name Config management */
   //@{
   struct MFCmnOptions
   {
      MFCmnOptions();

      bool operator!=(const MFCmnOptions& other) const;

      bool operator==(const MFCmnOptions& other) const
      {
         return !(*this != other);
      }

      /// how to sort the list of messages
      SortParams m_SortParams;

      /// how do we thread the messages
      ThreadParams m_ThrParams;

      /// do we want to resort messages on a status change? [NOT IMPL'D]
      bool m_ReSortOnChange;

      /// Timer update interval for checking folder content
      int m_UpdateInterval;
   } m_Config;

   /// Use the new options from m_Config
   virtual void DoUpdate();

   /// Update m_Config info from profile.
   void UpdateConfig(void);

   /// Read options from profile into the options struct
   virtual void ReadConfig(MailFolderCmn::MFCmnOptions& config);
   //@}

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
   //@}

   /** @name The listing information */
   //@{
   /// our listing or NULL if not created yet
   HeaderInfoList *m_headers;

   /// The last seen new UID, to check for new mails:
   UIdType m_LastNewMsgUId;
   //@}

   /// a timer to update information
   class MailFolderTimer *m_Timer;

private:
   /// We react to config change events.
   class MEventReceiver *m_MEventReceiver;

   /// suspend update count (updating is suspended if it is > 0)
   size_t m_suspendUpdates;

   /// the frame to which messages for this folder go by default
   MFrame *m_frame;

   friend class MfCloseEntry;
   friend class MFCmnEventReceiver;
};

#endif // MAILFOLDERCMN_H

