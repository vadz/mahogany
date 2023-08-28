///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/wxMsgCmdProc.cpp: implementation of MsgCmdProc
// Author:      Vadim Zeitlin
// Modified by:
// Created:     01.08.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin
// Licence:     Mahogany license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include  "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
   #include "strutil.h"          // for strutil_uniq_array()
   #include "Mdefaults.h"
   #include "guidef.h"
   #include "MApplication.h"
   #include "gui/wxIconManager.h"

   #include <wx/dynarray.h>
#endif // USE_PCH

#include "Collect.h"          // for InteractivelyCollectAddresses()

#include "UIdArray.h"
#include "gui/wxMenuDefs.h"

#include "HeaderInfo.h"
#include "Sequence.h"

#include "TemplateDialog.h"
#include "MessageView.h"
#include "Composer.h"
#include "SendMessage.h"
#include "MFolder.h"
#include "ASMailFolder.h"

#include "MsgCmdProc.h"

#include "SpamFilter.h"

#include "MIMETreeDialog.h"
#include "MFui.h"

#include "gui/wxDialogLayout.h"

#if wxUSE_DRAG_AND_DROP
   #include "Mdnd.h"
#endif // wxUSE_DRAG_AND_DROP

#include "modules/Filters.h"    // for FilterRule::Error

class AsyncStatusHandler;

WX_DEFINE_ARRAY(AsyncStatusHandler *, ArrayAsyncStatus);

// the trace mask for dnd messages
#define M_TRACE_DND _T("msgdnd")

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_AUTOCOLLECT_ADB;
extern const MOption MP_REPLY_QUOTE_ORIG;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_CONFIRM_RESEND;
extern const MPersMsgBox *M_MSGBOX_CONFIRM_ZAP;
extern const MPersMsgBox *M_MSGBOX_ZAP_SPAM;

// ----------------------------------------------------------------------------
// MsgCmdProcImpl
// ----------------------------------------------------------------------------

class MsgCmdProcImpl : public MsgCmdProc
{
public:
   /// default ctor
   MsgCmdProcImpl(MessageView *msgView, wxWindow *winForDnd);

   /// dtor
   virtual ~MsgCmdProcImpl();

   /**
     Set the folder to use - this should be called before ProcessCommand()
    */
   virtual void SetFolder(ASMailFolder *asmf);

   /**
     Set the frame to use for status messages and (as parent for) dialogs
    */
   virtual void SetFrame(wxFrame *frame) { m_frame = frame; }

   virtual void SetWindowForDnD(wxWindow *win) { m_winForDnd = win; }

   /**
     Process the given WXMENU_MSG_XXX command for all message

     @return true if the command was processed
    */
   virtual bool ProcessCommand(int id,
                               const UIdArray& messages,
                               MFolder *folder);

   virtual String GetFolderName() const
   {
      String name;
      if ( m_asmf )
         name = m_asmf->GetName();

      return name;
   }

   /** @name Event stuff

       None of these functions should be called by the outsiders, they are
       only for AsyncStatusHandler
    */
   //@{

   /// needed for async event processing
   virtual bool OnMEvent(MEventData& event);

   /// add an async status object to m_arrayAsyncStatus
   void AddAsyncStatus(AsyncStatusHandler *asyncStatus);

   /// remove the given async status object from m_arrayAsyncStatus
   void RemoveAsyncStatus(AsyncStatusHandler *asyncStatus);

   /// store the ticket for an async operation which was just launched
   void AddASyncOperation(Ticket ticket) { m_TicketList->Add(ticket); }

   //@}

   /// do not call DecRef() on the returned pointer (which is never NULL)
   Profile *GetProfile() const { return m_msgView->GetProfile(); }

   wxFrame *GetFrame() const { return m_frame; }

   MailFolder *GetMailFolder() const
      { return m_asmf ? m_asmf->GetMailFolder() : NULL; }

protected:
   /** @name Menu command handlers
    */
   //@{

   void OpenMessages(const UIdArray& selections);
   void EditMessages(const UIdArray& selections);

   void BounceMessages(const UIdArray& selections);
   void ResendMessages(const UIdArray& selections);

   void PrintOrPreviewMessages(const UIdArray& selections, bool preview);
   void PrintMessages(const UIdArray& selections);
   void PrintPreviewMessages(const UIdArray& selections);

   void DeleteOrTrashMessages(const UIdArray& selections);
   bool DeleteAndExpungeMessages(const UIdArray& selections);
   void UndeleteMessages(const UIdArray& selections);
   void ToggleMessagesFlag(const UIdArray& messages, MailFolder::MessageStatus f);
   void MarkRead(const UIdArray& messages, bool read);

   Ticket SaveMessagesToFolder(const UIdArray& selections, MFolder *folder = NULL);
   Ticket MoveMessagesToFolder(const UIdArray& messages, MFolder *folder = NULL);
   Ticket SaveMessagesToFile(const UIdArray& selections);

   void ExtractAddresses(const UIdArray& selections);

   void ApplyFilters(const UIdArray& selections);

   bool DragAndDropMessages(const UIdArray& selections);
   void DropMessagesToFolder(const UIdArray& selections, MFolder *folder = NULL);

   /// Show the raw text of the specified message
   void ShowRawText(UIdType uid);

   /// Show the MIME structure of the specified message
   void ShowMIMEDialog(UIdType uid);

#ifdef EXPERIMENTAL_show_uid
   /// Show the string uniquely identifying the message
   void ShowUIDL(UIdType uid);
#endif // EXPERIMENTAL_show_uid

   /// Reclassify the message as [non] spam
   void ReclassifyAsSpam(const UIdArray& uids, bool isSpam);

   /// Check whether the message is spam
   void CheckIfSpam(const UIdArray& uids);

   /// Remove attachments from the message
   void RemoveAttachments(UIdType uid);

   //@}

   /// access the m_TicketsToDeleteList creating it if necessary
   void AddTicketToDelete(Ticket t);

   /// get the Message with the given UID (caller must DecRef() it)
   Message *GetMessage(UIdType uid) const
   {
      // do it synchronously (FIXME should we?)
      MailFolder_obj mf(GetMailFolder());
      CHECK( mf, NULL, _T("no folder in MsgCmdProcImpl::GetMessage()") );

      return mf->GetMessage(uid);
   }

private:
   /// our folder (may be NULL)
   ASMailFolder *m_asmf;

   /// the array containing the progress objects for all async operations
   ArrayAsyncStatus m_arrayAsyncStatus;

   /// tickets for the async operations in progress
   ASTicketList *m_TicketList;

