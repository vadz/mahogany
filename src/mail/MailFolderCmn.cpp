//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/MailFolderCmn.cpp: generic MailFolder methods which don't
//              use cclient (i.e. don't really work with mail folders)
// Purpose:     handling of mail folders with c-client lib
// Author:      Karsten Ballüder
// Modified by:
// Created:     02.04.01 (extracted from mail/MailFolder.cpp)
// CVS-ID:      $Id$
// Copyright:   (C) 1997-2000 by Karsten Ballüder (ballueder@gmx.net)
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

#ifdef __GNUG__
#   pragma implementation "MailFolderCmn.h"
#endif

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include  "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "guidef.h"    // only for high-level functions
#  include "strutil.h"
#  include "Profile.h"
#  include "MEvent.h"
#  include "MApplication.h"
#endif

#include "Mdefaults.h"

#include "MFolder.h"
#include "MFilter.h"
#include "modules/Filters.h"

#include "MDialogs.h"         // for MProgressDialog

#include "HeaderInfo.h"

#include "MThread.h"

#include "MFCache.h"

#include "miscutil.h"         // for GetSequenceString()

#include <wx/timer.h>
#include <wx/datetime.h>
#include <wx/file.h>

#include "wx/persctrl.h"      // for wxPFileSelector

#include "MailFolderCmn.h"

// ----------------------------------------------------------------------------
// trace masks
// ----------------------------------------------------------------------------

// trace new mail processing
#define TRACE_NEWMAIL "newmail"

// trace mail folder ref counting
#define TRACE_MF_REF "mfref"

// trace mask for mail folder closing
#define TRACE_MF_CLOSE "mfclose"

// ----------------------------------------------------------------------------
// mailfolder closing helper classes
// ----------------------------------------------------------------------------

/**
   Idea: before actually closing a mailfolder, we keep it around for a
   some time. If someone reaccesses it, this speeds things up. So
   all we need, is a little helper class to keep a list of mailfolders
   open until a timeout occurs.
*/

// an entry which keeps one "closed" folder (it is really still opened)
class MfCloseEntry
{
public:
   enum
   {
      // special value for MfCloseEntry() ctor delay parameter
      NEVER_EXPIRES = INT_MAX
   };

   MfCloseEntry(MailFolderCmn *mf, int secs);
   ~MfCloseEntry();

   // restart the expire timer
   void ResetTimeout();

   // has this entry expired?
   bool HasExpired() const
   {
      return m_expires && (m_dt > wxDateTime::Now());
   }

   // does this entry match this folder?
   bool Matches(const MailFolder *mf) const
   {
      // an entry being deleted shouldn't match anything at all!
      return m_mf == mf
#ifdef DEBUG_FOLDER_CLOSE
            && !m_deleting
#endif // DEBUG_FOLDER_CLOSE
            ;
   }

private:
   // the folder we're going to close
   MailFolderCmn *m_mf;

   // the time when we will close it
   wxDateTime m_dt;

   // the timeout in seconds we're waiting before closing the folder
   int m_timeout;

   // if false, timeout is infinite
   bool m_expires;

#ifdef DEBUG_FOLDER_CLOSE
   // set to true just before deleting the folder
   bool m_deleting;
#endif // DEBUG_FOLDER_CLOSE
};

// a linked list of MfCloseEntries
KBLIST_DEFINE(MfList, MfCloseEntry);

// the timer which periodically really closes the previously "closed" folders
class MfCloser;
class MfCloseTimer : public wxTimer
{
public:
   MfCloseTimer(MfCloser *mfCloser) { m_mfCloser = mfCloser; }

   virtual void Notify(void);

private:
   MfCloser *m_mfCloser;
};

// the class which keeps alive all "closed" folders until next timeout
class MfCloser : public MObject
{
public:
   MfCloser();
   virtual ~MfCloser();

   // add a new "closed" folder to the list
   void Add(MailFolderCmn *mf, int delay);

   // close the folders which timed out
   void OnTimer(void);

   // close all folders (for example because the program terminates)
   void CleanUp(void);

   // get the entry for this folder or NULL if it's not in the list
   MfCloseEntry *GetCloseEntry(MailFolderCmn *mf) const;

   // restart the timer (useful if timer interval changed)
   void RestartTimer();

private:
   // the list of folder entries to close
   MfList m_MfList;

   // the timer we use for periodically checking for expired folders
   MfCloseTimer m_timer;

   // the interval of this timer (in seconds)
   int m_interval;
};

// ----------------------------------------------------------------------------
// MfCmnEventReceiver: event receiver for MailFolderCmn, reacts to options
//                     change event
// ----------------------------------------------------------------------------

class MfCmnEventReceiver : public MEventReceiver
{
public:
   MfCmnEventReceiver(MailFolderCmn *mf);
   virtual ~MfCmnEventReceiver();

   virtual bool OnMEvent(MEventData& event);

private:
   MailFolderCmn *m_Mf;

   void *m_MEventCookie,
        *m_MEventPingCookie;
};

/** a timer class to regularly ping the mailfolder. */
class MailFolderTimer : public wxTimer
{
public:
   /** constructor
       @param mf the mailfolder to query on timeout
   */
   MailFolderTimer(MailFolder *mf)
   {
      m_mf = mf;
   }

   /// get called on timeout and pings the mailfolder
   void Notify(void) { if(! m_mf->IsLocked() ) m_mf->Ping(); }

protected:
   /// the mailfolder to update
   MailFolder *m_mf;
};

// ----------------------------------------------------------------------------
// module global variables
// ----------------------------------------------------------------------------

// the unique MfCloser object
static MfCloser *gs_MailFolderCloser = NULL;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MfCloseEntry
// ----------------------------------------------------------------------------

