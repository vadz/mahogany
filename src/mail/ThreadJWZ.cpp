//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/ThreadJWZ.cpp: implementation of Jamie Zawinski threading
//              algorithm
// Purpose:     implements JWZThreadMessages function used for local threading
// Author:      Xavier Nodet
// Modified by:
// Created:     04.09.01 (extracted from mail/MailFolderCmn.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Mahogany team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #include "Threading.h"

   #include "Mcclient.h"         // for hash_table
#endif // USE_PCH

#include "HeaderInfo.h"

#if wxUSE_REGEX
   #include <wx/regex.h>
#endif // wxUSE_REGEX

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#define MFCMN_INDENT1_MARKER   ((size_t)-1)
#define MFCMN_INDENT2_MARKER   ((size_t)-2)

/// Used for wxLogTrace calls
#define TRACE_JWZ "jwz"

//---------------
// JWZ algorithm: see http://www.jwz.org/doc/threading.html
//---------------

// --------------------------------------------------------------------
// Threadable
// --------------------------------------------------------------------

/** This class is used as an interface to threadable object. In our
    case, some HeaderInfo. The result of the JWZ algorithm will be to
    compute the index that the message will have in the display, and
    its indentation.
    */
class Threadable {
private:
   /// The message that this instance abstracts
   HeaderInfo *m_hi;
   /** Sequence number of the messages in the sorted list that is
       the input of the algorithm.
       */
   size_t      m_indexInHilp;
   /// Resulting sequence number of the messages
   size_t      m_threadedIndex;
   /// Resulting indentation of the message
   size_t      m_indent;
   /// Is this a dummy message
   bool        m_dummy;
   /// Next brother of the message
   Threadable *m_next;
   /// First child of the message
   Threadable *m_child;
#if defined(DEBUG)
   String      m_subject;
   String      m_refs;
   String      m_id;
#endif
   String     *m_simplifiedSubject;
   bool        m_isReply;
public:
   Threadable(HeaderInfo *hi, size_t index = 0, bool dummy = false);
   ~Threadable();

   void destroy();

   Threadable *makeDummy() {
      return new Threadable(m_hi, m_indexInHilp, true);
   }
   int isDummy() const { return m_dummy; }
   size_t getIndex() { return m_indexInHilp; }
   size_t getThreadedIndex() { return m_threadedIndex; }
   size_t getIndent() { return m_indent; }
   void setThreadedIndex(size_t i) { m_threadedIndex = i; }
   void setIndent(size_t i) { m_indent = i; }

   Threadable *getNext() const { return m_next; }
   Threadable *getChild() const { return m_child; }
   void setNext(Threadable* x) { m_next = x; }
   void setChild(Threadable* x) { m_child = x; }

   String messageThreadID() const;
   kbStringList *messageThreadReferences() const;

#if wxUSE_REGEX
   String getSimplifiedSubject(wxRegEx *replyRemover,
                               const String &replacementString) const;
   bool subjectIsReply(wxRegEx *replyRemover,
                       const String &replacementString) const;
#else // wxUSE_REGEX
   String getSimplifiedSubject(bool removeListPrefix) const;
   bool subjectIsReply(bool removeListPrefix) const;
#endif // wxUSE_REGEX
};

inline
Threadable::Threadable(HeaderInfo *hi, size_t index, bool dummy)
   : m_hi(hi)
   , m_indexInHilp(index)
   , m_threadedIndex(0)
   , m_indent(0)
   , m_dummy(dummy)
   , m_next(0)
   , m_child(0)
   , m_simplifiedSubject(0)
   , m_isReply(false)
{
#if defined(DEBUG)
   m_subject = hi->GetSubject();
   m_refs = hi->GetReferences();
   m_id = hi->GetId();
#endif
}

inline
Threadable::~Threadable()
{
   delete m_simplifiedSubject;
}


void Threadable::destroy() {
   static size_t depth = 0;
   CHECK_RET(depth < 1000, "Deep recursion in Threadable::destroy()");
   if (m_child != 0)
   {
      depth++;
      m_child->destroy();
      depth--;
      CHECK_RET(depth >= 0, "Negative recursion depth in Threadable::destroy()");
   }
   delete m_child;
   m_child = 0;
   if (m_next != 0)
      m_next->destroy();
   delete m_next;
   m_next = 0;
}

String Threadable::messageThreadID() const
{
   String id = m_hi->GetId();
   if (!id.empty())
      return id;
   else
      return String("<emptyId:") << (int)this << ">";
}


// --------------------------------------------------------------------
// Computing the list of references to use
// --------------------------------------------------------------------

kbStringList *Threadable::messageThreadReferences() const
{
   // To build a seemingly correct list of references would be
   // quite easy if the References header could be trusted blindly.
   // But this is real life...
   //
   // So we add together the References and In-Reply-To headers, and
   // then remove duplicate references (keping the last of the two) so that:
   //
   //  - If one header is empty, the other is used.
   //
   //  - We circumvent Eudora's behaviour, where the content of In-Reply-To
   //    is not repeated in References.
   //
   //  - If the content of References is somewhat screwed-up, the last
   //    reference in the list will be the one given in In-Reply-To, which,
   //    when present, seems to be more reliable (no ordering problem).
   //
   // XNOFIXME:
   // One still standing problem is if there are conflicting References
   // headers between messages. In this case, the winner is he first one
   // seen by the algorithm. The problem is that the input order of the
   // messages in the algorithm depends on the sorting options chosen by
   // the user. Thus, the output of the algorithm depends on external
   // factors...
   //
   // As In-Reply-To headers are more reliable, but carry less information
   // (they do not allow to reconstruct threads when some messages are
   // missing), one possible way would be to make two passes: one with
   // only In-Reply-To, and then one with References.

   kbStringList *tmp = new kbStringList;
   String refs = m_hi->GetReferences() + m_hi->GetInReplyTo();
   if (refs.empty())
      return tmp;

   const char *s = refs.c_str();

   enum State {
      notInRef,
      openingSeen,
      atSeen
   } state = notInRef;

   size_t start = 0;
   size_t nbChar = refs.Len();
   size_t i = 0;
   while (i < nbChar)
   {
      char current = s[i];
      switch (state)
      {
      case notInRef:
         if (current == '<')
         {
            start = i;
            state = openingSeen;
         }
         break;
      case openingSeen:
         if (current == '@')
            state = atSeen;
         break;
      case atSeen:
         if (current == '>')
         {
            // We found a reference
            String *ref = new String(refs.Mid(start, i+1-start));
            // In case of duplicated reference we keep the last one:
            // It is important that In-Reply-To is the last reference
            // in the list.
            kbStringList::iterator i;
            for (i = tmp->begin(); i != tmp->end(); i++)
            {
               String *previous = *i;
               if (*previous == *ref)
               {
                  tmp->erase(i);
                  break;
               }
            }
            tmp->push_back(ref);
            state = notInRef;
         }
      }
      i++;
   }
   return tmp;
}