   /**
     drag and dropping messages is complicated because all operations
     (message saving, deletion *and* DoDragDrop() call) are async, so
     everything may happen in any order, yet we should never delete the
     messages which couldn't be copied successfully. To deal with this we
     maintain the following lists:

      * m_TicketsToDeleteList which contains the tickets of all messages we
        should delete once we receive the confirmation that they had been
        copied successfully to the target folder
      * m_TicketsDroppedList which contains all tickets created by
        DropMessagesToFolder()
      * m_UIdsCopiedOk which contains messages from tickets of
        m_TicketsDroppedList which have been already saved successfully.
   */

   /// a list of tickets we should delete if copy operation succeeded
   ASTicketList *m_TicketsToDeleteList;

   /// a list of tickets which we _might_ have to delete
   ASTicketList *m_TicketsDroppedList;

   /// a list of UIDs we might have to delete
   UIdArray m_UIdsCopiedOk;

   /// the list of tickets we want to open the compose windows for
   ASTicketList *m_TicketsToEditList;

   /// MEventManager reg info
   void *m_regASyncResult;

   /// the GUI window to use for status messages
   wxFrame *m_frame;

   /// the message view we process the messages for
   MessageView *m_msgView;

   /// the source window for message dragging
   wxWindow *m_winForDnd;
};

// ----------------------------------------------------------------------------
// AsyncStatusHandler: this object is constructed before launching an async
// operation to show some initial message
//
// Then it must be given the ticket to monitor - if the ticket is invalid, it
// deletes itself (with an error message) and the story stops here, otherwise
// it waits untiul OnASFolderResultEvent() deletes it and gives the final
// message indicating either success (default) or failure (if Fail() was
// called)
// ----------------------------------------------------------------------------

class AsyncStatusHandler
{
public:
   // create the handler for a new async operation and show the given message
   // to the user
   AsyncStatusHandler(MsgCmdProcImpl *msgCmdProc, const wxString& msgInitial);

   // monitor the given ticket, give error message if the corresponding
   // operation terminates with an error
   bool Monitor(Ticket ticket, const wxString& msgError);

   // used by OnASFolderResultEvent() to find the matching progress indicator
   Ticket GetTicket() const { return m_ticket; }

   // use to indicate that our operation finally failed
   void Fail() { m_ticket = ILLEGAL_TICKET; }

   // use different message on success (default is initial message + done)
   void SetSuccessMsg(const wxString& msgOk);

   // give the appropriate message
   ~AsyncStatusHandler();

private:
   // get the frame to use for status messages
   wxFrame *GetFrame() const { return m_msgCmdProc->GetFrame(); }

   // the folder view which initiated the async operation
   MsgCmdProcImpl *m_msgCmdProc;

   // the ticket for our operation
   Ticket m_ticket;

