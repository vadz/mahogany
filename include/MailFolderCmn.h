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

#include "MailFolder.h"         // for MailFolder

#ifndef USE_PCH
#  include "Sorting.h"
#  include "Threading.h"
#endif // USE_PCH

#include "MEvent.h"     // for MEventOptionsChangeData

// define this for some additional checks of folder closing logic
#undef DEBUG_FOLDER_CLOSE

// trace mask for the new mail messages
#define TRACE_MF_NEWMAIL "mfnew"

// trace mask for the mail folder related events
#define TRACE_MF_EVENTS "mfevent"

class FilterRule;

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
                                   wxWindow *parent = NULL);

   /** Mark messages as deleted or move them to trash.
       @param messages pointer to an array holding the message numbers
       @return true on success
   */
   virtual bool DeleteOrTrashMessages(const UIdArray *messages,
                                      int flags = DELETE_ALLOW_TRASH);

   /** Mark messages as deleted.
       @param messages pointer to an array holding the message numbers
       @return true on success
   */
   virtual bool DeleteMessages(const UIdArray *messages,
                               int flags = DELETE_NO_EXPUNGE);

   /** Mark messages as no longer deleted.
       @param messages pointer to an array holding the message numbers
       @return true on success
   */
   virtual bool UnDeleteMessages(const UIdArray *messages);

   /** Mark message as deleted.
       @param uid mesage uid
       @return true if ok
   */
   virtual bool DeleteMessage(unsigned long uid);

   /** Mark message as not deleted.
       @param uid mesage uid
       @return true if ok
   */
   virtual bool UnDeleteMessage(unsigned long uid);

   /** Reply to selected messages.
       @param messages pointer to an array holding the message numbers
       @param params reply parameters
       @param parent window for dialog
   */
   virtual void ReplyMessages(const UIdArray *messages,
                              const Params& params,
                              wxWindow *parent = NULL);

   /** Forward selected messages.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @param profile pointer for environment
   */
   virtual void ForwardMessages(const UIdArray *messages,
                                const Params& params,
                                wxWindow *parent = NULL);

   virtual UIdArray *SearchMessages(const SearchCriterium *crit, int flags);
   virtual bool ThreadMessages(const ThreadParams& thrParams,
                               ThreadData *thrData);

   virtual bool SortMessages(MsgnoType *msgnos, const SortParams& sortParams);
   //@}


   virtual HeaderInfoList *GetHeaders(void) const;

   virtual bool ProcessNewMail(UIdArray& uidsNew,
                               const MFolder *folderDst = NULL);

   /**
     Process new mail in some other folder when it appeared there independently
     of our actions and we don't know which messages exactly are new.

     @param folder is the folder where the new mail is in
     @param countNew is the (guess for the) number of new messages
     @return true if ok, false on error
    */
   static bool ProcessNewMail(const MFolder *folder, MsgnoType countNew)
   {
      return DoProcessNewMail(folder, NULL, NULL, countNew, NULL);
   }

   virtual int ApplyFilterRules(const UIdArray& msgs);

   /** Update the folder to correspond to the new parameters: called from
       Options_Change MEvent handler.

       @param kind the change kind (ok/apply/cancel)
   */
   virtual void OnOptionsChange(MEventOptionsChangeData::ChangeKind kind);

   /// VZ: adding this decl as it doesn't compile otherwise
   virtual void Checkpoint(void) = 0;

   virtual wxFrame *SetInteractiveFrame(wxFrame *frame);
   virtual wxFrame *GetInteractiveFrame() const;

   virtual void SuspendUpdates() { m_suspendUpdates++; }
   virtual void ResumeUpdates() { if ( !--m_suspendUpdates ) RequestUpdate(); }
   virtual void RequestUpdate();

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
   virtual void Close(bool mayLinger = true);

   /// is updating currently suspended?
   bool IsUpdateSuspended() const { return m_suspendUpdates != 0; }

   /// really count messages
   virtual bool DoCountMessages(MailFolderStatus *status) const = 0;

   /// cache the last count messages
   void CacheLastMessages(MsgnoType count);

   /**
     Send the message status event for the data in m_statusChangeData and clear
     it afterwards. It is safe to call this method even if m_statusChangeData
     is NULL, then simply nothing is done.
   */
   void SendMsgStatusChangeEvent();

   /**
     Send the message notifying the GUI about the messages which have been
     expunged (uses m_expungeData)
   */
   void RequestUpdateAfterExpunge();

   /**
     Delete m_expungeData and reset the pointer to NULL.
    */
   void DiscardExpungeData();

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

   /// our listing or NULL if not created yet
   HeaderInfoList *m_headers;

   /// a timer to update information
   class MailFolderTimer *m_Timer;

   /** @name Mail folder events data */
   //@{

   /**
     The two arrays inside m_expungeData are used between the moment when we
     get the expunge notification and until the moment we can send the
     notification about it to the GUI.

     We need both msgnos and positions because by the time GUI code gets our
     notification, the header listing doesn't have the items corresponding to
     the expunged msgnos (they were expunged!) and so can't be asked for the
     positions of these msgnos but the GUI needs them.
    */
   ExpungeData *m_expungeData;

   /**
     The elements are added to these arrays when the messages status changes
     (e.g. from mm_flags() in MailFolderCC) and then, during the next event
     loop iteration, SendMsgStatusChangeEvent() is called to notify the GUI
     code about all the changes at once.

     Outside of this time window this pointer is always NULL.
    */
   StatusChangeData *m_statusChangeData;

   /**
      The last msgno about which the outside (GUI) code knows.

      The reason for keeping this is that if we get an expunge notification for
      a message after this one we don't even have to tell anyone about it
      because it is as if nothing happened at all -- if we hadn't previously
      notified the GUI about new mail, we don't need to notify it about its
      disappearance.
    */
   MsgnoType m_msgnoLastNotified;

   //@}

private:
   /**
     public ProcessNewMail()s helper

     @param folder where the new mail is (must be non NULL)
     @param mf mail folder for folder or NULL if it is not opened
     @param uidsNew the UIDs of new messages or NULL if unknown
     @param countNew the count of new messages if uidsNew == NULL
     @param mfNew folder containing UIDs from uidsNew
     @return true if ok, false on error
    */
   static bool DoProcessNewMail(const MFolder *folder,
                                MailFolder *mf,
                                UIdArray *uidsNew,
                                MsgnoType countNew,
                                MailFolder *mfNew);

   /// apply the filter to the messages, return false on error
   bool FilterNewMail(FilterRule *filterRule, UIdArray& uidsNew);

   /// copy/move new mail to the NewMail folder, return false on error
   bool CollectNewMail(UIdArray& uidsNew, const String& newMailFolder);

   /// report new mail in the given folder to the user
   static void ReportNewMail(const MFolder *folder,
                             const UIdArray *uidsNew,
                             MsgnoType countNew,
                             MailFolder *mf);

   /// We react to config change events.
   class MEventReceiver *m_MEventReceiver;

   /// suspend update count (updating is suspended if it is > 0)
   size_t m_suspendUpdates;

   /// the frame to which messages for this folder go by default
   wxFrame *m_frame;

   friend class MfCloseEntry;
};

#endif // MAILFOLDERCMN_H