MfCloseEntry::MfCloseEntry(MailFolderCmn *mf, int secs)
{
   wxLogTrace(TRACE_MF_CLOSE,
              "Delaying closing of folder '%s' (%d) for %d seconds.",
              mf->GetName().c_str(), mf->GetNRef(), secs);

   m_mf = mf;

   // keep it for now
   m_mf->IncRef();

   m_expires = secs != NEVER_EXPIRES;
   m_timeout = secs;

   ResetTimeout();

#ifdef DEBUG_FOLDER_CLOSE
   m_deleting = false;
#endif // DEBUG_FOLDER_CLOSE
}

MfCloseEntry::~MfCloseEntry()
{
   if ( m_mf )
   {
      wxLogTrace(TRACE_MF_CLOSE, "Really closing mailfolder '%s' (%d)",
                 m_mf->GetName().c_str(), m_mf->GetNRef());

#ifdef DEBUG_FOLDER_CLOSE
      m_deleting = true;
#endif // DEBUG_FOLDER_CLOSE

      m_mf->RealDecRef();
   }
}

// restart the expire timer
void MfCloseEntry::ResetTimeout()
{
   wxLogTrace(TRACE_MF_CLOSE, "Reset close timeout for folder '%s' (%d)",
              m_mf->GetName().c_str(), m_mf->GetNRef());

   if ( m_expires )
   {
      m_dt = wxDateTime::Now() + wxTimeSpan::Seconds(m_timeout);
   }
   //else: no need to (re)set timeout which never expires, leave m_dt as is
}

// ----------------------------------------------------------------------------
// MfCloser
// ----------------------------------------------------------------------------

MfCloser::MfCloser()
        : m_MfList(true),
          m_timer(this)
{
   m_interval = READ_APPCONFIG(MP_FOLDER_CLOSE_DELAY);

   // start it, in fact
   RestartTimer();
}

MfCloser::~MfCloser()
{
   // we don't want to get any more timer events, we're shutting down
   m_timer.Stop();

   // close all folders
   CleanUp();
}

void MfCloser::Add(MailFolderCmn *mf, int delay)
{
#ifdef DEBUG_FOLDER_CLOSE
   // this shouldn't happen as we only add the folder to the list when
   // it is about to be deleted and it can't be deleted if we're holding
   // to it (MfCloseEntry has a lock), so this would be an error in ref
   // counting (extra DecRef()) elsewhere
   ASSERT_MSG( !GetCloseEntry(mf),
               "adding a folder to MfCloser twice??" );
#endif // DEBUG_FOLDER_CLOSE

   CHECK_RET( mf, "NULL MailFolder in MfCloser::Add()");
   m_MfList.push_back(new MfCloseEntry(mf, delay));

   if ( delay < m_interval )
   {
      // restart the timer using smaller interval - of course, normally we
      // should compute the pgcd of all intervals but let's keep it simple
      m_interval = delay;

      RestartTimer();
   }
}

void MfCloser::OnTimer(void)
{
   MfList::iterator i;
   for( i = m_MfList.begin(); i != m_MfList.end();)
   {
      MfCloseEntry *entry = *i;
      if ( entry->HasExpired() )
      {
         i = m_MfList.erase(i);
      }
      else
      {
         ++i;
      }
   }
}

void MfCloser::CleanUp(void)
{
   MfList::iterator i = m_MfList.begin();
   while (i != m_MfList.end() )
      i = m_MfList.erase(i);
}

MfCloseEntry *MfCloser::GetCloseEntry(MailFolderCmn *mf) const
{
   for ( MfList::iterator i = m_MfList.begin(); i != m_MfList.end(); i++ )
   {
      MfCloseEntry *entry = *i;
      if ( entry->Matches(mf) )
      {
         return entry;
      }
   }

   return NULL;
}

void MfCloser::RestartTimer()
{
   if ( m_timer.IsRunning() )
      m_timer.Stop();

   // delay is in seconds, we need ms here
   m_timer.Start(m_interval * 1000);
}

void MfCloseTimer::Notify(void)
{
   m_mfCloser->OnTimer();
}

// ----------------------------------------------------------------------------
// MailFolderCmn folder closing
// ----------------------------------------------------------------------------

bool
MailFolderCmn::DecRef()
{
   // don't keep folders alive artificially if we're going to terminate soon
   // anyhow
   if ( gs_MailFolderCloser && !mApplication->IsShuttingDown() )
   {
      switch ( GetNRef() )
      {
         case 1:
            // the folder is going to be really closed if this is the last
            // DecRef(), delay closing of the folder after checking that it
            // was opened successfully and is atill functional
            if ( IsAlive() )
            {
               int delay;

               if ( GetFlags() & MF_FLAGS_KEEPOPEN )
               {
                  // never close it at all
                  delay = MfCloseEntry::NEVER_EXPIRES;
               }
               else
               {
                  // don't close it only if the linger delay is set
                  delay = READ_CONFIG(GetProfile(), MP_FOLDER_CLOSE_DELAY);
               }

               if ( delay > 0 )
               {
                  Checkpoint(); // flush data immediately

                  gs_MailFolderCloser->Add(this, delay);
               }
            }
            break;

         case 2:
            // if the folder is already in gs_MailFolderCloser list the
            // remaining lock is hold by it and will be removed when the
            // timeout expires, but we want to reset the timeout as the folder
            // was just accessed and so we want to keep it around longer
            {
               MfCloseEntry *entry = gs_MailFolderCloser->GetCloseEntry(this);
               if ( entry )
               {
                  // restart the expire timer
                  entry->ResetTimeout();
               }
            }
            break;
      }
   }

   return RealDecRef();
}

#ifdef DEBUG_FOLDER_CLOSE