   // the initial message and the final message in case of success and failure
   wxString m_msgInitial,
            m_msgOk,
            m_msgError;
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// AsyncStatusHandler
// ----------------------------------------------------------------------------

AsyncStatusHandler::AsyncStatusHandler(MsgCmdProcImpl *msgCmdProc,
                                       const wxString& msgInitial)
                  : m_msgInitial(msgInitial)
{
   m_msgCmdProc = msgCmdProc;
   m_ticket = ILLEGAL_TICKET;

   m_msgCmdProc->AddAsyncStatus(this);

   wxLogStatus(GetFrame(), m_msgInitial);
   MBeginBusyCursor();
}

bool AsyncStatusHandler::Monitor(Ticket ticket, const wxString& msgError)
{
   m_msgError = msgError;

   m_ticket = ticket;

   if ( ticket == ILLEGAL_TICKET )
   {
      // self delete as MsgCmdProc will never do it as it will never get
      // the notifications for the invalid ticket
      m_msgCmdProc->RemoveAsyncStatus(this);

      delete this;

      return FALSE;
   }

   m_msgCmdProc->AddASyncOperation(ticket);

   return TRUE;
}

void AsyncStatusHandler::SetSuccessMsg(const wxString& msgOk)
{
   m_msgOk = msgOk;
}

AsyncStatusHandler::~AsyncStatusHandler()
{
   if ( m_ticket == ILLEGAL_TICKET )
   {
      // also put it into the status bar to overwrite the previous message
      // there
      wxLogStatus(GetFrame(), m_msgError);
   }
   else // success
   {
      if ( m_msgOk.empty() )
      {
         m_msgOk << m_msgInitial << _(" done.");
      }
      //else: set explicitly, use it as is

      wxLogStatus(GetFrame(), m_msgOk);
   }

   MEndBusyCursor();
}

// ----------------------------------------------------------------------------
// MsgCmdProc ctor/dtor
// ----------------------------------------------------------------------------

/* static */
MsgCmdProc *MsgCmdProc::Create(MessageView *msgView, wxWindow *winForDnd)
{
   CHECK( msgView, NULL, _T("must have an associated message view") );

   return new MsgCmdProcImpl(msgView, winForDnd);
}

MsgCmdProcImpl::MsgCmdProcImpl(MessageView *msgView, wxWindow *winForDnd)
{
   ASSERT_MSG( msgView, _T("NULL MessageView in MsgCmdProc -- will crash") );

   m_msgView = msgView;
   m_winForDnd = winForDnd;
   m_asmf = NULL;
   m_regASyncResult = MEventManager::Register(*this, MEventId_ASFolderResult);

   m_TicketList = ASTicketList::Create();

   // these are created on demand
   m_TicketsToDeleteList = NULL;
   m_TicketsDroppedList = NULL;
   m_TicketsToEditList = NULL;
}

MsgCmdProcImpl::~MsgCmdProcImpl()
{
   MEventManager::Deregister(m_regASyncResult);

   if ( m_TicketList )
      m_TicketList->DecRef();

   if ( m_TicketsToDeleteList )
      m_TicketsToDeleteList->DecRef();

   if ( m_TicketsDroppedList )
      m_TicketsDroppedList->DecRef();

   if ( m_TicketsToEditList )
      m_TicketsToEditList->DecRef();

   if ( m_asmf )
      m_asmf->DecRef();
}

// ----------------------------------------------------------------------------
// MsgCmdProcImpl misc operations
// ----------------------------------------------------------------------------

void MsgCmdProcImpl::SetFolder(ASMailFolder *asmf)
{
   if ( m_asmf )
      m_asmf->DecRef();

   m_asmf = asmf;

   if ( m_asmf )
      m_asmf->IncRef();
}

void MsgCmdProcImpl::AddTicketToDelete(Ticket t)
{
   if ( !m_TicketsToDeleteList )
      m_TicketsToDeleteList = ASTicketList::Create();

   m_TicketsToDeleteList->Add(t);
}

// ----------------------------------------------------------------------------
// MsgCmdProcImpl command dispatcher
// ----------------------------------------------------------------------------

bool MsgCmdProcImpl::ProcessCommand(int cmd,
                                    const UIdArray& messages,
                                    MFolder *folder)
{
   if ( !m_asmf )
      return false;

   CHECK( !messages.IsEmpty(), false, _T("no messages to operate on") );

   bool rc = true;

   // first process commands which can be reduced to other commands
   MessageTemplateKind templKind;
   switch ( cmd )
   {
      case WXMENU_MSG_REPLY_WITH_TEMPLATE:
         cmd = WXMENU_MSG_REPLY;
         templKind = MessageTemplate_Reply;
         break;

      case WXMENU_MSG_REPLY_SENDER_WITH_TEMPLATE:
         cmd = WXMENU_MSG_REPLY_SENDER;
         templKind = MessageTemplate_Reply;
         break;

      case WXMENU_MSG_REPLY_LIST_WITH_TEMPLATE:
         cmd = WXMENU_MSG_REPLY_LIST;
         templKind = MessageTemplate_Reply;
         break;

      case WXMENU_MSG_FORWARD_WITH_TEMPLATE:
         cmd = WXMENU_MSG_FORWARD;
         templKind = MessageTemplate_Forward;
         break;

      case WXMENU_MSG_REPLY_ALL_WITH_TEMPLATE:
         cmd = WXMENU_MSG_REPLY_ALL;
         templKind = MessageTemplate_Followup;
         break;

      default:
         templKind = MessageTemplate_None;
   }


   String templ;
   if ( templKind != MessageTemplate_None )
   {
      templ = ChooseTemplateFor(templKind, GetFrame());
      if ( !templ )
      {
         // cancelled by user
         return true;
      }
   }

   // dispatch all supported commands
   switch ( cmd )
   {
      case WXMENU_MSG_OPEN:
         OpenMessages(messages);
         break;

      case WXMENU_MSG_EDIT:
         EditMessages(messages);
         break;


      case WXMENU_MSG_SAVE_TO_FOLDER:
         if ( SaveMessagesToFolder(messages, folder) == ILLEGAL_TICKET )
         {
            // cancelled by user
            rc = false;
         }
         break;

      case WXMENU_MSG_SAVE_TO_FILE:
         if ( SaveMessagesToFile(messages) == ILLEGAL_TICKET )
         {
            // cancelled by user
            rc = false;
         }
         break;

      case WXMENU_MSG_MOVE_TO_FOLDER:
         if ( MoveMessagesToFolder(messages, folder) == ILLEGAL_TICKET )
         {
            // cancelled by user
            rc = false;
         }
         break;

      case WXMENU_MSG_DROP_TO_FOLDER :
         DropMessagesToFolder(messages, folder);
         break;

      case WXMENU_MSG_DRAG:
         DragAndDropMessages(messages);
         break;


      case WXMENU_MSG_REPLY:
      case WXMENU_MSG_REPLY_SENDER:
      case WXMENU_MSG_REPLY_ALL:
      case WXMENU_MSG_REPLY_LIST:
      case WXMENU_MSG_FOLLOWUP_TO_NEWSGROUP:
         {
            int quote = READ_CONFIG(GetProfile(), MP_REPLY_QUOTE_ORIG);

            if ( quote == M_ACTION_PROMPT )
            {
               switch ( MDialog_YesNoCancel
                        (
                           _("Do you want to include the original message "
                             "text in your reply?"),
                           GetFrame()
                        ) )
               {
                  default:
                     FAIL_MSG( _T("unexpected MDialog_YesNoCancel return") );
                     // fall through

                  case MDlg_Cancel:
                     // don't reply to the message at all
                     return true;

                  case MDlg_No:
                     quote = M_ACTION_NEVER;
                     break;

                  case MDlg_Yes:
                     quote = M_ACTION_ALWAYS;
                     break;
               }
            }

#define CASE_REPLY(kind) \
   case WXMENU_MSG_##kind: \
      replyKind = MailFolder::kind; \
      break

            MailFolder::ReplyKind replyKind;
            switch ( cmd )
            {
               default:
                  FAIL_MSG( _T("unknown reply menu command") );
                  wxFALLTHROUGH;

               CASE_REPLY(REPLY);
               CASE_REPLY(REPLY_SENDER);
               CASE_REPLY(REPLY_ALL);
               CASE_REPLY(REPLY_LIST);
               CASE_REPLY(FOLLOWUP_TO_NEWSGROUP);
            }

#undef CASE_REPLY

            MailFolder::Params params(templ, replyKind);
            if ( quote != M_ACTION_NEVER )
            {
               params.msgview = m_msgView;
            }

            m_TicketList->Add(m_asmf->ReplyMessages
                                      (
                                       &messages,
                                       params,
                                       GetFrame(),
                                       this
                                      )
                             );
         }
         break;

      case WXMENU_MSG_FORWARD:
         {
            MailFolder::Params params(templ);
            params.msgview = m_msgView;

            m_TicketList->Add(m_asmf->ForwardMessages
                                      (
                                       &messages,
                                       params,
                                       GetFrame(),
                                       this
                                      )
                             );
         }
         break;

      case WXMENU_MSG_BOUNCE:
         BounceMessages(messages);
         break;

      case WXMENU_MSG_RESEND:
         ResendMessages(messages);
         break;


      case WXMENU_MSG_UNDELETE:
         UndeleteMessages(messages);
         break;

      case WXMENU_MSG_DELETE:
         DeleteOrTrashMessages(messages);
         break;

      case WXMENU_MSG_DELETE_EXPUNGE:
         rc = DeleteAndExpungeMessages(messages);
         break;

      case WXMENU_MSG_FLAG:
      case WXMENU_MSG_MARK_ANSWERED:
         ToggleMessagesFlag(messages, cmd == WXMENU_MSG_FLAG
                                       ? MailFolder::MSG_STAT_FLAGGED
                                       : MailFolder::MSG_STAT_ANSWERED);
         break;

      case WXMENU_MSG_MARK_READ:
      case WXMENU_MSG_MARK_UNREAD:
         MarkRead(messages, cmd == WXMENU_MSG_MARK_READ);
         break;


      case WXMENU_MSG_PRINT:
         PrintMessages(messages);
         break;

#ifdef USE_PS_PRINTING
      case WXMENU_MSG_PRINT_PS:
         //FIXME PrintMessages(messages);
         break;
#endif // USE_PS_PRINTING

      case WXMENU_MSG_PRINT_PREVIEW:
         PrintPreviewMessages(messages);
         break;


      case WXMENU_MSG_SAVEADDRESSES:
         ExtractAddresses(messages);
         break;

      case WXMENU_MSG_FILTER:
         ApplyFilters(messages);
         break;

      case WXMENU_VIEW_SHOWRAWTEXT:
         ShowRawText(messages[0]);
         break;

#ifdef EXPERIMENTAL_show_uid
      case WXMENU_VIEW_SHOWUID:
         ShowUIDL(messages[0]);
         break;
#endif // EXPERIMENTAL_show_uid

      case WXMENU_VIEW_SHOWMIME:
         ShowMIMEDialog(messages[0]);
         break;

      case WXMENU_MSG_SPAM_MARK:
      case WXMENU_MSG_SPAM_UNMARK:
         ReclassifyAsSpam(messages, cmd == WXMENU_MSG_SPAM_MARK);
         break;

      case WXMENU_MSG_SPAM_CHECK:
         CheckIfSpam(messages);
         break;

      case WXMENU_MSG_REMOVE_ATTACHMENTS:
         RemoveAttachments(messages[0]);
         break;

      default:
         // try passing it to message view
         if ( !m_msgView->DoMenuCommand(cmd) )
         {
            // not our command
            return false;
         }
   }

   return rc;
}

// ----------------------------------------------------------------------------
// operations on messages
// ----------------------------------------------------------------------------

#ifdef EXPERIMENTAL_show_uid

void
MsgCmdProcImpl::ShowUIDL(UIdType uid)
{
   MailFolder_obj mf(GetMailFolder());
   CHECK_RET( mf, _T("no folder in MsgCmdProcImpl::ShowUIDL") );

   String uidString = mf->GetMessageUID(GetFocus());
   if ( uidString.empty() )
      wxLogWarning("This message doesn't have a valid UID.");
   else
      wxLogMessage("The UID of this message is '%s'.", uidString);
}

#endif // EXPERIMENTAL_show_uid

void
MsgCmdProcImpl::ShowRawText(UIdType uid)
{
   String text;

   Message_obj msg(GetMessage(uid));
   if ( msg )
   {
      msg->WriteToString(text, true);
   }

   if ( text.empty() )
   {
      wxLogError(_("Failed to get the raw text of the message."));
   }
   else
   {
      MDialog_ShowText(GetFrame(), _("Raw message text"), text, "RawMsgPreview");
   }
}

void
MsgCmdProcImpl::ShowMIMEDialog(UIdType uid)
{
   // do it synchronously (FIXME should we?)
   MailFolder_obj mf(GetMailFolder());
   CHECK_RET( mf, _T("no folder in MsgCmdProcImpl::ShowMIMEDialog") );

   Message_obj msg(mf->GetMessage(uid));
   const MimePart *part = msg ? msg->GetTopMimePart() : NULL;

   if ( !part )
   {
      wxLogError(_("Failed to retrieve the MIME structure of the message."));

      return;
   }

   // only pass message view to the dialog to allow showing the context menu
   // for the MIME parts - but for this we must be previewing this message!
   ShowMIMETreeDialog(part, GetFrame(),
                           m_msgView->GetUId() == uid ? m_msgView : NULL);
}

void MsgCmdProcImpl::RemoveAttachments(UIdType uid)
{
   // check that we can modify messages in this folder
   Message_obj msg(GetMessage(uid));
   if ( !msg )
   {
      wxLogError(_("Failed to access the message"));
      return;
   }

   // this object shouldn't be DecRef()'d
   MailFolder * const mf = msg->GetFolder();
   CHECK_RET( mf, _T("message without folder?") );

   // before doing anything else, ensure that we can write to the folder
   if ( !CanCreateMessagesInFolder(mf->GetType()) )
   {
      wxLogError(_("Messages in the folder \"%s\" can't be modified, "
                   "please copy the message to a local or IMAP folder "
                   "before removing attachments from it."),
                 mf->GetName());
      return;
   }

   if ( mf->IsReadOnly() )
   {
      wxLogError(_("Folder \"%s\" is read-only, please reopen it in "
                   "read-write mode."), mf->GetName());
      return;
   }


   // ask the user which attachments should be removed
   wxArrayString partsDescs;
   wxArrayInt partsRemove;
   const int count = msg->CountParts();
   int i;
   for ( i = 0; i < count; i++ )
   {
      const MimePart * const mimepart = msg->GetMimePart(i);
      MimeType mimetype = mimepart->GetType();

      String desc = mimepart->GetDescription();
      if ( desc.empty() )
         desc = mimepart->GetParam("name");
      if ( desc.empty() )
         desc = mimepart->GetFilename();
      if ( desc.empty() )
         desc.Printf(_("Unnamed %s #%d"), mimetype.GetFull(), i);

      desc << " ("
           << SizeInBytesToString(mimepart->GetSize(), SizeToString_Medium)
           << ")";
      partsDescs.push_back(desc);

      // remove all attachments by default
      if ( mimepart->IsAttachment() || mimetype.GetPrimary() != MimeType::TEXT )
         partsRemove.push_back(i);
   }

   if ( partsDescs.empty() )
   {
      wxLogWarning(_("There are no attachments to remove in this message."));
      return;
   }

   size_t numRemove = MDialog_GetSelections
                      (
                        _("Please check the attachments to be deleted.\n"
                          "\n"
                          "This operation cannot be undone, the selected "
                          "attachments will be permanently deleted,\n"
                          "please make sure to save them first if you need them!"),
                        String(M_TITLE_PREFIX) + _("Remove Attachments"),
                        partsDescs,
                        &partsRemove,
                        GetFrame(),
                        "RemoveAttachments"
                      );
   if ( numRemove == 0 )
      return;  // cancelled by the user

   // create a copy of the message without the attachments
   SendMessage_obj msgCopy(SendMessage::CreateFromMsg
                           (
                               GetProfile(),
                               msg.Get(),
                               Prot_SMTP,
                               GetFrame(),
                               &partsRemove
                           ));

   // and save it to the same folder: note that we don't use MailFolder::
   // AppendMessage(Message) overload as this would modify the date header and
   // we want to preserve it
   String msgstr;
   if ( !msgCopy->WriteToString(msgstr) || !mf->AppendMessage(msgstr) )
      return;

   // copy the flags of the original message
   {
      bool flagsOk = false;

      HeaderInfoList_obj headers(HeaderInfoList::Create(mf));
      const HeaderInfo * const hi = headers ? headers->GetEntryUId(uid) : NULL;
      if ( hi )
      {
         const int flags = hi->GetStatus();

         // the message we've just appended must be the last one
         Sequence seq;
         seq.Add(headers->Count());
         flagsOk = mf->SetSequenceFlag(MailFolder::SEQ_MSGNO, seq, flags);
      }

      if ( !flagsOk )
      {
         wxLogWarning(_("Failed to preserve the message flags"));
      }
   }

   // finally delete the old one
   UIdArray uids;
   uids.push_back(uid);
   if ( !mf->DeleteMessages(&uids, MailFolder::DELETE_EXPUNGE) )
   {
      wxLogWarning(_("Failed to delete the original message after creating "
                     "the message copy without attachments."));
   }
}

void MsgCmdProcImpl::ReclassifyAsSpam(const UIdArray& uids, bool isSpam)
{
   wxString msg = isSpam ? _("Reclassifiying as spam...")
                         : _("Reclassifiying as ham...");
   wxLogStatus(GetFrame(), msg);

   // first mark the messages as spam/ham
   const size_t count = uids.Count();
   UIdArray uidsReclassified;
   uidsReclassified.reserve(count);
   for ( size_t i = 0; i < count; i++ )
   {
      Message_obj message(GetMessage(uids[i]));
      if ( !message)
      {
         wxLogError(_("Failed to retrieve the message %#08x to reclassify."),
                    uids[i]);
         continue;
      }

      if ( SpamFilter::Reclassify(*message, isSpam) )
         uidsReclassified.push_back(uids[i]);
   }

   wxLogStatus(GetFrame(), msg + _("done"));

   // second, permanently delete all the spam
   if ( isSpam && !uidsReclassified.empty() && MDialog_YesNoDialog
        (
            String::Format
            (
               _("Do you want to permanently delete the %lu messages "
                 "marked as spam now?"),
               (unsigned long)uidsReclassified.size()
            ),
            GetFrame(),
            MDIALOG_YESNOTITLE,
            M_DLG_NO_DEFAULT,
            M_MSGBOX_ZAP_SPAM
        ) )
   {
      // TODO: make this action configurable (i.e. could be moved to another
      //       folder and it should also be possible to define an action for
      //       ham messages, e.g. move them back to inbox)
      DeleteAndExpungeMessages(uidsReclassified);
   }
}

void MsgCmdProcImpl::CheckIfSpam(const UIdArray& uids)
{
   wxString status(_("Checking message using spam filters..."));
   wxLogStatus(GetFrame(), status);

   const size_t n = uids.Count();
   for ( size_t i = 0; i < n; i++ )
   {
      Message_obj msg(GetMessage(uids[i]));
      if ( !msg )
      {
         wxLogError(_("Failed to retrieve the message to check in spam filter."));
         continue;
      }

      String str(_("The message with subject \"%s\" from \"%s\"\n"));

      String result;
      if ( SpamFilter::CheckIfSpam(*msg, wxEmptyString, &result) )
      {
         wxLogWarning(str + _("seems to be a spam (%s)."),
                      msg->Subject(),
                      msg->From(),
                      result);
      }
      else // !spam
      {
         wxLogMessage(str + _("doesn't seem to be a spam."),
                      msg->Subject(),
                      msg->From());
      }
   }

   wxLogStatus(GetFrame(), status + _("done"));
}

void
MsgCmdProcImpl::OpenMessages(const UIdArray& selections)
{
   size_t n = selections.Count();
   for ( size_t i = 0; i < n; i++ )
   {
      ShowMessageViewFrame(GetFrame(), m_asmf, selections[i]);
   }
}

void
MsgCmdProcImpl::EditMessages(const UIdArray& selections)
{
   if ( !m_TicketsToEditList )
      m_TicketsToEditList = ASTicketList::Create();

   size_t n = selections.Count();
   for ( size_t i = 0; i < n; i++ )
   {
      Ticket t = m_asmf->GetMessage(selections[i], this);

      m_TicketsToEditList->Add(t);

      AsyncStatusHandler *status =
         new AsyncStatusHandler(this, _("Retrieving messages..."));

      status->Monitor(t, _("Failed to retrieve messages to edit them."));
   }
}

void
MsgCmdProcImpl::BounceMessages(const UIdArray& messages)
{
   // TODO: should use a generic address entry control
   String address;
   if ( !MInputBox(&address,
                   _("Please enter the address"),
                   _("Bounce the message to:"),
                   GetFrame(),
                   "BounceAddress") )
   {
      // cancelled by user
      return;
   }

   size_t countOk = 0,
          count = messages.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      Message *msg = GetMessage(messages[n]);
      if ( !msg )
      {
         ERRORMESSAGE((_("Failed to retrieve the message to bounce.")));
         continue;
      }

      if ( SendMessage::Bounce(address, GetProfile(), *msg, GetFrame()) )
      {
         countOk++;
      }
      //else: error message already given

      msg->DecRef();
   }