#if !wxUSE_REGEX

// Removes the list prefix [ListName] at the beginning
// of the line (and the spaces around the result)
//
static
String RemoveListPrefix(const String &subject)
{
   String s = subject;
   if (s.empty())
      return s;
   if (s[0] != '[')
      return s;
   s = s.AfterFirst(']');
   s = s.Trim(false);
   return s;
}

#endif // !wxUSE_REGEX

// XNOFIXME: put this code in strutil.cpp when JWZ
// becomes the default algo.
//
// Removes all occurences of Re: Re[n]: Re(n):
// without considering the case, and remove unneeded
// spaces (start, end, or multiple consecutive spaces).
// Reply prefixes are removed only at the start of the
// string or just after a list prefix (i.e. [ListName]).
//
#if !wxUSE_REGEX
String
strutil_removeAllReplyPrefixes(const String &isubject,
                               bool &replyPrefixSeen)
{
   enum State {
      notInPrefix,
      inListPrefix,
      rSeen,
      eSeen,
      parenSeen,
      bracketSeen,
      closingSeen,
      colonSeen,
      becomeGarbage,
      garbage
   } state = notInPrefix;

   String subject = isubject;
   size_t length = subject.Len();
   size_t startIndex = 0;
   char *s = new char[length+1];
   size_t nextPos = 0;
   bool listPrefixAlreadySeen = false;
   replyPrefixSeen = false;
   for (size_t i = 0; subject[i] != '\0'; i++)
   {
      if (state == garbage)
         startIndex = i;

      char c = subject[i];
      switch (state)
      {
      case notInPrefix:
         if (c == 'r' || c == 'R')
         {
            state = rSeen;
            startIndex = i;
         } else if (c == ' ')
            startIndex = i+1;
         else if (!listPrefixAlreadySeen && c == '[')
         {
            s[nextPos++] = c;
            state = inListPrefix;
            listPrefixAlreadySeen = true;
         }
         else
            state = becomeGarbage;
         break;
      case inListPrefix:
         s[nextPos++] = c;
         if (c == ']')
         {
            state = notInPrefix;
            // notInPrefix will skip the next space, but we want it
            s[nextPos++] = ' ';
         }
         break;
      case rSeen:
         if (c == 'e' || c == 'E')
            state = eSeen;
         else
            state = becomeGarbage;
         break;
      case eSeen:
         if (c == ':')
            state = colonSeen;
         else if (c == '[')
            state = bracketSeen;
         else if (c == '(')
            state = parenSeen;
         else
            state = becomeGarbage;
         break;
      case parenSeen:
      case bracketSeen:
         if (c == (state == parenSeen ? ')' : ']'))
            state = closingSeen;
         else if (!isdigit(c))
            state = becomeGarbage;
         break;
      case closingSeen:
         if (c == ':')
            state = colonSeen;
         else
            state = becomeGarbage;
         break;
      case colonSeen:
         if (c == ' ')
         {
            replyPrefixSeen = true;
            state = notInPrefix;
            startIndex = i+1;
         }
         else
            state = becomeGarbage;
         break;
      case garbage:
         for (size_t j = startIndex; j <= i; ++j)
            s[nextPos++] = subject[j];
         if (c == ' ') {
            //state = notInPrefix;
            //startIndex = i+1;
         }
         break;
      }

      if (state == becomeGarbage)
      {
         for (size_t j = startIndex; j <= i; ++j)
            s[nextPos++] = subject[j];
         state = garbage;
      }
   }

   s[nextPos] = '\0';
   if (nextPos > 0 && s[nextPos-1] == ' ')
      s[nextPos-1] = '\0';
   String result(s);
   delete [] s;
   return result;
}
#endif // WX_HAVE_REGEX

#if wxUSE_REGEX
String Threadable::getSimplifiedSubject(wxRegEx *replyRemover,
                                        const String &replacementString) const
#else
String Threadable::getSimplifiedSubject(bool removeListPrefix) const
#endif
{
   // First, we remove all reply prefixes occuring near the beginning
   // of the subject (by near, we mean that a list prefix is ignored,
   // and we remove all reply prefixes at the beginning of the resulting
   // string):
   //   Re: Test              ->  Test
   //   Re: [M-Dev] Test      ->  [M-Dev] Test
   //   [M-Dev] Re: Test      ->  [M-Dev] Test
   //   [M-Dev] Re: Re: Test  ->  [M-Dev] Test
   //   Re: [M-Dev] Re: Test  ->  [M-Dev] Test
   //
   // And then, if asked for, we remove the list prefix.
   // This is handy for case where the two messages
   //   Test
   // and
   //   Re: [m-developpers] Test
   // must be threaded together.
   //
   // BTW, we save the results so that they do not have to
   // be computed again
   //
   Threadable *that = (Threadable *)this;    // Remove constness
   if (that->m_simplifiedSubject != 0)
      return *that->m_simplifiedSubject;
   that->m_simplifiedSubject = new String;
#if wxUSE_REGEX
   *(that->m_simplifiedSubject) = m_hi->GetSubject();
   if (!replyRemover)
   {
      that->m_isReply = false;
      return *that->m_simplifiedSubject;
   }
   size_t len = that->m_simplifiedSubject->Len();
   if (replyRemover->Replace(that->m_simplifiedSubject, replacementString, 1))
      that->m_isReply = (that->m_simplifiedSubject->Len() != len);
   else
      that->m_isReply = 0;
#else // wxUSE_REGEX
   *that->m_simplifiedSubject =
      strutil_removeAllReplyPrefixes(m_hi->GetSubject(),
                                     that->m_isReply);
   if (removeListPrefix)
      *that->m_simplifiedSubject = RemoveListPrefix(*that->m_simplifiedSubject);
#endif // wxUSE_REGEX
   return *that->m_simplifiedSubject;
}