void
MailFolderCmn::IncRef()
{
   wxLogTrace(TRACE_MF_REF, "MF(%s)::IncRef(): %u -> %u",
              GetName().c_str(), m_nRef, m_nRef + 1);

   MObjectRC::IncRef();
}

#endif // DEBUG_FOLDER_CLOSE

bool
MailFolderCmn::RealDecRef()
{
#ifdef DEBUG_FOLDER_CLOSE
   wxLogTrace(TRACE_MF_REF, "MF(%s)::DecRef(): %u -> %u",
              GetName().c_str(), m_nRef, m_nRef - 1);

   // check that the folder is not in the mail folder closer list any more if
   // we're going to delete it
   if ( m_nRef == 1 )
   {
      if ( gs_MailFolderCloser && gs_MailFolderCloser->GetCloseEntry(this) )
      {
         // we will crash later when removing it from the list!
         FAIL_MSG("deleting folder still in MfCloser list!");
      }
   }
#endif // DEBUG_FOLDER_CLOSE

   return MObjectRC::DecRef();
}

// ----------------------------------------------------------------------------
// MailFolderCmn creation/destruction
// ----------------------------------------------------------------------------

MailFolderCmn::MailFolderCmn()
{
#ifdef DEBUG_FOLDER_CLOSE
   m_PreCloseCalled = false;
#endif // DEBUG_FOLDER_CLOSE

   // We need to know if we are building the first folder listing ever
   // or not, to suppress NewMail events.
   m_FirstListing = true;
   m_ProgressDialog = NULL;
   m_Timer = new MailFolderTimer(this);
   m_LastNewMsgUId = UID_ILLEGAL;

   m_MEventReceiver = new MfCmnEventReceiver(this);
}

MailFolderCmn::~MailFolderCmn()
{
#ifdef DEBUG_FOLDER_CLOSE
   ASSERT(m_PreCloseCalled == true);
#endif // DEBUG_FOLDER_CLOSE

   delete m_Timer;
   delete m_MEventReceiver;
}

void
MailFolderCmn::PreClose(void)
{
   if ( m_Timer )
      m_Timer->Stop();

#ifdef DEBUG_FOLDER_CLOSE
   m_PreCloseCalled = true;
#endif // DEBUG_FOLDER_CLOSE
}

// ----------------------------------------------------------------------------
// MailFolderCmn message saving
// ----------------------------------------------------------------------------

// TODO: the functions below should share at least some common code instead of
//       duplicating it! (VZ)

bool
MailFolderCmn::SaveMessagesToFile(const UIdArray *selections,
                                  const String& fileName0,
                                  MWindow *parent)
{
   String fileName = fileName0;

   if ( fileName.empty() )
   {
      // ask the user
      fileName = wxPFileSelector
                 (
                  "MsgSave",
                  _("Choose file to save message to"),
                  NULL, NULL, NULL,
                  _("All files (*.*)|*.*"),
                  wxSAVE | wxOVERWRITE_PROMPT,
                  parent
                 );

      if ( fileName.IsEmpty() )
      {
         // cancelled by user
         return false;
      }
   }
   else // not empty, use this file name
   {
      fileName = strutil_expandpath(fileName0);
   }

   // truncate the file
   wxFile file;
   if ( !file.Create(fileName, TRUE /* overwrite */) )
   {
      wxLogError(_("Could not truncate the existing file."));
      return false;
   }

   // save the messages
   int n = selections->Count();

   MProgressDialog *pd = NULL;
   int threshold = GetProfile() ?
      READ_CONFIG(GetProfile(), MP_FOLDERPROGRESS_THRESHOLD)
      : MP_FOLDERPROGRESS_THRESHOLD_D;
   if ( threshold > 0 && n > threshold )
   {
      wxString msg;
      msg.Printf(_("Saving %d messages to the file '%s'..."),
                 n, fileName0.empty() ? fileName.c_str() : fileName0.c_str());

      pd = new MProgressDialog(GetName(), msg, 2*n, NULL);
   }

   bool rc = true;
   String tmpstr;
   for ( int i = 0; i < n; i++ )
   {
      Message_obj msg = GetMessage((*selections)[i]);

      if ( msg )
      {
         if ( pd )
            pd->Update( 2*i + 1 );

         // iterate over all parts
         int numParts = msg->CountParts();
         for ( int part = 0; part < numParts; part++ )
         {
            size_t size = msg->GetPartSize(part);
            if ( size == 0 )
            {
               // skip empty parts
               continue;
            }

            int partType = msg->GetPartType(part);
            if ( (partType == Message::MSG_TYPETEXT) ||
                 (partType == Message::MSG_TYPEMESSAGE ))
            {
               const char *cptr = msg->GetPartContent(part);
               if( !cptr )
               {
                  FAIL_MSG( "failed to get the content of a text psrt?" );

                  continue;
               }

               tmpstr = strutil_enforceNativeCRLF(cptr);
               if ( file.Write(tmpstr, tmpstr.length()) != size )
               {
                  rc = false;
               }
            }
         }

         if ( pd )
            pd->Update( 2*i + 2);
      }
   }

   delete pd;

   return rc;
}