   STATUSMESSAGE((_("Bounced %lu messages."), (unsigned long)countOk));
}

void
MsgCmdProcImpl::ResendMessages(const UIdArray& messages)
{
   size_t count = messages.GetCount();
   if ( !count )
      return;

   if ( !MDialog_YesNoDialog
         (
            String::Format
            (
               _("Do you want to resend the %lu selected messages?"),
               (unsigned long)count
            ),
            GetFrame(),
            MDIALOG_YESNOTITLE,
            M_DLG_NO_DEFAULT,
            M_MSGBOX_CONFIRM_RESEND
         ) )
   {
      // cancelled by user
      return;
   }

   size_t countOk = 0;
   for ( size_t n = 0; n < count; n++ )
   {
      Message_obj msg(GetMessage(messages[n]));
      if ( !msg )
      {
         ERRORMESSAGE((_("Failed to retrieve the message to be resent")));
         continue;
      }

      SendMessage_obj sendMsg(SendMessage::CreateResent
                              (
                                  GetProfile(),
                                  msg.Get(),
                                  GetFrame()
                              ));

      if ( !sendMsg )
      {
         ERRORMESSAGE((_("Failed to create a new message.")));
         continue;
      }

      String to = msg->GetAddressesString(MAT_TO),
             cc = msg->GetAddressesString(MAT_CC),
             bcc = msg->GetAddressesString(MAT_BCC);

      sendMsg->SetAddresses(to, cc, bcc);

      if ( !sendMsg->SendOrQueue() )
      {
         ERRORMESSAGE((_("Failed to resend the message %lu."), (unsigned long)n));
      }
      else
      {
         countOk++;
      }
   }

   STATUSMESSAGE((_("Resent %lu messages."), (unsigned long)countOk));
}

