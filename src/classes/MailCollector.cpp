/*-*- c++ -*-********************************************************
 * MailCollector class                                              *
 *                                                                  *
 * (C) 1999-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *                                                                  *
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "MailCollector.h"
#endif

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef   USE_PCH
#  include  "Mcommon.h"
#  include  "strutil.h"
#  include  "kbList.h"
#  include  "MApplication.h"
#  include   "MPython.h"

#  include  <wx/dynarray.h>
#  include  <wx/utils.h> // wxYield
#endif   // USE_PCH

#include "Mdefaults.h"
#include "MFolder.h"

#include "FolderView.h"
#include "MailFolder.h"

#include "MailCollector.h"

#include "MDialogs.h"   // MDialog_YesNoDialog
#include "Mpers.h"

// instead of writing our own wrapper for wxExecute()
#include  <wx/utils.h>
#define   SYSTEM(command) wxExecute(command, FALSE)

// should the mailcollector keep folders open?
//#define MC_KEEP_OPEN

struct MailCollectorFolderEntry
{
   String      m_name;
#ifdef MC_KEEP_OPEN
   MailFolder *m_folder;
#endif
};

KBLIST_DEFINE(MailCollectorFolderList, MailCollectorFolderEntry);


class MCEventReceiver : public MEventReceiver
{
public:
   MCEventReceiver(class MailCollector *mc)
      {
         m_MC = mc;
         m_MEventCookie = MEventManager::Register(*this, MEventId_FolderStatus);
         ASSERT_MSG( m_MEventCookie, "can't reg mail collector with event manager");
    }
   ~MCEventReceiver()
      {
         MEventManager::Deregister(m_MEventCookie);
      }
   virtual inline bool OnMEvent(MEventData& event)
      {
#if 0
         STATUSMESSAGE(( _("Checking for new mail...")));
         m_MC->Collect();
         STATUSMESSAGE(( _("Checking for new mail... done.")));
#endif
         return true;
      }
private:
   MailCollector *m_MC;
   void *m_MEventCookie;
};

class MailCollectorImpl : public MailCollector
{
public:
   MailCollectorImpl()
      {
         InternalCreate();
         m_EventReceiver = new MCEventReceiver(this);
      }
   /// Returns true if the mailfolder mf is an incoming folder.
   virtual bool IsIncoming(MailFolder *mf);
   /** Collect all mail from folder mf.
       @param mf the folder to collect from
       @return true on success
   */
   virtual bool Collect(MailFolder *mf = NULL);
   /** Tells the object about a new new mail folder.
       @param name use this folder as the new mail folder
   */
   virtual void SetNewMailFolder(const String &name);
   /** Adds a new incoming folder to the list.
       @param name folder to collect from
       @return true on success, false if folder was not found
   */
   virtual bool AddIncomingFolder(const String &name);
   /** Removes an incoming folder from the list.
       @param name no longer collect from this folder
       @return true on success, false if folder was not found
   */
   virtual bool RemoveIncomingFolder(const String &name);
   /** Returns true if the collector is locked.
       @return true if collecting
   */
   virtual bool IsLocked(void) const { return m_IsLocked; }
   /** Locks or unlocks the mail collector. If it is locked, it simly
       does nothing.
       @param lock true=lock, false=unlock
       @return the old state
   */
   virtual bool Lock(bool lock = true)
      { bool rc = m_IsLocked; m_IsLocked =lock; return rc; }
   /** Ask the MailCollector to re-initialise on next collection.
    */
   virtual void RequestReInit(void)
      {
         m_ReInit = true;
      }
protected:
   /// Collect mail from this one folder.
   bool CollectOneFolder(MailFolder *mf);
   ~MailCollectorImpl()
      {
         MOcheck();
         delete m_EventReceiver;
         InternalDestroy();
      }
#ifdef MC_KEEP_OPEN
   /// re-opens any closed folders, depending on network status
   void UpdateFolderList(void);
#endif
   /// Re-initialise the MailCollector if needed
   void ReCreate(void)
      {
         MOcheck();
         if(m_ReInit) // re-init requested
         {
            InternalDestroy();
            InternalCreate();
            MOcheck();
         }
      }