bool
MailFolderCmn::SaveMessages(const UIdArray *selections,
                            MFolder *folder)
{
   CHECK( folder, false, "SaveMessages() needs a valid folder pointer" );

   if ( !CanCreateMessagesInFolder(folder->GetType()) )
   {
      // normally, this should be checked by GUI code, but if it doesn't,
      // detect it here
      wxLogError(_("Impossible to copy messages in the folder '%s'.\n"
                   "You can't create messages in the folders of this type."),
                 folder->GetFullName().c_str());
      return false;
   }

   int n = selections->Count();
   CHECK( n, true, "SaveMessages(): nothing to save" );

   MailFolder *mf = MailFolder::OpenFolder(folder);
   if ( !mf )
   {
      String msg;
      msg.Printf(_("Cannot save messages to folder '%s'."),
                 folder->GetFullName().c_str());
      ERRORMESSAGE((msg));
      return false;
   }

   if ( mf->IsLocked() )
   {
      FAIL_MSG( "Can't SaveMessages() to locked folder" );

      mf->DecRef();

      return false;
   }

   MProgressDialog *pd = NULL;
   int threshold = mf->GetProfile() ?
      READ_CONFIG(mf->GetProfile(), MP_FOLDERPROGRESS_THRESHOLD)
      : MP_FOLDERPROGRESS_THRESHOLD_D;

   if ( threshold > 0 && n > threshold )
   {
      // open a progress window:
      wxString msg;
      msg.Printf(_("Saving %d messages to the folder '%s'..."),
                 n, folder->GetName().c_str());

      pd = new MProgressDialog(mf->GetName(), msg, 2*n, NULL);
   }

   bool rc = true;
   for ( int i = 0; i < n; i++ )
   {
      Message *msg = GetMessage((*selections)[i]);
      if(msg)
      {
         if(pd)
            pd->Update( 2*i + 1 );
         rc &= mf->AppendMessage(*msg, FALSE);
         if(pd)
            pd->Update( 2*i + 2);

         msg->DecRef();
      }
   }

   mf->Ping(); // with our flags
   mf->DecRef();

   delete pd;

   return rc;
}

bool
MailFolderCmn::SaveMessages(const UIdArray *selections,
                            const String& folderName)
{
   MFolder_obj folder(folderName);
   if ( !folder.IsOk() )
   {
      wxLogError(_("Impossible to save messages to not existing folder '%s'."),
                 folderName.c_str());

      return false;
   }

   return SaveMessages(selections, folder);
}

// ----------------------------------------------------------------------------
// MailFolderCmn message replying/forwarding
// ----------------------------------------------------------------------------

void
MailFolderCmn::ReplyMessages(const UIdArray *selections,
                             const MailFolder::Params& params,
                             MWindow *parent)
{
   int n = selections->Count();
   for( int i = 0; i < n; i++ )
   {
      Message *msg = GetMessage((*selections)[i]);
      if ( msg )
      {
         ReplyMessage(msg, params, GetProfile(), parent);
         msg->DecRef();
      }
   }
}


void
MailFolderCmn::ForwardMessages(const UIdArray *selections,
                               const MailFolder::Params& params,
                               MWindow *parent)
{
   int i;
   Message *msg;

   int n = selections->Count();
   for(i = 0; i < n; i++)
   {
      msg = GetMessage((*selections)[i]);
      ForwardMessage(msg, params, GetProfile(), parent);
      msg->DecRef();
   }
}

// ----------------------------------------------------------------------------
// MailFolderCmn sorting
// ----------------------------------------------------------------------------

// we can only sort one listing at a time currently as we use global data to
// pass information to the C callback used by qsort() - if this proves to be a
// significant limitation, we should reimplement sorting ourselves
static struct SortData
{
   // the struct must be locked while in use
   MMutex mutex;

   // the sort order to use
   long order;

   // the listing which we're sorting
   HeaderInfoList *hil;
} gs_SortData;

// return negative number if a < b, 0 if a == b and positive number if a > b
#define CmpNumeric(a, b) ((a)-(b))

static int CompareStatus(int stat1, int stat2)
{
   // deleted messages are considered to be less important than undeleted ones
   if ( stat1 & MailFolder::MSG_STAT_DELETED )
   {
      if ( ! (stat2 & MailFolder::MSG_STAT_DELETED ) )
         return -1;
   }
   else if ( stat2 & MailFolder::MSG_STAT_DELETED )
   {
      return 1;
   }

   /*
      We use a scoring system:

      recent = +1
      unseen   = +1
      answered = -1

      (by now, messages are either both deleted or neither one is)
   */

   int
      score1 = 0,
      score2 = 0;

   if(stat1 & MailFolder::MSG_STAT_RECENT)
      score1 += 1;
   if( !(stat1 & MailFolder::MSG_STAT_SEEN) )
      score1 += 1;
   if( stat1 & MailFolder::MSG_STAT_ANSWERED )
      score1 -= 1;

   if(stat2 & MailFolder::MSG_STAT_RECENT)
      score2 += 1;
   if( !(stat2 & MailFolder::MSG_STAT_SEEN) )
      score2 += 1;
   if( stat2 & MailFolder::MSG_STAT_ANSWERED )
      score2 -= 1;

   return CmpNumeric(score1, score2);
}