void
MsgCmdProcImpl::PrintOrPreviewMessages(const UIdArray& /* selections */,
                                       bool view)
{
   // this doesn't work because ShowMessage() is async and so the message is not
   // shown yet when Print() or PrintPreview() is called - we'd need to do
   // everything asynchronously in fact, but I don't have time for it now
   //
   // FIXME: we shouldn't tell the user to do something we can do ourselves, of
   //        course... and we should allow printing many messages at once
#if 0
   UIdType uidPreviewedOld = m_msgView->GetUId();

   size_t n = selections.Count();
   for ( size_t i = 0; i < n; i++ )
   {
      m_msgView->ShowMessage(selections[i]);

      if ( view )
         m_msgView->PrintPreview();
      else
         m_msgView->Print();
   }

   if ( uidPreviewedOld != UID_ILLEGAL )
   {
      // restore previously previewed message
      m_msgView->ShowMessage(uidPreviewedOld);
   }
#else // 1
   UIdType uidPreviewedOld = m_msgView->GetUId();
   if ( uidPreviewedOld == UID_ILLEGAL )
   {
      wxLogError(_("Please preview a message before printing it."));

      return;
   }

   // so print just the previewed message
   if ( view )
      m_msgView->PrintPreview();
   else
      m_msgView->Print();
#endif // 0/1
}