#if wxUSE_REGEX
bool Threadable::subjectIsReply(wxRegEx *replyRemover,
                                const String &replacementString) const
#else
bool Threadable::subjectIsReply(bool x) const
#endif
{
   Threadable *that = (Threadable *)this;    // Remove constness
   if (that->m_simplifiedSubject == 0)
      getSimplifiedSubject(replyRemover, replacementString);
   return that->m_isReply;
}



// --------------------------------------------------------------------
// ThreadContainer
// --------------------------------------------------------------------

#if defined(DEBUG)
// Count of the number of instances alive
// Should be 0 at the end of the threading
static int NumberOfThreadContainers = 0;
#endif



class ThreadContainer {
private:
   Threadable      *m_threadable;
   ThreadContainer *m_parent;
   ThreadContainer *m_child;
   ThreadContainer *m_next;

public:
   ThreadContainer(Threadable *th = 0);
   ~ThreadContainer();

   Threadable *getThreadable() const { return m_threadable; }
   ThreadContainer *getParent() const { return m_parent; }
   ThreadContainer *getChild() const { return m_child; }
   ThreadContainer *getNext() const { return m_next; }
   void setThreadable(Threadable *x) { m_threadable = x; }
   void setParent(ThreadContainer *x) { m_parent = x; }
   void setChild(ThreadContainer *x) { m_child = x; }
   void setNext(ThreadContainer *x) { m_next = x; }

   /** Returns the index to be used to compare instances
       of ThreadContainer to determine their ordering
       */
   size_t getIndex() const;
   /** Adds a ThreadContainer to the children of the caller,
       taking their index into account.
       */
   void addAsChild(ThreadContainer *c);

   /** Returns true if target is a children (maybe deep in the
       tree) of the caller.
       */
   bool findChild(ThreadContainer *target, bool withNexts = false) const;

   /// Recursively reverses the order of the the caller's children
   void reverseChildren();

   /** Computes and saves (in the Threadable they store) the indentation
       and the numbering of the messages
       */
   void flush(size_t &threadedIndex, size_t indent, bool indentIfDummyNode);

   /// Recursively destroys a tree
   void destroy();
};


inline
ThreadContainer::ThreadContainer(Threadable *th)
   : m_threadable(th)
   , m_parent(0)
   , m_child(0)
   , m_next(0)
{
#if defined(DEBUG)
   NumberOfThreadContainers++;
#endif
}


inline
ThreadContainer::~ThreadContainer()
{
   //if (m_threadable != NULL && m_threadable->isDummy())
   //   delete m_threadable;
#if defined(DEBUG)
   NumberOfThreadContainers--;
#endif
}


size_t ThreadContainer::getIndex() const
{
   const size_t foolish = 1000000000;
   static size_t depth = 0;
   CHECK(depth < 1000, foolish,
         "Deep recursion in ThreadContainer::getSmallestIndex()");
   Threadable *th = getThreadable();
   if (th != 0)
      return th->getIndex();
   else
   {
      if (getChild() != 0)
      {
         depth++;
         size_t index = getChild()->getIndex();
         depth--;
         CHECK(depth >= 0, foolish,
               "Negative recursion depth in ThreadContainer::getSmallestIndex()");
         return index;
      }
      else
         return foolish;
   }
}


void ThreadContainer::addAsChild(ThreadContainer *c)
{
   // Takes into account the indices so that the new kid
   // is inserted in the correct position
   CHECK_RET(c->getThreadable(), "No threadable in ThreadContainer::addAsChild()");
   size_t cIndex = c->getThreadable()->getIndex();
   ThreadContainer *prev = 0;
   ThreadContainer *current = getChild();
   while (current != 0)
   {
      if (current->getIndex() > cIndex)
         break;
      prev = current;
      current = current->getNext();
   }
   c->setParent(this);
   c->setNext(current);
   if (prev != 0)
      prev->setNext(c);
   else
      setChild(c);
}


bool ThreadContainer::findChild(ThreadContainer *target, bool withNexts) const
{
   static size_t depth = 0;
   CHECK(depth < 1000, true, "Deep recursion in ThreadContainer::findChild()");
   if (m_child == target)
      return true;
   if (withNexts && m_next == target)
      return true;
   if (m_child != 0)
   {
      depth++;
      bool found = m_child->findChild(target, true);
      depth--;
      CHECK(depth >= 0, true, "Negative recursion depth in ThreadContainer::findChild()");
      if (found)
         return true;
   }
   if (withNexts && m_next != 0)
   {
      bool found = m_next->findChild(target, true);
      if (found)
         return true;
   }
   return false;
}


void ThreadContainer::reverseChildren()
{
   static size_t depth = 0;
   CHECK_RET(depth < 1000, "Deep recursion in ThreadContainer::reverseChildren()");

   if (m_child != 0)
   {
      ThreadContainer *kid, *prev, *rest;
      for (prev = 0, kid = m_child, rest = kid->getNext();
           kid != 0;
           prev = kid, kid = rest, rest = (rest == 0 ? 0 : rest->getNext()))
         kid->setNext(prev);
      m_child = prev;

      depth++;
      for (kid = m_child; kid != 0; kid = kid->getNext())
         kid->reverseChildren();
      depth--;
      CHECK_RET(depth >= 0, "Negative recursion depth in ThreadContainer::reverseChildren()");
   }
}


