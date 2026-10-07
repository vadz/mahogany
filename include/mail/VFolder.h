//////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany - cross platform e-mail GUI client
// File name:   mail/VFolder.h: declaration of MailFolderVirt class
// Purpose:     virtual folder provides MailFolder interface to the message
//              physically living in other, different folders
// Author:      Vadim Zeitlin
// Modified by:
// Created:     16.07.02
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _MAIL_VOLFDER_H_
#define _MAIL_VOLFDER_H_

#include "MailFolderCmn.h"

#ifndef USE_PCH
#  include <wx/dynarray.h>
#endif // USE_PCH

#include <set>

class MailFolderVirt : public MailFolderCmn
{
public:
   /** @name Suspending and resuming */
   //@{

   bool Suspend() override;
   bool Resume() override;

   //@}

   /** @name Accessors */
   //@{

   bool IsOpened() const override;

   bool IsReadOnly() const override;

   bool CanSetFlag(int flags) const override;

   String GetName() const override;

   MFolderType GetType() const override;

   int GetFlags() const override;

   // the pointer returned by this function should *NOT* be DecRef()'d
   Profile *GetProfile() const override;

   bool IsInCriticalSection() const override;

   ServerInfoEntry *CreateServerInfo(const MFolder *folder) const override;

   char GetFolderDelimiter() const override;

   //@}

   /** @name Functions working with message headers */
   //@{
   MsgnoType GetHeaderInfo(ArrayHeaderInfo& headers,
                           const Sequence& seq) override;

   unsigned long GetMessageCount() const override;

   unsigned long CountNewMessages() const override;

   unsigned long CountRecentMessages() const override;

   unsigned long CountUnseenMessages() const override;

   unsigned long CountDeletedMessages() const override;

   MsgnoType GetMsgnoFromUID(UIdType uid) const override;
   //@}

   /** @name Operations on the folder */
   //@{
   bool Ping() override;

   void Checkpoint() override;

   Message *GetMessage(unsigned long uid) const override;

   bool SetMessageFlag(unsigned long uid,
                       int flag,
                       bool set = true) override;
   bool SetSequenceFlag(SequenceKind kind,
                        const Sequence& sequence,
                        int flag,
                        bool set = true) override;

   bool AppendMessage(const Message& msg) override;

   bool AppendMessage(const String& msg) override;

   void ExpungeMessages() override;

   MsgnoArray *SearchByFlag(MessageStatus flag,
                            int flags = SEARCH_SET |
                                        SEARCH_UNDELETED,
                            MsgnoType last = 0) const override;

   void ListFolders(class ASMailFolder *asmf,
                    const String &pattern = _T("*"),
                    bool subscribed_only = false,
                    const String &reference = wxEmptyString,
                    UserData ud = nullptr,
                    Ticket ticket = ILLEGAL_TICKET) override;
   //@}

   /**@name Access control */
   //@{

   bool Lock() const override;

   void UnLock() const override;

   bool IsLocked() const override;

   //@}

   /** @name The driver methods */
   //@{

   /// initialize virtual folders
   static bool Init();

   /// shutdown
   static void Cleanup();

   /// open a virtual folder
   static MailFolder *OpenFolder(const MFolder *folder,
                                 const String& login,
                                 const String& password,
                                 OpenMode openmode = Normal,
                                 wxFrame *frame = nullptr);

   /// update the status of a virtual folder
   static bool CheckStatus(const MFolder *folder);

   /// delete a virtual folder
   static bool DeleteFolder(const MFolder *folder);

   /// rename a virtual folder
   static bool RenameFolder(const MFolder *folder, const String& name);

   /// clear the virtual folder
   static long ClearFolder(const MFolder *folder);

   /// return full folder spec including the login name
   static String GetFullImapSpec(const MFolder *folder, const String& login);

   //@}

protected:
   bool DoCountMessages(MailFolderStatus *status) const override;

   /// common part of SetMessageFlag and SetSequenceFlag
   virtual bool DoSetMessageFlag(SequenceKind kind,
                                 unsigned long uid,
                                 int flag,
                                 bool set = true);

   /** @name message store implementation */
   //@{

   /// struct kept for each message in the virtual folder
   struct Msg
   {
      /// the folder this message lives in
      MailFolder *mf;

      /// the UID of this message in that folder
      UIdType uidPhys;

      /// the UID of this message in this folder
      UIdType uidVirt;

      /// the flags of the message in this folder (not original one)
      int flags;

      Msg(MailFolder *mf_, UIdType uidPhys_, UIdType uidVirt_, int flags_)
         : mf(mf_), uidPhys(uidPhys_), uidVirt(uidVirt_), flags(flags_)
      {
         mf->IncRef();
      }

      ~Msg() { mf->DecRef(); }
   };

   WX_DEFINE_ARRAY(Msg *, MsgArray);

   /// the array of messages in the folder
   MsgArray m_messages;

   /// All physical folders our messages belong to.
   typedef std::set<MailFolder*> MailFoldersSet;
   MailFoldersSet m_underlyingMFs;

   /// Set of folders that should be resumed, possibly NULL.
   MailFoldersSet m_foldersToResume;

   //@}

   /** @name folder properties */
   //@{

   /// the folder object representing us in the main program
   MFolder *m_folder;

   /// the mode we're opened in (Normal or ReadOnly)
   OpenMode m_openMode;

   /// the highest UID we have assigned so far
   UIdType m_uidLast;

   //@}

   /** @name Functions to work with m_messages array

     These methods encapsulate access to m_messages, no other methods should
     access it directly.
    */
   //@{

   /// get the number of messages in this folder
   size_t GetMsgCount() const { return m_messages.GetCount(); }

   /// get the Msg corresponding to the given msgno or NULL
   Msg *GetMsgFromMsgno(MsgnoType msgno) const;

   /// get the Msg corresponding to the given UID or NULL
   Msg *GetMsgFromUID(UIdType uid) const;

   /// add a new message (takes ownership of it)
   void AddMsg(Msg *msg);

   /// the opaque type used by GetFirst/NextMsg() and DeleteMsg()
   typedef size_t MsgCookie;

   /// start iterating over all messages
   Msg *GetFirstMsg(MsgCookie& cookie) const;

   /// continue iterating over messages
   Msg *GetNextMsg(MsgCookie& cookie) const;

   /// erase the given message
   void DeleteMsg(MsgCookie& cookie);

   /// erase all messages
   void ClearMsgs();

   //@}

private:
   /// private ctor, only OpenFolder() creates us
   MailFolderVirt(const MFolder *folder, OpenMode openmode);

   /// private dtor, we're never deleted directly
   virtual ~MailFolderVirt();

   GCC_DTOR_WARN_OFF
};

#endif // _MAIL_VOLFDER_H_