void
MsgCmdProcImpl::PrintMessages(const UIdArray& selections)
{
   PrintOrPreviewMessages(selections, false /* print */);
}

void
MsgCmdProcImpl::PrintPreviewMessages(const UIdArray& selections)
{
   PrintOrPreviewMessages(selections, true /* preview */);
}

// ----------------------------------------------------------------------------
// [un]deleting messages
// ----------------------------------------------------------------------------

void
MsgCmdProcImpl::DeleteOrTrashMessages(const UIdArray& selections)
{
   AsyncStatusHandler *status =
      new AsyncStatusHandler(this, _("Deleting messages..."));

   status->Monitor(m_asmf->DeleteOrTrashMessages
                           (
                              &selections,
                              MailFolder::DELETE_ALLOW_TRASH,
                              this
                           ),
                   _("Failed to delete messages"));
}

bool
MsgCmdProcImpl::DeleteAndExpungeMessages(const UIdArray& selections)
{
   if ( !MDialog_YesNoDialog
         (
            String::Format
            (
               _("Do you really want to permanently delete "
                 "the %lu selected messages?\n"
                 "\n"
                 "Note that it will be impossible to restore them!"),
               (unsigned long)selections.Count()
            ),
            GetFrame(),
            MDIALOG_YESNOTITLE,
            M_DLG_NO_DEFAULT | M_DLG_NOT_ON_NO | M_DLG_DISABLE,
            M_MSGBOX_CONFIRM_ZAP
         ) )
   {
      // cancelled by user
      return false;
   }

   AsyncStatusHandler *status =
      new AsyncStatusHandler(this, _("Permanently deleting messages..."));

   status->Monitor(m_asmf->DeleteMessages(&selections, true /* expunge */, this),
                   _("Failed to permanently delete messages"));

   return true;
}

void
MsgCmdProcImpl::UndeleteMessages(const UIdArray& selections)
{
   m_TicketList->Add(m_asmf->UnDeleteMessages(&selections, this));
}

// ----------------------------------------------------------------------------
// changing message flags
// ----------------------------------------------------------------------------

void
MsgCmdProcImpl::ToggleMessagesFlag(const UIdArray& messages,
                                   MailFolder::MessageStatus flag)
{
   MailFolder_obj mf(GetMailFolder());
   CHECK_RET( mf, _T("no folder in MsgCmdProcImpl::ToggleMessagesFlag") );

   HeaderInfoList_obj hil(mf->GetHeaders());
   CHECK_RET( hil, _T("can't toggle messages flag without folder listing") );

   size_t count = messages.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      // find the corresponding entry in the listing
      UIdType uid = messages[n];
      UIdType idx = hil->GetIdxFromUId(uid);
      if ( idx == UID_ILLEGAL )
      {
         wxLogDebug(_T("Inexistent message (uid = %lu) selected?"), uid);

         continue;
      }

      HeaderInfo *hi = hil->GetItemByIndex(idx);
      if ( !hi )
      {
         FAIL_MSG( _T("HeaderInfoList::GetItemByIndex() failed?") );

         continue;
      }

      // is the message currently flagged?
      const bool flagged = (hi->GetStatus() & flag) != 0;

      // invert the flag
      m_asmf->SetMessageFlag(uid, flag, !flagged);
   }
}

void
MsgCmdProcImpl::MarkRead(const UIdArray& selections, bool read)
{
   const unsigned long count = selections.GetCount();

   AsyncStatusHandler *status =
      new AsyncStatusHandler(this, wxString::Format
                                   (
                                       _("Marking %lu message(s) as %s..."),
                                       count, (read ? "read" : "unread")
                                   ));

   Ticket t = m_asmf->MarkRead(&selections, this, read);

   status->Monitor(t, _("Failed to mark messages."));
}

// ----------------------------------------------------------------------------
// copying/moving messages
// ----------------------------------------------------------------------------

Ticket
MsgCmdProcImpl::SaveMessagesToFolder(const UIdArray& selections,
                                     MFolder *folder)
{
   if ( !folder )
   {
      folder = MDialog_FolderChoose(GetFrame());
      if ( !folder )
      {
         // cancelled by user
         return ILLEGAL_TICKET;
      }
   }
   else
   {
      folder->IncRef(); // to match DecRef() below
   }

   const unsigned long count = selections.GetCount();

   AsyncStatusHandler *status =
      new AsyncStatusHandler(this, wxString::Format
                                   (
                                       _("Saving %lu message(s) to '%s'..."),
                                       count, folder->GetFullName()
                                   ));

   Ticket t = m_asmf->
                  SaveMessagesToFolder(&selections, GetFrame(), folder, this);

   status->Monitor(t, wxString::Format
                      (
                         _("Failed to save messages to the folder '%s'."),
                         folder->GetFullName()
                      ));

   folder->DecRef();

   return t;
}

Ticket
MsgCmdProcImpl::MoveMessagesToFolder(const UIdArray& messages, MFolder *folder)
{
   Ticket t = SaveMessagesToFolder(messages, folder);
   if ( t != ILLEGAL_TICKET )
   {
      // delete messages once they're successfully saved
      AddTicketToDelete(t);
   }

   return t;
}

Ticket
MsgCmdProcImpl::SaveMessagesToFile(const UIdArray& selections)
{
   const unsigned long count = selections.GetCount();

   AsyncStatusHandler *status =
      new AsyncStatusHandler(this, wxString::Format
                                   (
                                       _("Saving %lu message(s) to file..."),
                                       count
                                   ));

   Ticket t = m_asmf->SaveMessagesToFile(&selections, GetFrame(), this);
   status->Monitor(t, _("Saving messages to file failed."));

   return t;
}