void ThreadContainer::flush(size_t &threadedIndex, size_t indent, bool indentIfDummyNode)
{
   static size_t depth = 0;
   CHECK_RET(depth < 1000, "Deep recursion in ThreadContainer::flush()");
   CHECK_RET(m_threadable != 0, "No threadable in ThreadContainer::flush()");
   m_threadable->setChild(m_child == 0 ? 0 : m_child->getThreadable());
   if (!m_threadable->isDummy())
   {
      m_threadable->setThreadedIndex(threadedIndex++);
      m_threadable->setIndent(indent);
   }
   if (m_child != 0)
   {
      depth++;
      if (indentIfDummyNode)
         m_child->flush(threadedIndex, indent+1, indentIfDummyNode);
      else
         m_child->flush(threadedIndex,
                        indent+(m_threadable->isDummy() ? 0 : 1),
                        indentIfDummyNode);
      depth--;
      CHECK_RET(depth >= 0, "Negative recursion depth in ThreadContainer::flush()");
   }
   if (m_threadable != 0)
      m_threadable->setNext(m_next == 0 ? 0 : m_next->getThreadable());
   if (m_next != 0)
      m_next->flush(threadedIndex, indent, indentIfDummyNode);
}


void ThreadContainer::destroy()
{
   static size_t depth = 0;
   CHECK_RET(depth < 1000, "Deep recursion in ThreadContainer::destroy()");
   if (m_child != 0)
   {
      depth++;
      m_child->destroy();
      depth--;
      CHECK_RET(depth >= 0, "Negative recursion depth in ThreadContainer::destroy()");
   }
   delete m_child;
   m_child = 0;
   if (m_next != 0)
      m_next->destroy();
   delete m_next;
   m_next = 0;
}




// --------------------------------------------------------------------
// Threader : a JWZ threader
// --------------------------------------------------------------------

class Threader {
private:
   ThreadContainer *m_root;
   HASHTAB         *m_idTable;
   int              m_bogusIdCount;

   // Options
   bool m_gatherSubjects;
   wxRegEx *m_replyRemover;
   String m_replacementString;
   bool m_breakThreadsOnSubjectChange;
   bool m_indentIfDummyNode;
   bool m_removeListPrefixGathering;
   bool m_removeListPrefixBreaking;

public:
   Threader()
      : m_root(0)
      , m_idTable(0)
      , m_bogusIdCount(0)
      , m_gatherSubjects(true)
      , m_replyRemover(0)
      , m_replacementString()
      , m_breakThreadsOnSubjectChange(true)
      , m_indentIfDummyNode(false)
      , m_removeListPrefixGathering(true)
      , m_removeListPrefixBreaking(true)
   {}

   ~Threader()
   {
      delete m_replyRemover;
   }

   /** Does all the job. Input is a list of messages, output
       is a dummy root message, with all the tree under.

       If shouldGatherSubjects is true, all messages that
       have the same non-empty subject will be considered
       to be part of the same thread.

       If indentIfDummyNode is true, messages under a dummy
       node will be indented.
       */
   Threadable *thread(Threadable *threadableRoot);

   void setGatherSubjects(bool x) { m_gatherSubjects = x; }

#if wxUSE_REGEX
   void setSimplifyingRegex(const String &x)
   {
      if (!m_replyRemover)
         m_replyRemover = new wxRegEx(x);
      else
         m_replyRemover->Compile(x);
   }
#else // !wxUSE_REGEX
   void setRemoveListPrefixGathering(bool x) { m_removeListPrefixGathering = x; }
   void setRemoveListPrefixBreaking(bool x) { m_removeListPrefixBreaking = x; }
#endif // wxUSE_REGEX/!wxUSE_REGEX

   void setReplacementString(const String &x) { m_replacementString = x; }
   void setBreakThreadsOnSubjectChange(bool x) { m_breakThreadsOnSubjectChange = x; }
   void setIndentIfDummyNodet(bool x) { m_indentIfDummyNode = x; }

private:
   void buildContainer(Threadable *threadable);
   void findRootSet();
   void pruneEmptyContainers(ThreadContainer *parent,
                             bool fromBreakThreads);
   size_t collectSubjects(HASHTAB*, ThreadContainer *, bool recursive);
   void gatherSubjects();
   void breakThreads(ThreadContainer*);

   // An API on top of the hash-table
   HASHTAB *create(size_t) const;
   ThreadContainer* lookUp(HASHTAB*, const String&) const;
   void add(HASHTAB*, const String&, ThreadContainer*) const;
   void destroy(HASHTAB **) const;
};





static bool SeemsPrime(size_t n)
{
   if (n % 2 == 0)
      return false;
   if (n % 3 == 0)
      return false;
   if (n % 5 == 0)
      return false;
   if (n % 7 == 0)
      return false;
   if (n % 11 == 0)
      return false;
   if (n % 13 == 0)
      return false;
   if (n % 17 == 0)
      return false;
   if (n % 19 == 0)
      return false;
   return true;
}


static size_t FindPrime(size_t n)
{
   if (n % 2 == 0)
      n++;
   while (!SeemsPrime(n))
      n += 2;
   return n;
}


HASHTAB *Threader::create(size_t size) const
{
   // Find a correct size for an hash-table
   size_t aPrime = FindPrime(size);
   // Create it
   return hash_create(aPrime);
}


void Threader::add(HASHTAB *hTable, const String &str, ThreadContainer *container) const
{
   // As I do not want to ensure that the String instance given
   // as key will last at least as long as the table, I copy its
   // value.
   char *s = new char[str.Len()+1];
   strcpy(s, str.c_str());
   hash_add(hTable, s, container, 0);
}


ThreadContainer *Threader::lookUp(HASHTAB *hTable, const String &s) const
{
   void** data = hash_lookup(hTable, (char*) s.c_str());
   if (data != 0)
      return (ThreadContainer*)data[0];
   else
      return 0;
}


void Threader::destroy(HASHTAB ** hTable) const
{
   // Delete all the strings that have been used as key
   HASHENT *ent;
   size_t i;
   for (i = 0; i < (*hTable)->size; i++)
      for (ent = (*hTable)->table[i]; ent != 0; ent = ent->next)
         delete [] ent->name;
      // and destroy the hash-table
      hash_destroy(hTable);
}