extern "C"
{
   static int ComparisonFunction(const void *p1, const void *p2)
   {
      // check that the caller didn't forget to acquire the global data lock
      ASSERT_MSG( gs_SortData.mutex.IsLocked(), "using unlocked gs_SortData" );

      size_t n1 = *(size_t *)p1,
             n2 = *(size_t *)p2;

      HeaderInfo *i1 = gs_SortData.hil->GetItemByIndex(n1),
                 *i2 = gs_SortData.hil->GetItemByIndex(n2);

      // copy it as we're going to modify it while processing
      long sortOrder = gs_SortData.order;

      int result = 0;
      while ( result == 0 && sortOrder != 0 )
      {
         long criterium = sortOrder & 0xF;
         sortOrder >>= 4;

         // we rely on MessageSortOrder values being what they are: as _REV
         // version immediately follows the normal order constant, we should
         // reverse the comparison result for odd values of MSO_XXX
         int reverse = criterium % 2;
         switch ( criterium - reverse )
         {
            case MSO_NONE:
               break;

            case MSO_DATE:
               result = CmpNumeric(i1->GetDate(), i2->GetDate());
               break;

            case MSO_SUBJECT:
               {
                  String
                     subj1 = strutil_removeReplyPrefix(i1->GetSubject()),
                     subj2 = strutil_removeReplyPrefix(i2->GetSubject());

                  result = Stricmp(subj1, subj2);
               }
               break;

            case MSO_AUTHOR:
               result = Stricmp(i1->GetFrom(), i2->GetFrom());
               break;

            case MSO_STATUS:
               result = CompareStatus(i1->GetStatus(), i2->GetStatus());
               break;

            case MSO_SIZE:
               result = CmpNumeric(i1->GetSize(), i2->GetSize());
               break;

            case MSO_SCORE:
               result = CmpNumeric(i1->GetScore(), i2->GetScore());
               break;

            default:
               FAIL_MSG("unknown sorting criterium");
         }

         if ( reverse )
         {
            if ( criterium == MSO_NONE )
            {
               // special case: result is 0 for MSO_NONE but -1 (reversed order)
               // for MSO_NONE_REV
               result = -1;
            }
            else // for all other cases just revert the result value
            {
               result = -result;
            }
         }
      }

      return result;
   }
}

// ----------------------------------------------------------------------------
// MailFolderCmn threading
// ----------------------------------------------------------------------------

/*
   FIXME:

   1. we should use server side threading when possible
   2. this algorithm is O(N^2) and awfully inefficient in its using of
      Set/GetIndentation() for its own temporary data, we should use JWZ
      algorithm for threading instead, it gives better results and is more
      efficient!
 */

#define MFCMN_INDENT1_MARKER   ((size_t)-1)
#define MFCMN_INDENT2_MARKER   ((size_t)-2)

KBLIST_DEFINE(SizeTList, size_t);

static void AddDependents(size_t &idx,
                          int level,
                          int i,
                          size_t indices[],
                          size_t indents[],
                          HeaderInfoList *hilp,
                          SizeTList dependents[])
{
   SizeTList::iterator it;

   for(it = dependents[i].begin(); it != dependents[i].end(); it++)
   {
      size_t idxToAdd = **it;
      // a non-zero index means, this one has been added already
      if((*hilp)[idxToAdd]->GetIndentation() != MFCMN_INDENT1_MARKER)
      {
         ASSERT(idx < (*hilp).Count());
         indices[idx] = idxToAdd;
         indents[idx] = level;
         (*hilp)[idxToAdd]->SetIndentation(MFCMN_INDENT1_MARKER);
         idx++;
         // after adding an element, we need to check if we have to add
         // any of its own dependents:
         AddDependents(idx, level+1, idxToAdd, indices, indents, hilp, dependents);
      }
   }
}

static void ThreadMessages(MailFolder *mf, HeaderInfoList *hilp)
{
   CHECK_RET( hilp, "no listing to thread" );

   HeaderInfoList_obj hil(hilp);

   size_t count = hil->Count();

   if ( count <= 1 )
   {
      // nothing to be done: one message is never threaded
      return;
   }

   MFrame *frame = mf->GetInteractiveFrame();
   if ( frame )
   {
      wxLogStatus(frame, _("Threading %u messages..."), count);
   }

   // reset indentation first
   size_t i;
   for ( i = 0; i < count; i++ )
   {
      hil[i]->SetIndentation(0);
   }

   // We need a list of dependent messages for each entry.
   SizeTList *dependents = new SizeTList[count];

   for ( i = 0; i < count; i++ )
   {
      String id = hil[i]->GetId();

      if ( !id.empty() ) // no Id lines in Outbox!
      {
         for ( size_t j = 0; j < count; j++ )
         {
            String references = hil[j]->GetReferences();
            if ( references.Find(id) != wxNOT_FOUND )
            {
               dependents[i].push_back(new size_t(j));
               hil[j]->SetIndentation(MFCMN_INDENT2_MARKER);
            }
         }
      }
   }

   /* Now we have all the dependents set up properly and can re-build
      the list. We now build an array of indices for the new list.
   */

   size_t *indices = new size_t[count];
   size_t *indents = new size_t[count];
   memset(indents, 0, count*sizeof(size_t));

   size_t idx = 0; // where to store next entry
   for (i = 0; i < count; i++ )
   {
      // we mark used indices with a non-0 indentation, so only the top level
      // messages still have 0 indentation
      if ( hil[i]->GetIndentation() == 0 )
      {
         ASSERT(idx < (*hilp).Count());

         indices[idx++] = i;
         hil[i]->SetIndentation(MFCMN_INDENT1_MARKER);
         AddDependents(idx, 1, i, indices, indents, hilp, dependents);
      }
   }

   // we should have found all the messages
   ASSERT_MSG( idx == hilp->Count(), "logic error in threading code" );

   // we use AddTranslationTable() instead of SetTranslationTable() as this one
   // should be combined with the existing table from sorting
   hilp->AddTranslationTable(indices);

   for ( i = 0; i < hilp->Count(); i++ )
   {
      hil[i]->SetIndentation(indents[i]);
   }

   delete [] indices;
   delete [] indents;
   delete [] dependents;

   if ( frame )
   {
      wxLogStatus(frame, _("Threading %lu messages... done."), count);
   }
}