// ----------------------------------------------------------------------------
// misc
// ----------------------------------------------------------------------------

void
MsgCmdProcImpl::ExtractAddresses(const UIdArray& selections)
{
   // FIXME: should this be implemented as async operation?

   MailFolder_obj mf(GetMailFolder());
   CHECK_RET( mf, _T("message preview without folder?") );

   // extract all addresses from the selected messages to this array
   wxArrayString addressesSorted;
   size_t count = selections.GetCount();

   MProgressDialog *dlg;
   if ( count > 10 )  // FIXME: hardcoded
   {
      dlg = new MProgressDialog
                (
                  _("Extracting addresses"),
                  _("Scanning messages..."),
                  count,
                  GetFrame(),       // parent
                  wxPD_CAN_ABORT
                );
   }
   else
   {
      dlg = NULL;
   }

   for ( size_t n = 0; n < count; n++ )
   {
      Message *msg = mf->GetMessage(selections[n]);
      if ( !msg )
      {
         FAIL_MSG( _T("selected message disappeared?") );

         continue;
      }

      if ( !msg->ExtractAddressesFromHeader(addressesSorted) )
      {
         // very strange
         wxLogWarning(_("Selected message doesn't contain any valid addresses."));
      }

      msg->DecRef();

      if ( dlg && !dlg->Update(n) )
      {
         // interrupted
         break;
      }
   }

   delete dlg;

   addressesSorted.Sort();
   wxArrayString addresses = strutil_uniq_array(addressesSorted);
   if ( !addresses.IsEmpty() )
   {
      InteractivelyCollectAddresses(addresses,
                                    READ_APPCONFIG(MP_AUTOCOLLECT_ADB),
                                    mf->GetName(),
                                    GetFrame());
   }
}

// ----------------------------------------------------------------------------
// Drag and drop
// ----------------------------------------------------------------------------

void
MsgCmdProcImpl::DropMessagesToFolder(const UIdArray& selections,
                                     MFolder *folder)
{
   wxLogTrace(M_TRACE_DND, _T("Saving %lu message(s) to folder '%s'"),
              (unsigned long)selections.GetCount(),
              folder->GetFullName());

   Ticket t = SaveMessagesToFolder(selections, folder);

   if ( !m_TicketsDroppedList )
   {
      wxLogTrace(M_TRACE_DND, _T("Creating m_TicketsDroppedList"));

      m_TicketsDroppedList = ASTicketList::Create();
   }

   m_TicketsDroppedList->Add(t);
}

bool
MsgCmdProcImpl::DragAndDropMessages(const UIdArray& selections)
{
   // normally DragAndDropMessages() shouldn't be called at all in this case
   // (i.e. currently it is only called from wxFolderView which does provide a
   // window for us but not from wxMessageViewFrame)
   CHECK( m_winForDnd, false, _T("this msg view doesn't support dnd") );

   bool didDrop = false;

#if wxUSE_DRAG_AND_DROP
   MailFolder_obj mf(GetMailFolder());
   CHECK( mf, false, _T("no mail folder to drag messages from?") );

   MMessagesDataObject dropData(this, mf, selections);

   // setting up the dnd icons can't be done in portable way :-(
#if defined(__WXMSW__) || defined(__WXMAC__)
   wxDropSource dropSource(dropData, m_winForDnd,
                           wxCursor(_T("msg_copy")),
                           wxCursor(_T("msg_move")));
#else // Unix
   wxIconManager *iconManager = mApplication->GetIconManager();
   wxIcon icon = iconManager->GetIcon(selections.Count() > 1 ? _T("dnd_msgs")
                                                             : _T("dnd_msg"));
   wxDropSource dropSource(dropData, m_winForDnd, icon);
#endif // OS

   switch ( dropSource.DoDragDrop(wxDrag_DefaultMove) )
   {
      default:
         wxFAIL_MSG( _T("unexpected DoDragDrop return value") );
         // fall through

      case wxDragError:
         // always give this one in debug mode, it is not supposed to
         // happen!
         wxLogDebug(_T("An error occured during drag and drop operation"));
         break;

      case wxDragNone:
      case wxDragCancel:
         wxLogTrace(M_TRACE_DND, _T("Drag and drop aborted by user."));
         break;

      case wxDragMove:
         if ( m_TicketsDroppedList )
         {
            // we have to delete the messages as they were moved
            while ( !m_TicketsDroppedList->IsEmpty() )
            {
               Ticket t = m_TicketsDroppedList->Pop();

               // the message hasn't been saved yet, wait with deletion
               // until it is copied successfully
               AddTicketToDelete(t);
            }

            // also delete the messages which have been already saved
            if ( !m_UIdsCopiedOk.IsEmpty() )
            {
               m_TicketList->Add
               (
                  // true => expunge as well
                  m_asmf->DeleteMessages(&m_UIdsCopiedOk, true, this)
               );
               m_UIdsCopiedOk.Empty();

               wxLogTrace(M_TRACE_DND, _T("Deleted previously dropped msgs."));
            }
            else
            {
               // maybe we didn't have time to really copy the messages
               // yet, then they will be deleted later
               wxLogTrace(M_TRACE_DND, _T("Dropped msgs will be deleted later"));
            }
         }
         break;

      case wxDragCopy:
         // nothing special to do
         break;
   }

   if ( m_TicketsDroppedList )
   {
      didDrop = true;

      m_TicketsDroppedList->DecRef();
      m_TicketsDroppedList = NULL;

      wxLogTrace(M_TRACE_DND, _T("DragAndDropMessages() done ok"));
   }
   else
   {
      wxLogTrace(M_TRACE_DND, _T("Nothing dropped"));
   }

   if ( !m_UIdsCopiedOk.IsEmpty() )
   {
      m_UIdsCopiedOk.Empty();
   }
#endif // wxUSE_DRAG_AND_DROP

   // did we do anything?
   return didDrop;
}

void
MsgCmdProcImpl::ApplyFilters(const UIdArray& selections)
{
   const unsigned long count = selections.GetCount();

   AsyncStatusHandler *status =
      new AsyncStatusHandler(this, wxString::Format
                                   (
                                      _("Applying filter rules to %lu message(s)..."),
                                      count
                                   ));

   Ticket t = m_asmf->ApplyFilterRules(&selections, this);
   if ( status->Monitor(t, _("Failed to apply filter rules.")) )
   {
      status->SetSuccessMsg(wxString::Format
                            (
                              _("Applied filters to %lu message(s), "
                              "see log window for details."),
                              count
                            ));
   }
}