Threadable *Threader::thread(Threadable *threadableRoot)
{
   if (threadableRoot == 0)
      return 0;

#if defined(DEBUG)
   // In case we leaked in a previous run of the algo, reset the count
   NumberOfThreadContainers = 0;
#endif

   // Count the number of messages to thread, and verify
   // the input list
   Threadable *th = threadableRoot;
   size_t thCount = 0;
   for (; th != 0; th = th->getNext())
   {
      VERIFY(th->getChild() == 0, "Bad input list in Threader::thread()");
      thCount++;
   }
   // Make an hash-table big enough to hold all the instances of
   // ThreadContainer that will be created during the run.
   // Note that some dummy containers will be created also, so
   // don't be shy...
   m_idTable = create(thCount*2);

   // Scan the list, and build the corresponding ThreadContainers
   // This step will:
   //  - Build a ThreadContainer for the Threadable given as argument
   //  - Scan the references of the message, and build on the fly some
   //    ThreadContainer for them, if the corresponding messages have not
   //    already been processed
   //  - Store all those containers in the hash-table
   //  - Build their parent/child relations
   for (th = threadableRoot; th != 0; th = th->getNext())
   {
      VERIFY(th->getIndex() >= 0, "Negative index in Threader::thread()");
      VERIFY(th->getIndex() < thCount, "Too big in Threader::thread()");
      // As things are now, dummy messages won't get past the algorithm
      // and won't be displayed. Thus we should not get them back when
      // re-threading this folder.
      // When this changes, we will have to take care to skip them.
      VERIFY(!th->isDummy(), "Dummy message in Threader::thread()");
      buildContainer(th);
   }

   // Create a dummy root ThreadContainer (stored in m_root) and
   // scan the content of the hash-table to make all parent-less
   // containers (i.e. first message in a thread) some children of
   // this root.
   findRootSet();

   // We are finished with this hash-table
   destroy(&m_idTable);

   // Remove all the useless containers (e.g. those that are not
   // in the root-set, have no message, but have a child)
   pruneEmptyContainers(m_root, false);

   // If asked to, gather all the messages that have the same
   // non-empty subjet in the same thread.
   if (m_gatherSubjects)
      gatherSubjects();

   ASSERT(m_root->getNext() == 0); // root node has a next ?!

   // Build dummy messages for the nodes that have no message.
   // Those can only appear in the root set.
   ThreadContainer *thr;
   for (thr = m_root->getChild(); thr != 0; thr = thr->getNext())
      if (thr->getThreadable() == 0)
         thr->setThreadable(thr->getChild()->getThreadable()->makeDummy());

   if (m_breakThreadsOnSubjectChange)
      breakThreads(m_root);

   // Prepare the result to be returned: the root of the
   // *Threadable* tree that we will build by flushing the
   // ThreadContainer tree structure.
   Threadable *result = (m_root->getChild() == 0
      ? 0
      : m_root->getChild()->getThreadable());

   // Compute the index of each message (the line it will be
   // displayed to when threaded) and its indentation.
   size_t threadedIndex = 0;
   if (m_root->getChild() != 0)
      m_root->getChild()->flush(threadedIndex, 0, m_indentIfDummyNode);

   // Destroy the ThreadContainer structure.
   m_root->destroy();
   delete m_root;
   ASSERT(NumberOfThreadContainers == 0); // Some ThreadContainers leak

   return result;
}



void Threader::buildContainer(Threadable *th)
{
   String id = th->messageThreadID();
   ASSERT(!id.empty());
   ThreadContainer *container = lookUp(m_idTable, id);

   if (container != 0)
   {
      if (container->getThreadable() != 0)
      {
         id = String("<bogusId:") << m_bogusIdCount++ << ">";
         container = 0;
      } else {
         container->setThreadable(th);
      }
   }

   if (container == 0)
   {
      container = new ThreadContainer(th);
      add(m_idTable, id, container);
   }

   ThreadContainer *parentRefCont = 0;
   kbStringList *refs = th->messageThreadReferences();
   kbStringList::iterator i;
   for (i = refs->begin(); i != refs->end(); i++)
   {
      String *ref = *i;
      ThreadContainer *refCont = lookUp(m_idTable, *ref);
      if (refCont == 0)
      {
         refCont = new ThreadContainer();
         add(m_idTable, *ref, refCont);
      }

      if ((parentRefCont != 0) &&
          (refCont->getParent() == 0) &&
          (parentRefCont != refCont) &&
          !parentRefCont->findChild(refCont))
      {
         if (!refCont->findChild(parentRefCont))
         {
            refCont->setParent(parentRefCont);
            refCont->setNext(parentRefCont->getChild());
            parentRefCont->setChild(refCont);
         } else
         {
            // refCont should be a child of parentRefCont (because
            // refCont is seens in the references after parentRefCont
            // but refCont is the parent of parentRefCont.
            // This must be because some References fields contradict
            // each other about the order. Let's keep the first
            // encountered.

            // VZ: commenting this out to silence a warning, what did
            //     you really mean, Xavier? (FIXME)
            //int i = 0;
         }
      }
      parentRefCont = refCont;
   }
   if ((parentRefCont != 0) &&
       ((parentRefCont == container) ||
        container->findChild(parentRefCont)))
      parentRefCont = 0;

   if (container->getParent() != 0) {
      ThreadContainer *rest, *prev;
      for (prev = 0, rest = container->getParent()->getChild();
           rest != 0;
           prev = rest, rest = rest->getNext())
         if (rest == container)
            break;
         ASSERT(rest != 0); // Did not find a container in parent
         if (prev == 0)
            container->getParent()->setChild(container->getNext());
         else
            prev->setNext(container->getNext());
         container->setNext(0);
         container->setParent(0);
   }

   if (parentRefCont != 0)
   {
      ASSERT(!container->findChild(parentRefCont));
      container->setParent(parentRefCont);
      container->setNext(parentRefCont->getChild());
      parentRefCont->setChild(container);
   }
   delete refs;
   //wxLogTrace(TRACE_JWZ, "Leaving Threader::buildContainer(Threadable 0x%lx)", th);
}


