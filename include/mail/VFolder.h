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

#ifdef __GNUG__
   #pragma interface "VFolder.h"
#endif

#include "MailFolderCmn.h"

#ifndef USE_PCH
#  include <wx/dynarray.h>
#endif // USE_PCH

class MailFolderVirt : public MailFolderCmn
{
public:
   /** @name Accessors */
   //@{

   virtual bool IsOpened() const;

   virtual bool IsReadOnly() const;

   virtual bool CanSetFlag(int flags) const;

   virtual String GetName() const;

   virtual String GetImapSpec() const;

   virtual MFolderType GetType() const;

   virtual int GetFlags() const;

   // the pointer returned by this function should *NOT* be DecRef()'d
   virtual Profile *GetProfile() const;

   virtual bool IsInCriticalSection() const;

   virtual ServerInfoEntry *CreateServerInfo(const MFolder *folder) const;

   virtual char GetFolderDelimiter() const;

   //@}

   /** @name Functions working with message headers */
   //@{
   virtual MsgnoType GetHeaderInfo(ArrayHeaderInfo& headers,
                                   const Sequence& seq);

   virtual unsigned long GetMessageCount() const;

   virtual unsigned long CountNewMessages() const;

   virtual unsigned long CountRecentMessages() const;

   virtual unsigned long CountUnseenMessages() const;

   virtual unsigned long CountDeletedMessages() const;

   virtual MsgnoType GetMsgnoFromUID(UIdType uid) const;
   //@}

   /** @name Operations on the folder */
   //@{
   virtual bool Ping();

   virtual void Checkpoint();

   virtual Message *GetMessage(unsigned long uid) const;

   virtual bool SetMessageFlag(unsigned long uid,
                               int flag,
                               bool set = true);
   virtual bool SetSequenceFlag(SequenceKind kind,
                                const Sequence& sequence,
                                int flag,
                                bool set = true);

   virtual bool AppendMessage(const Message& msg);

   virtual bool AppendMessage(const String& msg);

   virtual void ExpungeMessages();

   virtual MsgnoArray *SearchByFlag(MessageStatus flag,
                                    int flags = SEARCH_SET |
                                                SEARCH_UNDELETED,
                                    MsgnoType last = 0) const;

   virtual void ListFolders(class ASMailFolder *asmf,
                            const String &pattern = "*",
                            bool subscribed_only = false,
                            const String &reference = "",
                            UserData ud = 0,
                            Ticket ticket = ILLEGAL_TICKET);
   //@}

   /**@name Access control */
   //@{

   virtual bool Lock() const;

   virtual void UnLock() const;

   virtual bool IsLocked() const;

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
                                 wxFrame *frame = NULL);

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
   virtual bool DoCountMessages(MailFolderStatus *status) const;

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
   Msg *MailFolderVirt::GetMsgFromUID(UIdType uid) const;

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