// ----------------------------------------------------------------------------
// MsgCmdProc async stuff
// ----------------------------------------------------------------------------

bool
MsgCmdProcImpl::OnMEvent(MEventData& ev)
{
   if ( ev.GetId() != MEventId_ASFolderResult )
      return true;

   ASMailFolder::Result *result = ((MEventASFolderResultData&)ev).GetResult();

   if ( result->GetUserData() == this )
   {
      const Ticket& t = result->GetTicket();

      ASSERT_MSG( m_TicketList->Contains(t), _T("unexpected async result ticket") );

      m_TicketList->Remove(t);

      ASMailFolder::OperationId operation = result->GetOperation();

      int resval = ((ASMailFolder::ResultInt *)result)->GetValue();

      // the result is normally 0 if an error occured except for the apply
      // filter case where it can be 0 if no messages were filtered (which is
      // not an error)
      bool ok;
      if ( operation == ASMailFolder::Op_ApplyFilterRules )
         ok = resval != FilterRule::Error;
      else
         ok = resval != 0;

      // find the corresponding AsyncStatusHandler object, if any
      bool hadStatusObject = false;
      size_t progressCount = m_arrayAsyncStatus.GetCount();
      for ( size_t n = 0; n < progressCount; n++ )
      {
         AsyncStatusHandler *asyncStatus = m_arrayAsyncStatus[n];
         if ( asyncStatus->GetTicket() == t )
         {
            if ( !ok )
            {
               // tell it to show the error message, not success one
               asyncStatus->Fail();
            }

            delete asyncStatus;
            m_arrayAsyncStatus.RemoveAt(n);

            hadStatusObject = true;

            break;
         }
      }

      switch ( operation )
      {
         case ASMailFolder::Op_SaveMessagesToFolder:
            // we may have to do a few extra things here:
            //
            //  - if the message was marked for deletion (its ticket is in
            //    m_TicketsToDeleteList), we have to delete it and give a
            //    message about a successful move and not copy operation
            //
            //  - if we're inside DoDragDrop(), m_TicketsDroppedList is !NULL
            //    but we don't know yet if we will have to delete dropped
            //    messages or not, so just remember those of them which were
            //    copied successfully
            {
               String msg;

               bool toDelete = m_TicketsToDeleteList &&
                                    m_TicketsToDeleteList->Contains(t);
               bool wasDropped = m_TicketsDroppedList &&
                                    m_TicketsDroppedList->Contains(t);

               // remove from lists before testing value: this should be done
               // whether we succeeded or failed
               if ( toDelete )
               {
                  m_TicketsToDeleteList->Remove(t);
               }

               if ( wasDropped )
               {
                  m_TicketsDroppedList->Remove(t);

                  wxLogTrace(M_TRACE_DND, _T("Dropped msgs copied ok"));
               }

               if ( !ok )
               {
                  // something failed - what?
                  if ( toDelete )
                     msg = _("Moving messages failed.");
                  else if ( wasDropped )
                     msg = _("Dragging messages failed.");
                  else
                     msg = _("Copying messages failed.");
               }
               else // success
               {
                  // message was copied ok, what else to do with it?
                  UIdArray *seq = result->GetSequence();
                  CHECK( seq, false, _T("invalid async event data") );

                  unsigned long count = seq->Count();

                  if ( wasDropped )
                  {
                     if ( !toDelete )
                     {
                        // remember the UIDs as we may have to delete them later
                        // even if they are not in m_TicketsToDeleteList yet
                        WX_APPEND_ARRAY(m_UIdsCopiedOk, (*seq));
                     }
                     //else: dropped and already marked for deletion, delete below

                     msg.Printf(_("Dropped %lu message(s)."), count);
                  }
                  //else: not dropped

                  if ( toDelete )
                  {
                     // delete right now

                     // don't copy the messages to the trash, they
                     // had been already copied somewhere
                     Ticket tDel = m_asmf
                                 ? m_asmf->DeleteOrTrashMessages
                                   (
                                    seq,
                                    MailFolder::DELETE_NO_TRASH,
                                    this
                                   )
                                 : ILLEGAL_TICKET;

                     if ( tDel != ILLEGAL_TICKET )
                     {
                        m_TicketList->Add(tDel);
                     }
                     else // failed to delete?
                     {
                        wxLogError(_("Failed to delete messages after "
                                     "moving them."));
                     }

                     if ( !wasDropped )
                     {
                        msg.Printf(_("Moved %lu message(s)."), count);
                     }
                     //else: message already given above
                  }
                  else // simple copy, not move
                  {
                     if ( !hadStatusObject )
                     {
                        msg.Printf(_("Copied %lu message(s)."), count);
                     }
                     //else: message already given
                  }
               }

               if ( !msg.empty() )
               {
                  wxLogStatus(GetFrame(), msg);
               }
            }
            break;

         case ASMailFolder::Op_GetMessage:
            if ( m_TicketsToEditList->Contains(t) )
            {
               m_TicketsToEditList->Remove(t);

               // edit the message in composer
               Message *msg =
                  ((ASMailFolder::ResultMessage *)result)->GetMessage();

               Composer::EditMessage(GetProfile(), msg);
            }
            else
            {
               // we don't use them for anything else yet
               FAIL_MSG( _T("unexpected GetMessage() ticket") );
            }

         // nothing special to do for these cases
         case ASMailFolder::Op_ApplyFilterRules:
         case ASMailFolder::Op_SaveMessagesToFile:
         case ASMailFolder::Op_ReplyMessages:
         case ASMailFolder::Op_ForwardMessages:
         case ASMailFolder::Op_DeleteMessages:
         case ASMailFolder::Op_UnDeleteMessages:
         case ASMailFolder::Op_MarkRead:
         case ASMailFolder::Op_MarkUnread:
            break;

         default:
            FAIL_MSG( _T("unexpected async operation result") );
      }
   }

   result->DecRef();

   return true;
}

void MsgCmdProcImpl::RemoveAsyncStatus(AsyncStatusHandler *asyncStatus)
{
   size_t progressCount = m_arrayAsyncStatus.GetCount();
   for ( size_t n = 0; n < progressCount; n++ )
   {
      if ( asyncStatus == m_arrayAsyncStatus[n] )
      {
         m_arrayAsyncStatus.RemoveAt(n);

         return;
      }
   }

   wxFAIL_MSG( _T("async status not found in m_arrayAsyncStatus") );
}

void MsgCmdProcImpl::AddAsyncStatus(AsyncStatusHandler *asyncStatus)
{
   m_arrayAsyncStatus.Add(asyncStatus);
}