private:
   void InternalCreate(void);
   void InternalDestroy(void);
   /// a list of folder names and mailfolder pointers
   class MailCollectorFolderList *m_list;
   /// the folder to save new mail to
   MailFolder     *m_NewMailFolder;
   /// profile for the new mail folder
   Profile    *m_NewMailProfile;
   /// are we not supposed to collect anything?
   bool           m_IsLocked;
   /// The message for the new mail dialog.
   String         m_Message;
   /// The number of new messages.
   unsigned long  m_Count;
   /// we react to events
   MCEventReceiver *m_EventReceiver;
   /// if this is set, we want to be re-initialised
   bool m_ReInit;
};

/* static */
MailCollector *
MailCollector::Create(void)
{
   // also start the mail auto collection timer
   mApplication->StartTimer(MAppBase::Timer_PollIncoming);

   return new MailCollectorImpl;
}

/// only used by MailCollector to find incoming folders
class MAppFolderTraversal : public MFolderTraversal
{
public:
   MAppFolderTraversal(MailCollectorFolderList *list)
      : MFolderTraversal(*(m_folder = MFolder::Get("")))
      { m_list = list; }
   ~MAppFolderTraversal()
      { m_folder->DecRef(); }
   bool OnVisitFolder(const wxString& folderName)
      {
         MFolder *f = MFolder::Get(folderName);
         if(f && f->GetFlags() & MF_FLAGS_INCOMING)
         {
            wxLogDebug("Found incoming folder '%s'.",
                       folderName.c_str());
            MailCollectorFolderEntry *e = new MailCollectorFolderEntry;
            e->m_name = folderName;
#ifdef MC_KEEP_OPEN
            e->m_folder = MailFolder::OpenFolder(folderName); // might be NULL!
#endif
            m_list->push_back(e);
         }
         if(f) f->DecRef();
         return true;
      }

private:
   MFolder *m_folder;
   MailCollectorFolderList *m_list;
};


void
MailCollectorImpl::InternalCreate(void)
{
   MOcheck();
   m_IsLocked = false;
   m_list = new MailCollectorFolderList;
   MAppFolderTraversal t (m_list);
   if(! t.Traverse(true))
      wxLogError(_("Cannot build list of incoming mail folders."));
   m_ReInit = false;
   m_NewMailFolder = NULL;

   // keep it open all the time to speed things up if we have any folders to
   // monitor
   if ( m_list->size() )
   {
      SetNewMailFolder(READ_APPCONFIG(MP_NEWMAIL_FOLDER));
   }
}

void
MailCollectorImpl::InternalDestroy(void)
{
   // the m_EventReceiver is not destroyed, we continue to use it
   MOcheck();
#ifdef MC_KEEP_OPEN
   MailCollectorFolderList::iterator i;
   for(i = m_list->begin();i != m_list->end(); i++)
      if((**i).m_folder)
         (**i).m_folder->DecRef();
#endif
   if(m_NewMailFolder)
   {
      m_NewMailFolder->DecRef();
      m_NewMailProfile->DecRef();
   }
   delete m_list;
}

bool
MailCollectorImpl::IsIncoming(MailFolder *mf)
{
   ReCreate();
   MOcheck();
   MailCollectorFolderList::iterator i;
   for(i = m_list->begin();i != m_list->end();i++)
#ifdef MC_KEEP_OPEN
      if((**i).m_folder == mf)
         return true;
#else
   if( (**i).m_name == mf->GetName() )
      return true;
#endif
   return false;
}