static void SortListing(MailFolder *mf, HeaderInfoList *hil, long sortOrder)
{
   CHECK_RET( hil, "no listing to sort" );

   // don't sort the listing if we don't have any sort criterium (so sorting
   // "by arrival order" will be much faster!)
   if ( sortOrder != 0 )
   {
      size_t count = hil->Count();
      if ( count >= 1 )
      {
         MFrame *frame = mf->GetInteractiveFrame();
         if ( frame )
         {
            wxLogStatus(frame, _("Sorting %u messages..."), count);
         }

         MLocker lock(gs_SortData.mutex);
         gs_SortData.order = sortOrder;
         gs_SortData.hil = hil;

         // start with unsorted listing
         size_t *transTable = new size_t[count];
         for ( size_t n = 0; n < count; n++ )
         {
            transTable[n] = n;
         }

         // now sort it
         qsort(transTable, count, sizeof(size_t), ComparisonFunction);

         // and tell the listing to use it (it will delete it)
         hil->SetTranslationTable(transTable);

         // just in case
         gs_SortData.hil = NULL;

         if ( frame )
         {
            wxLogStatus(frame, _("Sorting %u messages... done."), count);
         }
      }
      //else: avoid sorting empty listing or listing of 1 element
   }
   //else: configured not to do any sorting

   hil->DecRef();
}

// ----------------------------------------------------------------------------
// MailFolderCmn new mail check
// ----------------------------------------------------------------------------

/*
  This function is called by GetHeaders() immediately after building a
  new folder listing. It checks for new mails and, if required, sends
  out new mail events.
*/
void
MailFolderCmn::CheckForNewMail(HeaderInfoList *hilp)
{
   CHECK_RET( hilp, "no listing in CheckForNewMail" );

   UIdType n = hilp->Count();
   if ( !n )
      return;

   UIdType *messageIDs = new UIdType[n];

   wxLogTrace(TRACE_NEWMAIL,
              "CheckForNewMail(): folder: %s highest seen uid: %lu.",
              GetName().c_str(), (unsigned long) m_LastNewMsgUId);

   // new messages are supposed to have UIDs greater than the last new message
   // seen, but not all messages with greater UID are new, so we have to first
   // find all messages with such UIDs and then check if they're really new

   // when we check for the new mail the first time, m_LastNewMsgUId is still
   // invalid and so we must reset it before comparing with it
   bool firstTime = m_LastNewMsgUId == UID_ILLEGAL;
   if ( firstTime )
      m_LastNewMsgUId = 0;

   // Find the new messages:
   UIdType nextIdx = 0;
   UIdType highestId = m_LastNewMsgUId;
   for ( UIdType i = 0; i < n; i++ )
   {
      HeaderInfo *hi = hilp->GetItemByIndex(i);
      UIdType uid = hi->GetUId();
      if ( uid > m_LastNewMsgUId )
      {
         if ( IsNewMessage(hi) )
         {
            messageIDs[nextIdx++] = uid;
         }

         if ( uid > highestId )
            highestId = uid;
      }
   }

   ASSERT_MSG( nextIdx <= n, "more new messages than total?" );

   if ( nextIdx != 0)
   {
      MEventManager::Send(new MEventNewMailData(this, nextIdx, messageIDs));
   }
   //else: no new messages found

   m_LastNewMsgUId = highestId;

   wxLogTrace(TRACE_NEWMAIL,
              "CheckForNewMail() after test: folder: %s highest seen uid: %lu.",
              GetName().c_str(), (unsigned long) highestId);


   delete [] messageIDs;
}


/* This will do the work in the new mechanism:

   - For now it only sorts or threads the headerinfo list
 */
void
MailFolderCmn::ProcessHeaderListing(HeaderInfoList *hilp)
{
   CHECK_RET( hilp, "no listing to process" );

   hilp->IncRef();
   SortListing(this, hilp, m_Config.m_ListingSortOrder);

   if ( m_Config.m_UseThreading )
   {
      hilp->IncRef();
      ThreadMessages(this, hilp);
   }
   else // reset indentation as it may have been set by threading code before
   {
      size_t count = hilp->Count();
      for ( size_t i = 0; i < count; i++ )
      {
         (*hilp)[i]->SetIndentation(0);
      }
   }
}

// ----------------------------------------------------------------------------
// MailFolderCmn options handling
// ----------------------------------------------------------------------------

void
MailFolderCmn::OnOptionsChange(MEventOptionsChangeData::ChangeKind kind)
{
   /*
      We want to avoid rebuilding the listing unnecessary (it is an expensive
      operation) so we only do it if something really changed for us.
   */

   MFCmnOptions config;
   ReadConfig(config);
   if ( config != m_Config )
   {
      m_Config = config;

      DoUpdate();

      // listing has been resorted/rethreaded
      RequestUpdate();
   }
}

void
MailFolderCmn::UpdateConfig(void)
{
   ReadConfig(m_Config);

   DoUpdate();
}

void
MailFolderCmn::DoUpdate()
{
   int interval = m_Config.m_UpdateInterval * 1000;
   if ( interval != m_Timer->GetInterval() )
   {
      m_Timer->Stop();

      if ( interval > 0 ) // interval of zero == disable ping timer
         m_Timer->Start(interval);
   }

   if ( HasHeaders() )
   {
      HeaderInfoList *hil = GetHeaders();
      if ( hil )
      {
         ProcessHeaderListing(hil);
         hil->DecRef();
      }
   }
}

void
MailFolderCmn::ReadConfig(MailFolderCmn::MFCmnOptions& config)
{
   Profile *profile = GetProfile();
   config.m_ListingSortOrder = READ_CONFIG(profile, MP_MSGS_SORTBY);
   config.m_ReSortOnChange = READ_CONFIG(profile, MP_MSGS_RESORT_ON_CHANGE) != 0;
   config.m_UpdateInterval = READ_CONFIG(profile, MP_UPDATEINTERVAL);
   config.m_UseThreading = READ_CONFIG(profile, MP_MSGS_USE_THREADING) != 0;
}

// ----------------------------------------------------------------------------
// MailFolderCmn message deletion
// ----------------------------------------------------------------------------