void Threader::findRootSet()
{
   wxLogTrace(TRACE_JWZ, "Entering Threader::findRootSet()");
   m_root = new ThreadContainer();
   HASHENT *ent;
   size_t i;
   for (i = 0; i < m_idTable->size; i++)
      for (ent = m_idTable->table[i]; ent != 0; ent = ent->next) {
         ThreadContainer *container = (ThreadContainer*)ent->data[0];
         if (container->getParent() == 0)
         {
            // This one will be in the root set
            ASSERT(container->getNext() == 0);
            // Find the correct position to insert it, so that the
            // order given to us is respected
            ThreadContainer *c = m_root->getChild();
            if (c == 0)
            {
               // This is the first one found
               container->setNext(m_root->getChild());
               m_root->setChild(container);
            } else {
               size_t index = container->getIndex();
               ThreadContainer *prev = 0;
               while ((c != 0) &&
                  (c->getIndex() < index))
               {
                  prev = c;
                  c = c->getNext();
               }
               ASSERT(container->getNext() == 0);
               container->setNext(c);
               if (prev == 0)
               {
                  ASSERT(m_root->getChild() == c);
                  m_root->setChild(container);
               } else
               {
                  ASSERT(prev->getNext() == c);
                  prev->setNext(container);
               }
            }
         }
      }
}


void Threader::pruneEmptyContainers(ThreadContainer *parent,
                                    bool fromBreakThreads)
{
   ThreadContainer *c, *prev, *next;
   for (prev = 0, c = parent->getChild(), next = c->getNext();
   c != 0;
   prev = c, c = next, next = (c == 0 ? 0 : c->getNext()))
   {
      if ((c->getThreadable() == 0 ||
           c->getThreadable()->isDummy()) &&
          (c->getChild() == 0))
      {
         if (prev == 0)
            parent->setChild(c->getNext());
         else
            prev->setNext(c->getNext());
         if (fromBreakThreads)
            delete c->getThreadable();
         delete c;
         c = prev;

      } else if ((c->getThreadable() == 0 ||
                  c->getThreadable()->isDummy()) &&
                 (c->getChild() != 0) &&
                 ((c->getParent() != 0) ||
                  (c->getChild()->getNext() == 0)))
      {
         ThreadContainer *kids = c->getChild();
         if (prev == 0)
            parent->setChild(kids);
         else
            prev->setNext(kids);

         ThreadContainer *tail;
         for (tail = kids; tail->getNext() != 0; tail = tail->getNext())
            tail->setParent(c->getParent());

         tail->setParent(c->getParent());
         tail->setNext(c->getNext());

         next = kids;

         if (fromBreakThreads)
            delete c->getThreadable();

         delete c;
         c = prev;

      } else if (c->getChild() != 0 && !fromBreakThreads)

         pruneEmptyContainers(c, false);
   }
}


// If one message in the root set has the same subject than another
// message (not necessarily in the root-set), merge them.
// This is so that messages which don't have Reference headers at all
// still get threaded (to the extent possible, at least.)
//
void Threader::gatherSubjects()
{
   size_t count = 0;
   ThreadContainer *c = m_root->getChild();
   for (; c != 0; c = c->getNext())
      count++;

   // Make the hash-table large enough. Let's consider
   // that there are not too many (not more than one per
   // thread) subject changes.
   HASHTAB *subjectTable = create(count*2);

   // Collect the subjects in all the tree
   count = collectSubjects(subjectTable, m_root, true);

   if (count == 0)            // If the table is empty, we're done.
   {
      destroy(&subjectTable);
      return;
   }

   // The sujectTable is now populated with one entry for each subject.
   // Now iterate over the root set, and gather together the difference.
   //
   ThreadContainer *prev, *rest;
   for (prev = 0, c = m_root->getChild(), rest = c->getNext();
        c != 0;
        prev = c, c = rest, rest = (rest == 0 ? 0 : rest->getNext()))
   {
      Threadable *th = c->getThreadable();
      if (th == 0)   // might be a dummy -- see above
      {
          th = c->getChild()->getThreadable();
          ASSERT(th != NULL);
      }

#if wxUSE_REGEX
      String subject = th->getSimplifiedSubject(m_replyRemover,
                                                m_replacementString);
#else // !wxUSE_REGEX
      String subject = th->getSimplifiedSubject(m_removeListPrefixGathering);
#endif // wxUSE_REGEX/!wxUSE_REGEX

      // Don't thread together all subjectless messages; let them dangle.
      if (subject.empty())
         continue;

      ThreadContainer *old = lookUp(subjectTable, subject);
      if (old == c)    // oops, that's us
         continue;

      // Ok, so now we have found another container in the root set with
      // the same subject. There are a few possibilities:
      //
      // - If both are dummies, append one's children to the other, and remove
      //   the now-empty container.
      //
      // - If one container is a dummy and the other is not, make the non-dummy
      //   one be a child of the dummy, and a sibling of the other "real"
      //   messages with the same subject (the dummy's children.)
      //
      // - If that container is a non-dummy, and that message's subject does
      //   not begin with "Re:", but *this* message's subject does, then make
      //   this be a child of the other.
      //
      // - If that container is a non-dummy, and that message's subject begins
      //   with "Re:", but *this* message's subject does *not*, then make that
      //   be a child of this one -- they were misordered. (This happens
      //   somewhat implicitely, since if there are two messages, one with "Re:"
      //   and one without, the one without will be in the hash-table,
      //   regardless of the order in which they were seen.)
      //
      // - If that container is in the root-set, make a new dummy container and
      //   make both messages be a child of it.  This catches the both-are-replies
      //   and neither-are-replies cases, and makes them be siblings instead of
      //   asserting a hierarchical relationship which might not be true.
      //
      // - Otherwise (that container is not in the root set), make this container
      //   be a sibling of that container.
      //

      // Remove the "second" message from the root set.
      if (prev == 0)
         m_root->setChild(c->getNext());
      else
         prev->setNext(c->getNext());
      c->setNext(0);

      if (old->getThreadable() == 0 && c->getThreadable() == 0)
      {
         // They're both dummies; merge them by
         // adding all the children of c to old
         ThreadContainer *kid = c->getChild();
         while (kid != NULL)
         {
            ThreadContainer *next = kid->getNext();
            old->addAsChild(kid);
            kid = next;
         }
         delete c;
      }
      else if (old->getThreadable() == 0 ||               // old is empty, or
               (c->getThreadable() != 0 &&
#if wxUSE_REGEX
               c->getThreadable()->subjectIsReply(m_replyRemover,
                                                  m_replacementString) &&   // c has "Re:"
                !old->getThreadable()->subjectIsReply(m_replyRemover,
                                                      m_replacementString))) // and old does not
#else // !wxUSE_REGEX
               c->getThreadable()->subjectIsReply(m_removeListPrefixGathering) &&   // c has "Re:"
                !old->getThreadable()->subjectIsReply(m_removeListPrefixGathering))) // and old does not