bool
MailCollectorImpl::Collect(MailFolder *mf)
{
   ReCreate();

   MOcheck();
   m_Message = "";
   m_Count = 0;
   bool rc = true;

   if ( !m_NewMailFolder )
   {
      if ( mf || m_list->size() )
      {
         // create it now
         SetNewMailFolder(READ_APPCONFIG(MP_NEWMAIL_FOLDER));
      }
      else
      {
         // nothing to do: no folders to auto collect from
         return true;
      }
   }

#ifdef MC_KEEP_OPEN
   UpdateFolderList();
#endif
   
   int updateFlags = m_NewMailFolder->GetUpdateFlags();
   if(mf == NULL)
   {
      MailCollectorFolderList::iterator i;
      for(i = m_list->begin();i != m_list->end();i++)
      {
         // set flags each time because they get reset by SaveMessages()
         m_NewMailFolder->SetUpdateFlags(0 /* no updates, no new mail
                                              detection */);
#ifdef MC_KEEP_OPEN
         if ((*i)->m_folder)
            rc &= CollectOneFolder((*i)->m_folder);
#else   
         MFolder *mfolder = MFolder::Get( (**i).m_name );
         if(! mfolder)
         {
            ERRORMESSAGE((_("Cannot find incoming folder '%s'."),
                          (**i).m_name.c_str()));
            continue; // skip this one
         }

         if(mfolder->NeedsNetwork() && ! mApplication->IsOnline())
         {
            ERRORMESSAGE((_("Cannot collect from incoming mailbox '%s'"
                          " while network is offline."),
                          (**i).m_name.c_str()));
            continue; // skip to next in list
         }
         MailFolder *imf = MailFolder::OpenFolder( mfolder );
         mfolder->DecRef();
         if(imf)
         {
            rc &= (imf && CollectOneFolder(imf));
            imf->DecRef();
         }
         else
         {
            ERRORMESSAGE((_("Cannot open incoming mailbox '%s'."),
                          (**i).m_name.c_str()));
            wxString msg;
            msg.Printf(_("Accessing the incoming folder\n"
                         "'%s' failed.\n\n"
                         "Do you want to stop collecting\n"
                         "mail from it in this session?"),
                       (**i).m_name.c_str());
            if(MDialog_YesNoDialog(
               msg, NULL, _("Mail collection failed"),
               TRUE, GetPersMsgBoxName(M_MSGBOX_SUSPENDAUTOCOLLECT)))
            {
               RemoveIncomingFolder((**i).m_name);
               // re-start from beginning of list to avoid iterator trouble:
               i = m_list->begin();
            }
         }
#endif   
      }
   }
   else
      rc = CollectOneFolder(mf);

   /* We might have filter rules set on the NewMail folder, so we
      apply these as well: */
//   m_NewMailFolder->ApplyFilterRules(true);

   m_NewMailFolder->SetUpdateFlags(updateFlags);
   m_NewMailFolder->RequestUpdate();
   // requesting a listing triggers folder update:
   SafeDecRef(m_NewMailFolder->GetHeaders());
   return rc;
}

bool
MailCollectorImpl::CollectOneFolder(MailFolder *mf)
{
   ReCreate();
   MOcheck();
   ASSERT(mf);
   bool rc = true;

   if(mf->NeedsNetwork() && ! mApplication->IsOnline())
      return false;

   bool locked = Lock();
   if(locked) // was already locked
      return true; // not an error, just recursion avoidance
   if(mf == m_NewMailFolder)
   {
      wxLogError(_(
         "You are trying to collect mail from your central New Mail folder.\n"
         "This makes no sense. Please correct its settings."));
      RemoveIncomingFolder(mf->GetName());
      return false;
   }
   
#ifdef EXPERIMENTAL_newnewmail
   // all this now handled by folder itself
   mf->Ping(); //update it
   
#else
   
   mf->ApplyFilterRules(false);

   wxLogStatus(_("Auto-collecting mail from incoming folder '%s'."),
               mf->GetName().c_str());
//      wxSafeY   ield(); // normal wxYield() is not ok here
   int updateFlags = mf->GetUpdateFlags();
   mf->SetUpdateFlags(MailFolder::UF_UpdateCount);
   UIdArray selections;

   const HeaderInfo *hi;
   size_t i;
   HeaderInfoList *hil = mf->GetHeaders();
   if( hil )
   {
      if( hil->Count() > 0 )
      {
         m_Message << _("From folder '") << mf->GetName() << "':\n";
         for(i = 0; i < hil->Count(); i++)
         {
            hi=(*hil)[i];
            selections.Add(hi->GetUId());
            m_Count ++;
            m_Message << _("  Subject: ") << hi->GetSubject()
                      << _("  From: ") << hi->GetFrom()
                      << '\n';
         }
      }
      hil->DecRef();
   }
   else
   {
      wxLogStatus(_("Cannot get listing for incoming folder '%s'."),
                  mf->GetName().c_str());
      rc = false;
   }
   if(selections.Count() > 0)
   {
      if(mf->SaveMessages(&selections,
                          READ_APPCONFIG(MP_NEWMAIL_FOLDER),
                          true /* isProfile */, false /* update count */))
      {
         mf->DeleteMessages(&selections);
         mf->ExpungeMessages();
         rc = true;
      }
      else
         rc = false;
   }
   else
      rc = true;

   mf->SetUpdateFlags(updateFlags);
   mf->Ping(); //update it
#endif

   Lock(locked);

   return rc;
}