bool
MailFolderCmn::UnDeleteMessages(const UIdArray *selections)
{
   int n = selections->Count();
   int i;
   bool rc = true;
   for(i = 0; i < n; i++)
      rc &= UnDeleteMessage((*selections)[i]);
   return rc;
}


/** Delete a message.
    @param uid mesage uid
    @return true if ok
    */
bool
MailFolderCmn::DeleteMessage(unsigned long uid)
{
   UIdArray a;
   a.Add(uid);

   // delete without expunging
   return DeleteMessages(&a);
}


bool
MailFolderCmn::DeleteOrTrashMessages(const UIdArray *selections)
{
   CHECK( CanDeleteMessagesInFolder(GetType()), FALSE,
          "can't delete messages in this folder" );

   // we can either "really delete" the messages (in fact, just mark them as
   // deleted) or move them to trash which implies deleting and expunging them
   bool reallyDelete = !READ_CONFIG(GetProfile(), MP_USE_TRASH_FOLDER);

   // If we are the trash folder, we *really* delete
   wxString trashFolderName = READ_CONFIG(GetProfile(), MP_TRASH_FOLDER);
   if ( !reallyDelete && GetName() == trashFolderName )
      reallyDelete = true;

   bool rc;
   if ( reallyDelete )
   {
      // delete without expunging
      rc = DeleteMessages(selections, FALSE);
   }
   else // move to trash
   {
      rc = SaveMessages(selections, trashFolderName);
      if ( rc )
      {
         // delete and expunge
         rc = DeleteMessages(selections, TRUE);
      }
   }

   return rc;
}

bool
MailFolderCmn::DeleteMessages(const UIdArray *selections, bool expunge)
{
   String seq = GetSequenceString(selections);
   if ( seq.empty() )
   {
      // nothing to do
      return true;
   }

   bool rc = SetSequenceFlag(seq, MailFolder::MSG_STAT_DELETED);
   if ( rc && expunge )
      ExpungeMessages();

   return rc;
}

// ----------------------------------------------------------------------------
// MailFolderCmn filtering
// ----------------------------------------------------------------------------

/// apply the filters to the selected messages
int
MailFolderCmn::ApplyFilterRules(UIdArray msgs)
{
   // Has the folder got any filter rules set? Concat all its filters together
   String filterString;

   MFolder_obj folder(GetName());
   wxArrayString filters = folder->GetFilters();
   size_t count = filters.GetCount();

   for ( size_t n = 0; n < count; n++ )
   {
      MFilter_obj filter(filters[n]);
      MFilterDesc fd = filter->GetDesc();
      filterString += fd.GetRule();
   }

   int rc = 0;
   if ( !filterString.empty() )
   {
      // Obtain pointer to the filtering module:
      MModule_Filters *filterModule = MModule_Filters::GetModule();

      if ( filterModule )
      {
         FilterRule *filterRule = filterModule->GetFilter(filterString);
         if ( filterRule )
         {
            wxBusyCursor busyCursor;

            // don't ignore deleted messages here (hence "false")
            rc = filterRule->Apply(this, msgs, false);

            filterRule->DecRef();
         }
         else
         {
            wxLogWarning(_("Error in filter code for folder '%s', "
                           "filters not applied"),
                         filterString.c_str());

            rc = FilterRule::Error;
         }

         filterModule->DecRef();
      }
      else // no filter module
      {
         wxLogWarning(_("Filter module couldn't be loaded, "
                        "filters not applied"));

         rc = FilterRule::Error;
      }
   }
   else // no filters to apply
   {
      wxLogVerbose(_("No filters configured for this folder."));
   }

   return rc;
}

/** Checks for new mail and filters if necessary.
    Returns FilterRule flag combination
*/
int
MailFolderCmn::FilterNewMail(HeaderInfoList *hil)
{
   CHECK(hil, 0, "FilterNewMail() needs header info list");

   int rc = 0;

   // Maybe we are lucky and have nothing to do?
   size_t count = hil->Count();
   if ( count > 0 )
   {
      UIdType nRecent = CountRecentMessages();
      if ( nRecent > 0 )
      {
         // Obtain pointer to the filtering module:
         MModule_Filters *filterModule = MModule_Filters::GetModule();
         if(filterModule)
         {
            // Build an array of NEW message UIds to apply the filters to:
            UIdArray messages;
            for ( size_t idx = 0; idx < count; idx++ )
            {
               HeaderInfo *hi = hil->GetItemByIndex(idx);
               int status = hi->GetStatus();

               // only take NEW (== RECENT && !SEEN) and ignore DELETED
               if ( (status & 
                     (MSG_STAT_RECENT | MSG_STAT_SEEN | MSG_STAT_DELETED))
                     == MSG_STAT_RECENT )
               {
                  messages.Add(hi->GetUId());
               }
            }

            // build a single program from all filter rules:
            String filterString;
            MFolder_obj folder(GetName());
            wxArrayString filters;
            if ( folder )
               filters = folder->GetFilters();
            size_t countFilters = filters.GetCount();
            for ( size_t n = 0; n < countFilters; n++ )
            {
               MFilter_obj filter(filters[n]);
               MFilterDesc fd = filter->GetDesc();
               filterString += fd.GetRule();
            }
            if(filterString[0]) // not empty
            {
               // translate filter program:
               FilterRule * filterRule = filterModule->GetFilter(filterString);
               // apply it:
               if ( filterRule )
               {
                  // true here means "ignore deleted"
                  rc |= filterRule->Apply(this, messages, true);

                  filterRule->DecRef();
               }
            }
            filterModule->DecRef();
         }
         else
         {
            wxLogVerbose("Filter module couldn't be loaded.");

            rc = FilterRule::Error;
         }
      }

      // do we have any messages to move?
      if ( (GetFlags() & MF_FLAGS_INCOMING) != 0)
      {
         // where to we move the mails?
         String newMailFolder = READ_CONFIG(GetProfile(), MP_NEWMAIL_FOLDER);
         if ( newMailFolder == GetName() )
         {
            ERRORMESSAGE((_("Cannot collect mail from folder '%s' into itself."),
                         GetName().c_str()));
         }
         else // ok, move to another folder
         {
            UIdArray messages;
            for ( size_t idx = 0; idx < count; idx++ )
            {
               // check that the message isn't deleted - avoids getting 2 (or
               // more!) copies of the same "new" message (first normal,
               // subsequent deleted) if, for whatever reason, we failed to
               // expunge the messages the last time we collected mail from here
               HeaderInfo *hi = hil->GetItemByIndex(idx);
               if ( !(hi->GetStatus() & MSG_STAT_DELETED) )
               {
                  messages.Add(hi->GetUId());
               }
            }

            if ( SaveMessages(&messages, newMailFolder) )
            {
               // delete and expunge
               DeleteMessages(&messages, true);

               rc |= FilterRule::Expunged;
            }
            else // don't delete them if we failed to move
            {
               ERRORMESSAGE((_("Cannot move newly arrived messages.")));
            }
         }
      }
   }

   hil->DecRef();

   return rc;
}

