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

#define MC_MAX_FAIL   5

struct MailCollectorFolderEntry
{
   String      m_name;
   /** Failcount is 0 initially, when it reaches MC_MAX_FAIL a warning is
       printed. If set to -1, user has been warned about it being
       inaccessible and no more warnings are printed. */
   int m_failcount;
};

KBLIST_DEFINE(MailCollectorFolderList, MailCollectorFolderEntry);

class MailCollectorImpl : public MailCollector
{
public:
   MailCollectorImpl()
      {
         InternalCreate();
      }
   /// Returns true if the mailfolder mf is an incoming folder.
   virtual bool IsIncoming(MailFolder *mf);
   /** Collect all mail from folder mf.
       @param mf the folder to collect from
       @return true on success
   */
   virtual bool Collect(MailFolder *mf = NULL);
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
         InternalDestroy();
      }
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
            e->m_failcount = 0;
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
   m_list = new MailCollectorFolderList;
   MAppFolderTraversal t (m_list);
   if(! t.Traverse(true))
      wxLogError(_("Cannot build list of incoming mail folders."));
   m_ReInit = false;
}

void
MailCollectorImpl::InternalDestroy(void)
{
   MOcheck();
   delete m_list;
}

bool
MailCollectorImpl::IsIncoming(MailFolder *mf)
{
   ReCreate();
   MOcheck();
   MailCollectorFolderList::iterator i;
   for(i = m_list->begin();i != m_list->end();i++)
   if( (**i).m_name == mf->GetName() )
      return true;
   return false;
}

bool
MailCollectorImpl::Collect(MailFolder *mf)
{
   ReCreate();

   MOcheck();
   bool rc = true;

   // first thing to do is to ping all currently opened folders as we want to
   // do it anyhow - even if we don't use "new mail" folder - as checking for
   // new mail should update them
   MailFolder::PingAllOpened();

   // maybe we've got nothing to do?
   if(mf == NULL && m_list->size() == 0)
      return TRUE;
   
   // first case, mf==NULL, collect from all incoming folders:
   if(mf == NULL)
   {
      MailCollectorFolderList::iterator i;
      for(i = m_list->begin();i != m_list->end();i++)
      {
         MFolder *mfolder = MFolder::Get( (**i).m_name );
         if(! mfolder)
         {
            ERRORMESSAGE((_("Cannot find incoming folder '%s'."),
                          (**i).m_name.c_str()));
            RemoveIncomingFolder((**i).m_name);
            continue; // skip this one
         }

         // has online status changed?
         if( (**i).m_failcount < 0 // failed previously
             && mfolder->NeedsNetwork()
             && mApplication->IsOnline())
            (**i).m_failcount = 0; // try again

         if(mfolder->NeedsNetwork()
            && ! mApplication->IsOnline()
            && (**i).m_failcount != -1)
         {
            ERRORMESSAGE((_("Cannot collect from incoming mailbox '%s'"
                          " while network is offline."),
                          (**i).m_name.c_str()));
            (**i).m_failcount = -1; // shut up
         }

         if( (**i).m_failcount == -1 )
         {
            mfolder->DecRef();
            continue; // skip to next in list
         }

         MailFolder *imf = MailFolder::OpenFolder( mfolder );
         mfolder->DecRef();
         if(imf)
         {
            rc &= (imf && CollectOneFolder(imf));
            imf->DecRef();
            (**i).m_failcount = 0;
         }
         else
         {
            ASSERT( (**i).m_failcount >= 0);
            (**i).m_failcount ++;
            if((**i).m_failcount > MC_MAX_FAIL)
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
               else
               {
                  // reset failure count for new tries
                  (**i).m_failcount = 0;
               }
            }
         }
      }
   }
   else
      // second case, collect from only one folder:
      rc = CollectOneFolder(mf);
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
   // Folder just needs to retrieve a listing to auto-collect
   SafeDecRef(mf->GetHeaders()); //update it
   return rc;
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
         m_list->erase(i);
         return true;
      }
   }

   // it's not an error - may be the folder couldn't be opened during this
   // even though it does have "autocollect" flag set

   return false;
}