void
MailCollectorImpl::SetNewMailFolder(const String &name)
{
   ReCreate();
   MOcheck();
   if(m_NewMailFolder)
   {
      m_NewMailFolder->DecRef();
      m_NewMailProfile->DecRef();
   }
   m_NewMailProfile = Profile::CreateProfile(name);
   m_NewMailFolder = MailFolder::OpenFolder(name);
}


bool
MailCollectorImpl::AddIncomingFolder(const String &name)
{
   ReCreate();
   MOcheck();
   MailFolder *mf = MailFolder::OpenFolder(name);
   if(mf == NULL)
      return false;
   MailCollectorFolderEntry *e = new MailCollectorFolderEntry;
   e->m_name = name;
#ifdef MC_KEEP_OPEN
   e->m_folder = mf; // might be NULL
#endif
   m_list->push_back(e);
   return true;
}

bool
MailCollectorImpl::RemoveIncomingFolder(const String &name)
{
   ReCreate();
   MOcheck();
   MailCollectorFolderList::iterator i;
   for(i = m_list->begin();i != m_list->end();i++)
   {
      if((**i).m_name == name)
      {
#ifdef MC_KEEP_OPEN
         if ((**i).m_folder)
            (**i).m_folder->DecRef();
#endif
         m_list->erase(i);
         return true;
      }
   }

   // it's not an error - may be the folder couldn't be opened during this
   // even though it does have "autocollect" flag set

   return false;
}

#ifdef MC_KEEP_OPEN

/** Search for incoming folders which are open but cannot be accessed
    or which are closed and try to reopen them. */
void
MailCollectorImpl::UpdateFolderList(void)
{
   ReCreate();
   MOcheck();

   MailCollectorFolderList::iterator i;

   if(mApplication->IsOnline())
   {
      // Search for folders to try to open:
      for(i = m_list->begin();i != m_list->end();i++)
      {
         if((**i).m_folder == NULL) // try to open:
         {
            (**i).m_folder = MailFolder::OpenFolder((**i).m_name);
            if((**i).m_folder == NULL) // folder inaccessible:
            {
               ERRORMESSAGE((_("Cannot open incoming folder '%s'."),
                             (**i).m_name.c_str()));
               wxString msg;
               msg.Printf(_("Accessing the incoming folder\n"
                            "'%s' failed.\n\n"
                            "Do you want to stop collecting\n"
                            "mail from it in this session?"),
                          (**i).m_name.c_str());
               if(MDialog_YesNoDialog(
                  msg, NULL, _("Mail collection failed"),
                  TRUE, GetPersMsgBoxName(M_MSGBOX_SUSPENDAUTOCOLLECT)))
               {
                  RemoveIncomingFolder((**i).m_name);
                  // re-start from beginning of list to avoid iterator trouble:
                  i = m_list->begin();
               }
            }
         }
      }
   }
   else // we are offline, do we need to disable some folder:
   {
      // Search for folders to try to open:
      for(i = m_list->begin();i != m_list->end();i++)
      {
         if((**i).m_folder != NULL &&
            (**i).m_folder->NeedsNetwork())
         {
            (**i).m_folder->DecRef(); // close
            (**i).m_folder = NULL; // will try to re-open when online
         }
      }
   }
}
#endif