#endif // wxUSE_REGEX/!wxUSE_REGEX
      {
         // Make this message be a child of the other.
         old->addAsChild(c);
      }
      else
      {
         // old may be not in the root-set, as we searched for subjects
         // in all the tree. If this is the case, we must not create a dummy
         // node, but rather insert c as a sibling of old.

         if (old->getParent() == NULL)
         {
            // Make the old and new message be children of a new dummy container.
            // We do this by creating a new container object for old->msg and
            // transforming the old container into a dummy (by merely emptying it
            // so that the hash-table still points to the one that is at depth 0
            // instead of depth 1.

            ThreadContainer *newC = new ThreadContainer(old->getThreadable());
            newC->setChild(old->getChild());
            ThreadContainer *tail = newC->getChild();
            for (; tail != 0; tail = tail->getNext())
               tail->setParent(newC);

            old->setThreadable(0);
            old->setChild(0);
            c->setParent(old);
            newC->setParent(old);

            // Determine the one, between c and newC, that must be displayed
            // first, by considering their index. Reorder them so that c is the one
            // with smallest index (to be displayed first)
            if (c->getThreadable()->getIndex() >
               newC->getThreadable()->getIndex())
            {
               ThreadContainer *tmp = c;
               c = newC;
               newC = tmp;
            }

            // Make old point to its kids
            old->setChild(c);
            c->setNext(newC);
         }
         else
            old->getParent()->addAsChild(c);
      }

      // We've done a merge, so keep the same 'prev' next time around.
      c = prev;
   }

   destroy(&subjectTable);
}


size_t Threader::collectSubjects(HASHTAB *subjectTable,
                                 ThreadContainer *parent,
                                 bool recursive)
{
   static size_t depth = 0;
   VERIFY(depth < 1000, "Deep recursion in Threader::collectSubjects()");
   size_t count = 0;
   ThreadContainer *c;
   for (c = parent->getChild(); c != 0; c = c->getNext())
   {
      Threadable *cTh = c->getThreadable();
      Threadable *th = cTh;

      // If there is no threadable, this is a dummy node in the root set.
      // Only root set members may be dummies, and they always have at least
      // two kids. Take the first kid as representative of the subject.
      if (th == 0)
      {
         ASSERT(c->getParent() == 0);          // In root-set
         th = c->getChild()->getThreadable();
      }
      ASSERT(th != 0);

#if wxUSE_REGEX
      String subject = th->getSimplifiedSubject(m_replyRemover,
                                                m_replacementString);
#else // !wxUSE_REGEX
      String subject = th->getSimplifiedSubject(m_removeListPrefixGathering);
#endif // wxUSE_REGEX/!wxUSE_REGEX
      if (subject.empty())
         continue;

      ThreadContainer *old = lookUp(subjectTable, subject);

      // Add this container to the table if:
      //  - There is no container in the table with this subject, or
      //  - This one is a dummy container and the old one is not: the dummy
      //    one is more interesting as a root, so put it in the table instead
      //  - The container in the table has a "Re:" version of this subject,
      //    and this container has a non-"Re:" version of this subject.
      //    The non-"Re:" version is the more interesting of the two.
      //
      if (old == 0 ||
          (cTh == 0 && old->getThreadable() != 0) ||
          (old->getThreadable() != 0 &&
#if wxUSE_REGEX
           old->getThreadable()->subjectIsReply(m_replyRemover,
                                                m_replacementString) &&
#else // !wxUSE_REGEX
           old->getThreadable()->subjectIsReply(m_removeListPrefixGathering) &&
#endif // wxUSE_REGEX/!wxUSE_REGEX
           cTh != 0 &&
#if wxUSE_REGEX
           !cTh->subjectIsReply(m_replyRemover, m_replacementString)
#else // !wxUSE_REGEX
           !cTh->subjectIsReply(m_removeListPrefixGathering)
#endif // wxUSE_REGEX/!wxUSE_REGEX
         ))
      {
         // The hash-table we use does not offer to replace (nor remove)
         // an entry. But adding a new entry with the same key will 'hide'
         // the old one. This is just not as efficient if there are collisions
         // but we do not care for now.
         add(subjectTable, subject, c);
         count++;
      }

      if (recursive)
      {
         depth++;
         count += collectSubjects(subjectTable, c, true);
         depth--;
         VERIFY(depth >= 0, "Negative recursion depth in Threader::collectSubjects()");
      }
   }

   // Return number of subjects found
   return count;
}


