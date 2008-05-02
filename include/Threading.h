///////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany - cross platform e-mail GUI client
// File name:   Threading.h - sorting related constants and functions
// Purpose:     defines ThreadParams
// Author:      Vadim Zeitlin
// Modified by:
// Created:     04.09.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _THREADING_H_
#define _THREADING_H_

#ifndef  USE_PCH
#endif // USE_PCH

class Profile;
class HeaderInfoList;
class WXDLLIMPEXP_FWD_CORE wxWindow;

// THREADNODE is defined in <cclient.h> which may have been or not included
#ifndef THREADNODE
   #define THREADNODE struct thread_node
#endif

/// struct holding all threading parameters
struct ThreadParams
{
   /// do we thread at all?
   bool useThreading;

   /// do we use server side threading if available?
   bool useServer;

   /// do we use server side threading by references only (not by subject)?
   bool useServerByRefOnly;

   /// the strings to use to bring subject to canonical form
   String simplifyingRegex;
   String replacementString;

   /// Should we gather in same thread messages with same subject
   bool gatherSubjects;

   /// Should we break thread when subject changes
   bool breakThread;

   /// Should we indent messages with missing ancestor
   bool indentIfDummyNode;

   /// def ctor
   ThreadParams();

   /// read the sort params from profile
   void Read(Profile *profile);

   /// compare SortParams
   bool operator==(const ThreadParams& other) const;
   bool operator!=(const ThreadParams& other) const { return !(*this == other); }
};

/// the thread data as it is stored internally
struct ThreadData
{
   /// the number of messages in the arrays
   MsgnoType m_count;

   /// the translation table containing the msgnos in threaded order
   MsgnoType *m_tableThread;

   /// table containing the message indent in the thread
   size_t *m_indents;

   /// table containing the total number of children of each message
   MsgnoType *m_children;

   /// Root of the built tree, must be allocated with fs_get()!
   THREADNODE *m_root;

   /// free the existing tree, but not the other tables
   void killTree();


   /// ctor reserves memory for holding info about count messages
   ThreadData(MsgnoType count);

   /// dtor frees memory
   ~ThreadData();


   DECLARE_NO_COPY_CLASS(ThreadData);
};

/**
   The function which threads messages according to the JWZ algorithm

   @param thrParams specifies how to thread messages
   @param hil the headers to thread
   @param indices will contain the indices in the "thread" order on return
   @param indents will contain the indents of the messages on return
 */
extern void JWZThreadMessages(const ThreadParams& thrParams,
                              const HeaderInfoList *hil,
                              ThreadData *thrData);

/**
   Another function which threads messages according to the JWZ algorithm

   @param thrParams specifies how to thread messages
   @param hil the headers to thread
*/
extern THREADNODE* JWZThreadMessages(const ThreadParams& thrParams,
                                     const HeaderInfoList *hil);

                              /**
   Show the dialog to configure the message threading for the folder using this
   profile.

   @param profile to read/write settings from
   @param parent the parent window for the dialog
   @return true if anything was changed, false otherwise
 */
extern bool ConfigureThreading(Profile *profile, wxWindow *parent);

#endif // _THREADING_H_