// ----------------------------------------------------------------------------
// eliminating duplicate messages code
// ----------------------------------------------------------------------------

// VZ: disabling this stuff - it is a workaround for the bug which doesn't exist
//     any more
#if 0
struct Dup_MsgInfo
{
   Dup_MsgInfo(const String &s, UIdType id, unsigned long size)
      { m_Id = s; m_UId = id; m_Size = size; }
   String m_Id;
   UIdType m_UId;
   unsigned long m_Size;
};

KBLIST_DEFINE(Dup_MsgInfoList, Dup_MsgInfo);

UIdType
MailFolderCmn::DeleteDuplicates()
{
   HeaderInfoList *hil = GetHeaders();
   if(! hil)
      return 0;

   Dup_MsgInfoList mlist;

   UIdArray toDelete;

   for(size_t idx = 0; idx < hil->Count(); idx++)
   {
      String   id = (*hil)[idx]->GetId();
      UIdType uid = (*hil)[idx]->GetUId();
      size_t size = (*hil)[idx]->GetSize();
      bool found = FALSE;
      for(Dup_MsgInfoList::iterator i = mlist.begin();
          i != mlist.end();
          i++)
         if( (**i).m_Id == id )
         {
            /// if new message is larger, keep it instead
            if( (**i).m_Size < size )
            {
               toDelete.Add((**i).m_UId);
               (**i).m_UId = uid;
               (**i).m_Size = size;
               found = FALSE;
            }
            else
               found = TRUE;
            break;
         }
      if(found)
         toDelete.Add(uid);
      else
      {
         Dup_MsgInfo *mi = new Dup_MsgInfo(id, uid, size);
         mlist.push_back(mi);
      }
   }
   hil->DecRef();

   if(toDelete.Count() == 0)
      return 0; // nothing to do

   if(DeleteMessages(&toDelete,FALSE))
      return toDelete.Count();
   // else - uncommented or compiler thinks there's return without value
   return UID_ILLEGAL; // an error happened
}
#endif // 0

// ----------------------------------------------------------------------------
// MailFolderCmn message counting
// ----------------------------------------------------------------------------

bool MailFolderCmn::CountInterestingMessages(MailFolderStatus *status) const
{
   String name = GetName();
   MfStatusCache *mfStatusCache = MfStatusCache::Get();
   if ( !mfStatusCache->GetStatus(name, status) )
   {
      if ( !DoCountMessages(status) )
         return false;

      mfStatusCache->UpdateStatus(name, *status);
   }

   return true;
}

// ----------------------------------------------------------------------------
// MfCmnEventReceiver
// ----------------------------------------------------------------------------

MfCmnEventReceiver::MfCmnEventReceiver(MailFolderCmn *mf)
{
   m_Mf = mf;

   m_MEventCookie = MEventManager::Register(*this, MEventId_OptionsChange);
   ASSERT_MSG( m_MEventCookie, "can't reg folder with event manager");

   m_MEventPingCookie = MEventManager::Register(*this, MEventId_Ping);
   ASSERT_MSG( m_MEventPingCookie, "can't reg folder with event manager");
}

MfCmnEventReceiver::~MfCmnEventReceiver()
{
   MEventManager::Deregister(m_MEventCookie);
   MEventManager::Deregister(m_MEventPingCookie);
}

bool MfCmnEventReceiver::OnMEvent(MEventData& event)
{
   if ( event.GetId() == MEventId_Ping )
   {
      m_Mf->Ping();
   }
   else // options change
   {
      // do update - but only if this event is for us
      MEventOptionsChangeData& data = (MEventOptionsChangeData&)event;
      if ( data.GetProfile()->IsAncestor(m_Mf->GetProfile()) )
         m_Mf->OnOptionsChange(data.GetChangeKind());

      // also update close timer in case timeout changed
      if ( gs_MailFolderCloser )
      {
         gs_MailFolderCloser->RestartTimer();
      }
   }

   return true;
}

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

extern bool MailFolderCmnInit()
{
   if ( !gs_MailFolderCloser )
   {
      gs_MailFolderCloser = new MfCloser;
   }

   return true;
}

extern void MailFolderCmnCleanup()
{
   if ( gs_MailFolderCloser )
   {
      // any MailFolderCmn::DecRef() shouldn't add folders to
      // gs_MailFolderCloser from now on, so NULL it immediately
      MfCloser *mfCloser = gs_MailFolderCloser;
      gs_MailFolderCloser = NULL;

      delete mfCloser;
   }
}

