/*-*- c++ -*-********************************************************
 * MailCollector class                                              *
 *                                                                  *
 * (C) 1999 by Karsten Ballüder (Ballueder@usa.net)                 *
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

#include "gui/wxMDialogs.h"   // MDialog_YesNoDialog

// instead of writing our own wrapper for wxExecute()
#include  <wx/utils.h>
#define   SYSTEM(command) wxExecute(command, FALSE)

struct MailCollectorFolderEntry
{
   String      m_name;
   MailFolder *m_folder;
};

KBLIST_DEFINE(MailCollectorFolderList, MailCollectorFolderEntry);

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
            MailFolder *mf = MailFolder::OpenFolder(MF_PROFILE,folderName);
            if(mf)
            {
               MailCollectorFolderEntry *e = new MailCollectorFolderEntry;
               e->m_name = folderName;
               e->m_folder = mf;
               m_list->push_back(e);
            }
            else
               wxLogError(_("Cannot open incoming folder '%s'."), folderName.c_str());
         }
         if(f)f->DecRef();
         return true;
      }
   
private:
   MFolder *m_folder;
   MailCollectorFolderList *m_list;
};



MailCollector::MailCollector()
{
   Lock(false);
   m_list = new MailCollectorFolderList;
   MAppFolderTraversal t (m_list);
   if(! t.Traverse(true))
      wxLogError(_("Cannot build list of incoming mail folders."));
   // keep it open all the time to speed things up
   m_NewMailFolder = NULL;
   SetNewMailFolder(READ_APPCONFIG(MP_NEWMAIL_FOLDER));
}

MailCollector::~MailCollector()
{
   MailCollectorFolderList::iterator i;
   for(i = m_list->begin();i != m_list->end(); i++)
      (**i).m_folder->DecRef();
   if(m_NewMailFolder)
   {
      m_NewMailFolder->DecRef();
      m_NewMailProfile->DecRef();
   }
   delete m_list;
}

bool
MailCollector::IsIncoming(MailFolder *mf)
{
   MailCollectorFolderList::iterator i;
   for(i = m_list->begin();i != m_list->end();i++)
      if((**i).m_folder == mf)
         return true;
   return false;
}

bool
MailCollector::Collect(MailFolder *mf)
{
   m_Message = "";
   m_Count = 0;
   bool rc = true;

   CHECK(m_NewMailFolder,false,_("Cannot collect mail without New Mail folder."));

   if(IsLocked())
      return true; // didn't collect, but no error
   
   bool locked = Lock();
   if(mf == NULL)
   {
      MailCollectorFolderList::iterator i;
      for(i = m_list->begin();i != m_list->end();i++)
      {
         // set flags each time because they get reset by SaveMessages()
         m_NewMailFolder->EnableNewMailEvents(false,false);
         rc &= CollectOneFolder((*i)->m_folder);
      }
   }
   else
      rc = CollectOneFolder(mf);

   m_NewMailFolder->EnableNewMailEvents(true,true);
   m_NewMailFolder->Ping();

   if(m_Count)
   {
      // step 1: execute external command if it's configured
      String command = READ_CONFIG(m_NewMailFolder->GetProfile(), MP_NEWMAILCOMMAND);
      if(! command.IsEmpty())
      {
         if ( ! SYSTEM(command) )
         {
            // TODO ask whether the user wants to disable it
            wxLogError(_("Command '%s' (to execute on new mail reception)"
                         " failed."), command.c_str());
         }
      }

#ifdef   USE_PYTHON
      // step 2: folder specific Python callback
      if(! PythonCallback(MCB_FOLDER_NEWMAIL, 0, m_NewMailFolder, m_NewMailFolder->GetClassName(),
                          m_NewMailFolder->GetProfile())
         && 
      // step 3: global python callback
         ! PythonCallback(MCB_MAPPLICATION_NEWMAIL, 0, mApplication, "MApplication",
                          mApplication->GetProfile()))
#endif //USE_PYTHON   
         if(READ_CONFIG(m_NewMailProfile, MP_SHOW_NEWMAILMSG))
         {
            String
               text, title;
            text.Printf(_("You have %lu new messages in folder '%s'.\n"),
                        m_Count, m_NewMailFolder->GetName().c_str());
            if ( m_Count <= (unsigned long) READ_CONFIG(m_NewMailProfile,MP_SHOW_NEWMAILINFO)) 
               text << m_Message;
            // TODO make it a wxPMessageBox to let the user shut if off from here?
            MDialog_Message(text, NULL, _("New Mail"));
         }
   }
   Lock(locked);
   return rc;
}

bool
MailCollector::CollectOneFolder(MailFolder *mf)
{
   ASSERT(mf);
   bool rc = true;
   
   bool locked = Lock();
   if(mf == m_NewMailFolder)
   {
      wxLogError(_(
         "You are trying to collect mail from your central New Mail folder.\n"
         "This makes no sense. Please correct its settings."));
      RemoveIncomingFolder(mf->GetName());
      return false;
   }
   wxLogStatus(_("Auto-collecting mail from incoming folder '%s'."),
               mf->GetName().c_str());
   wxYield(); // normal wxYield() should be ok here, this code never
   // gets called from a menu or such
   long oldcount = m_NewMailFolder->CountMessages();
   bool sendsEvents = mf->SendsNewMailEvents();
   mf->EnableNewMailEvents(false,true);
   mf->Ping(); //update it
   INTARRAY selections;
   
   const HeaderInfo *hi = mf->GetFirstHeaderInfo();
   if(hi)
   {
      m_Message << _("From folder '") << mf->GetName() << "':\n";
      for(;
          hi;
          hi = mf->GetNextHeaderInfo(hi))
      {
         selections.Add(hi->GetUId());
         m_Count ++;
         m_Message << _("  Subject: ") << hi->GetSubject()
                   << _("  From: ") << hi->GetFrom()
                   << '\n';
      }
      if(mf->SaveMessages(&selections,
                          READ_APPCONFIG(MP_NEWMAIL_FOLDER),
                          true /* isProfile */, true /* update count */))
      {
         mf->DeleteMessages(&selections);
         mf->ExpungeMessages();
         rc = true;
      }
      else
         rc = false;
      long i = 0;
      String seq;
      for(hi = m_NewMailFolder->GetFirstHeaderInfo();
          hi;
          hi = m_NewMailFolder->GetNextHeaderInfo(hi))
      {
         if(i >= oldcount)
         {
            if(seq.Length()) seq << ',';
            seq << strutil_ultoa(hi->GetUId());
         }
         i++;
      }
      // mark new messages as new:
      m_NewMailFolder->SetSequenceFlag(seq,MailFolder::MSG_STAT_RECENT, true);
      m_NewMailFolder->SetSequenceFlag(seq,MailFolder::MSG_STAT_SEEN, false);
   }
   mf->EnableNewMailEvents(sendsEvents);
   mf->Ping(); //update it
   Lock(locked);
   return rc;
}


void
MailCollector::SetNewMailFolder(const String &name)
{
   if(m_NewMailFolder)
   {
      m_NewMailFolder->DecRef();
      m_NewMailProfile->DecRef();
   }
   m_NewMailFolder = MailFolder::OpenFolder(MF_PROFILE, name);
   m_NewMailProfile = ProfileBase::CreateProfile(name);
}


bool
MailCollector::AddIncomingFolder(const String &name)
{
   MailFolder *mf = MailFolder::OpenFolder(MF_PROFILE,name);
   if(mf)
   {
      MailCollectorFolderEntry *e = new MailCollectorFolderEntry;
      e->m_name = name;
      e->m_folder = mf;
      m_list->push_back(e);
      return true;
   }
   else
      return false;
}

bool
MailCollector::RemoveIncomingFolder(const String &name)
{
   MailCollectorFolderList::iterator i;
   for(i = m_list->begin();i != m_list->end();i++)
   {
      if((**i).m_name == name)
      {
         (**i).m_folder->DecRef();
         m_list->erase(i);
         return true;
      }
   }

   // it's not an error - may be the folder couldn't be opened during this
   // even though it does have "autocollect" flag set

   return false;
}