void Threader::breakThreads(ThreadContainer* c)
{
   static size_t depth = 0;
   CHECK_RET(depth < 1000, "Deep recursion in Threader::breakThreads()");

   // Dummies should have been built
   CHECK_RET((c == m_root) || (c->getThreadable() != NULL),
             "No threadable in Threader::breakThreads()");

   // If there is no parent, there is no thread to break.
   if (c->getParent() != NULL)
   {
      // Dummies should have been built
      CHECK_RET(c->getParent()->getThreadable() != NULL,
                "No parent's threadable in Threader::breakThreads()");

      ThreadContainer *parent = c->getParent();

#if wxUSE_REGEX
      if (c->getThreadable()->getSimplifiedSubject(m_replyRemover,
                                                   m_replacementString) !=
          parent->getThreadable()->getSimplifiedSubject(m_replyRemover,
                                                        m_replacementString))
#else // !wxUSE_REGEX
      if (c->getThreadable()->getSimplifiedSubject(m_removeListPrefixGathering) !=
          parent->getThreadable()->getSimplifiedSubject(m_removeListPrefixGathering))
#endif // wxUSE_REGEX/!wxUSE_REGEX
      {
         // Subject changed. Break the thread
         if (parent->getChild() == c)
            parent->setChild(c->getNext());
         else
         {
            ThreadContainer *prev = parent->getChild();
            CHECK_RET(parent->findChild(c),
                      "Not child of its parent in Threader::breakThreads()");
            while (prev->getNext() != c)
               prev = prev->getNext();
            prev->setNext(c->getNext());
         }
         m_root->addAsChild(c);
         c->setParent(0);

         // If the parent was a dummy node and c has other siblings,
         // we should remove the parent.
         if (parent->getThreadable()->isDummy())
         {
            ThreadContainer *grandParent = parent->getParent();
            if (grandParent == NULL)
               grandParent = m_root;
            pruneEmptyContainers(grandParent, true);
         }
      }
   }

   ThreadContainer *kid = c->getChild();
   while (kid != NULL)
   {
      // We save the next to check, as kid may disappear of this thread
      ThreadContainer *next = kid->getNext();

      depth++;
      breakThreads(kid);
      depth--;
      VERIFY(depth >= 0, "Negative recursion depth in Threader::breakThreads()");

      kid = next;
   }
}

// Build the input struture for the threader: a list
// of all the messages to thread.
static Threadable *BuildThreadableList(const HeaderInfoList *hilp)
{
   wxLogTrace(TRACE_JWZ, "Entering BuildThreadableList");
   Threadable *root = 0;
   size_t count = hilp->Count();
   for (size_t i = 0; i < count; ++i) {
      HeaderInfo *hi = hilp->GetItem(i);
      Threadable *item = new Threadable(hi, i);
      item->setNext(root);
      root = item;
   }
   wxLogTrace(TRACE_JWZ, "Leaving BuildThreadableList");
   return root;
}


// Scan the Threadable tree and fill the arrays needed to
// correctly display the threads
//
static void FlushThreadable(Threadable *t,
                            MsgnoType *indices,
                            size_t *indents
#if defined(DEBUG)
                            , bool seen[]
#endif
                            )
{
   if (!t->isDummy())
   {
      size_t threadedIndex = t->getThreadedIndex();
      size_t index = t->getIndex();
      indices[threadedIndex] = index;
      indents[index] = t->getIndent();
#if defined(DEBUG)
      seen[index] = true;
#endif
   }
   if (t->getChild())
#if defined(DEBUG)
      FlushThreadable(t->getChild(), indices, indents, seen);
#else
      FlushThreadable(t->getChild(), indices, indents);
#endif
   if (t->getNext())
#if defined(DEBUG)
      FlushThreadable(t->getNext(), indices, indents, seen);
#else
   FlushThreadable(t->getNext(), indices, indents);
#endif
}

// ----------------------------------------------------------------------------
// our public API
// ----------------------------------------------------------------------------

extern void JWZThreadMessages(const ThreadParams& thrParams,
                              const HeaderInfoList *hilp,
                              ThreadData *thrData)
{
   MsgnoType *indices = thrData->m_tableThread;
   size_t *indents = thrData->m_indents;

   // reset indentation first
   memset(indents, 0, thrData->m_count * sizeof(size_t));

   wxLogTrace(TRACE_JWZ, "Entering BuildDependantsWithJWZ");
   Threadable *threadableRoot = BuildThreadableList(hilp);

   // FIXME: can't we just take count from thrData?
   size_t count = 0;
   Threadable *th = threadableRoot;
   for (; th != 0; th = th->getNext())
    count++;
   Threader *threader = new Threader;

   threader->setGatherSubjects(thrParams.gatherSubjects);
#if wxUSE_REGEX
   threader->setSimplifyingRegex(thrParams.simplifyingRegex);
   threader->setReplacementString(thrParams.replacementString);
#else // !wxUSE_REGEX
   threader->setRemoveListPrefixGathering(removeListPrefixGathering);
   threader->setRemoveListPrefixBreaking(removeListPrefixBreaking);
#endif // wxUSE_REGEX
   threader->setBreakThreadsOnSubjectChange(thrParams.breakThread);
   threader->setIndentIfDummyNodet(thrParams.indentIfDummyNode);

   threadableRoot = threader->thread(threadableRoot);

   if (threadableRoot != 0)
   {
#if defined(DEBUG)
      size_t i;
      bool *seen = new bool[count];
      for (i = 0; i < count; ++i)
         seen[i] = false;
      FlushThreadable(threadableRoot, indices, indents, seen);
      for (i = 0; i < count; ++i)
         ASSERT(seen[i]);
      for (i = 0; i < count; ++i)
         if (indices[i] > count)
         {
            indices[i] = 0;
            indents[i] = 0;
         }
      delete [] seen;

#else
      FlushThreadable(threadableRoot, indices, indents);
#endif
      threadableRoot->destroy();

      // compute the number of children for each message
      //
      // TODO: again, it surely may be done far simpler in the code above
      for ( i = 0; i < count; i++ )
      {
         // our children are all messages after this one with indent
         // strictly greater than ours
         size_t level = indents[i];
         size_t j;
         for ( j = i + 1; j < count; j++ )
         {
            if ( indents[j] <= level )
            {
               // not a child any more
               break;
            }
         }

         // save the number of children
         thrData->m_children[indices[i]] = j - i - 1;
      }

      // convert to msgnos from indices
      //
      // TODO: compute directly msgnos, not indices in the code above
      for ( i = 0; i < count; i++ )
      {
         indices[i]++;
      }
   }
   else
   {
      // It may be an error...
      for (size_t i = 0; i < count; ++i)
      {
         indices[i] = i;
         indents[i] = 0;
      }
   }

   delete threadableRoot;
   delete threader;
   wxLogTrace(TRACE_JWZ, "Leaving BuildDependantsWithJWZ");
}

