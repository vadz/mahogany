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

/* can't leave this or VC++ fails to compile because it is confused by ifdefs
   before the PCH inclusion, so uncomment this manually to test

// define this to test strutil_removeAllReplyPrefixes() function
#ifdef TEST_SUBJECT_NORMALIZE
   #include <string>

   using namespace std;
   #define String std::string
#else // !TEST_SUBJECT_NORMALIZE
*/

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #include "Threading.h"

   #include "Mcclient.h"         // for hash_table
#endif // USE_PCH

#include "strlist.h"
#include "HeaderInfo.h"

#if wxUSE_REGEX
  #if defined(JWZ_USE_REGEX)
    #include <wx/regex.h>       // for wxRegEx
  #endif
#else
  #undef JWZ_USE_REGEX
#endif // wxUSE_REGEX

/*
#endif // TEST_SUBJECT_NORMALIZE/!TEST_SUBJECT_NORMALIZE
*/

// consider that no thread can be more than this level deep
#define MAX_THREAD_DEPTH (100000)

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#define MFCMN_INDENT1_MARKER   ((size_t)-1)
#define MFCMN_INDENT2_MARKER   ((size_t)-2)

/// Used for wxLogTrace calls
#define TRACE_JWZ _T("jwz")

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

#if !defined(JWZ_USE_REGEX)

// XNOFIXME: put this code in strutil.cpp.
//
// Removes all occurences of Re: Re[n]: Re(n):
// without considering the case, and remove unneeded
// spaces (start, end, or multiple consecutive spaces).
// Reply prefixes are removed only at the start of the
// string or just after a list prefix (i.e. [ListName]).
//
// Returns the resulting string. replyPrefixSeen is set
// to true if at least one occurence of 'Re: ', 'Re[n]: '
// or 'Re(n): ' has been removed. It is set to false
// otherwise.
//
// This code is much more complicated than using a regex,
// but it is much more efficient also, at least with the
// implementation of regex in wxWindows.

String
strutil_removeAllReplyPrefixes(const String& subject,
                               bool& replyPrefixSeen)
{
   // the current state is one of:
   enum State
   {
      notInPrefix,               // initial state, before prefix
      inListPrefix,              // inside "[...]" part
      rSeen,                     // just seen the possible start of "Re:"
      eSeen,                     // just seen the possible 'e' of "Re:"
      parenSeen,                 // just seen opening parenthese
      bracketSeen,               //                   bracket
      closingSeen,               //           matching closing ')' or ']'
      colonSeen,                 // ':' of "Re:"
      normalText                 // the rest is the subject body
   } stateOld,
     state = notInPrefix;

   size_t length = subject.length();
   String subjectNorm;
   subjectNorm.reserve(length);

   // have we seen the list/reply prefix already?
   bool listPrefixAlreadySeen = false;
   replyPrefixSeen = false;

   // where does the interesting (i.e. what we don't discard) part starts
   size_t startIndex = 0;
   for ( size_t i = 0; i < length; i++ )
   {
      // by default consider that we don't have anything special
      stateOld = state;
      state = normalText;

      const wxChar c = subject[i];
      switch ( stateOld )
      {
         case notInPrefix:
            if ( c == 'r' || c == 'R' )
            {
               state = rSeen;

               // rememeber it in case we discover it is not a "Re" later
               startIndex = i;
            }
            else if ( !listPrefixAlreadySeen )
            {
               if ( c == ' ' )
               {
                  // skip leading whitespace and stay in the current state
                  state = notInPrefix;

                  startIndex = i + 1;
               }
               else if ( c == '[' )
               {
                  // we'll copy the list prefix into output
                  subjectNorm += c;

                  state = inListPrefix;
                  listPrefixAlreadySeen = true;
               }
            }
            break;

         case inListPrefix:
            subjectNorm += c;
            if ( c == ']' )
            {
               state = notInPrefix;

               // notInPrefix skips leading whitespace but here it is
               // significant
               while ( subject[i + 1] == ' ' )
               {
                  subjectNorm += ' ';
                  i++;
               }

               // the stuff before already copied
               startIndex = i + 1;
            }
            else
            {
               state = inListPrefix;
            }
            break;

         case rSeen:
            if ( c == 'e' || c == 'E' )
               state = eSeen;
            else
               state = normalText;
            break;

         case eSeen:
            // what do we have after "Re"?
            switch ( c )
            {
               case ':':
                  state = colonSeen;
                  break;

               case '[':
                  state = bracketSeen;
                  break;

               case '(':
                  state = parenSeen;
                  break;
            }
            break;

         case parenSeen:
         case bracketSeen:
            // just skip all digits inside "()" or "[]" following "Re"
            if ( isdigit(c) )
            {
               state = stateOld;
            }
            else // end of the reply depth indicator?
            {
               if ( c == (stateOld == parenSeen ? ')' : ']') )
                  state = closingSeen;
            }
            break;

         case closingSeen:
            if ( c == ':' )
               state = colonSeen;
            break;

         case colonSeen:
            // "Re:" must be followed by a space to be recognized
            if ( c == ' ' )
            {
               replyPrefixSeen = true;
               state = notInPrefix;
               startIndex = i + 1;
            }
            break;

         default:
#ifndef TEST_SUBJECT_NORMALIZE
            FAIL_MSG( _T("unknown parser state") );
#endif
            // fall through

         case normalText:
            // force the exit from the loop
            i = length;
      }
   }

   // just copy the rest to the output and bail out
   subjectNorm += subject.c_str() + startIndex;

   return subjectNorm;
}

#endif // JWZ_USE_REGEX

#ifdef TEST_SUBJECT_NORMALIZE

#include <stdio.h>

static String ShowSubject(const String& s, bool& replySeen)
{
   String sNorm = strutil_removeAllReplyPrefixes(s, replySeen);
   printf("'%s' -> '%s'%s",
          s.c_str(), sNorm.c_str(), replySeen ? " (reply)" : "");

   return sNorm;
}

int main(int argc, char **argv)
{
   static const struct
   {
      const char *in;
      const char *out;
      bool reply;
   }
   testSubjects[] =
   {
      { "",                               "",            false },
      { "foo",                            "foo",         false },
      { "Re: foo",                        "foo",         true  },
      { "Re: ",                           "",            true  },
      { "[bar] foo",                      "[bar] foo",   false },
      { "[bar] Re: foo",                  "[bar] foo",   true },
      { "Re: [bar] foo",                  "[bar] foo",   true  },
      { "Re(2): [bar] Re[4]: Re: foo",    "[bar] foo",   true  },
   };

   bool replySeen;

   puts("Example subjects test:");
   for ( size_t n = 0; n < sizeof(testSubjects)/sizeof(testSubjects[0]); n++ )
   {
      if ( ShowSubject(testSubjects[n].in, replySeen) != testSubjects[n].out )
      {
         printf(" (ERROR: should be '%s')\n", testSubjects[n].out);
      }
      else if ( replySeen != testSubjects[n].reply )
      {
         printf(" (ERROR: should %sbe reply)\n", replySeen ? "not " : "");
      }
      else
      {
         puts(" (ok)");
      }
   }

   puts("\nCommand line arguments test:");
   for ( int i = 1; i < argc; i++ )
   {
      ShowSubject(argv[i], replySeen);
      putchar('\n');
   }

   return 0;
}

#else // !TEST_SUBJECT_NORMALIZE

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
#endif
   String     *m_id;
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
   StringList messageThreadReferences() const;

#if defined(JWZ_USE_REGEX)
   String getSimplifiedSubject(wxRegEx *replyRemover,
                               const String &replacementString) const;
   bool subjectIsReply(wxRegEx *replyRemover,
                       const String &replacementString) const;
#else
   String getSimplifiedSubject(bool removeListPrefix) const;
   bool subjectIsReply(bool removeListPrefix) const;
#endif
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
   , m_id(0)
   , m_simplifiedSubject(0)
   , m_isReply(false)
{
#if defined(DEBUG)
   m_subject = hi->GetSubject();
   m_refs = hi->GetReferences();
#endif
}

inline
Threadable::~Threadable()
{
   delete m_simplifiedSubject;
   delete m_id;
}


void Threadable::destroy() {
   static int depth = 0;
   CHECK_RET(depth < MAX_THREAD_DEPTH, _T("Deep recursion in Threadable::destroy()"));
   if (m_child != 0)
   {
      depth++;
      m_child->destroy();
      depth--;
      CHECK_RET(depth >= 0, _T("Negative recursion depth in Threadable::destroy()"));
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
   Threadable *that = (Threadable *)this;    // Remove constness
   if (that->m_id != 0)
      return *that->m_id;
   that->m_id = new String;

   // Scan the provided id to remove the garbage
   const wxChar *s = m_hi->GetId().c_str();

   enum State {
      notInRef,
      openingSeen,
      atSeen
   } state = notInRef;

   size_t start = 0;
   size_t nbChar = m_hi->GetId().Len();
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
            *that->m_id = m_hi->GetId().Mid(start, i+1-start);
            i = nbChar; // exit the while loop
         }
         break;
      }
      i++;
   }

   if (that->m_id->empty()) {
      *that->m_id = String::Format(_T("<emptyId:%p>"), this);
   }

   return *that->m_id;
}


// --------------------------------------------------------------------
// Computing the list of references to use
// --------------------------------------------------------------------

StringList Threadable::messageThreadReferences() const
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
   // Each reference consists in a <...@...> form. The remaining is removed.

   StringList tmp;
   String refs = m_hi->GetReferences() + m_hi->GetInReplyTo();
   if (refs.empty())
      return tmp;

   const wxChar *s = refs.c_str();

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
            String ref(refs.Mid(start, i+1-start));
            // In case of duplicated reference we keep the last one:
            // It is important that In-Reply-To is the last reference
            // in the list.
            StringList::iterator i;
            for (i = tmp.begin(); i != tmp.end(); i++)
            {
               if (*i == ref)
               {
                  tmp.erase(i);
                  break;
               }
            }
            tmp.push_back(ref.ToStdString());
            state = notInRef;
         }
      }
      i++;
   }
   return tmp;
}

#if !defined(JWZ_USE_REGEX)

// Removes the list prefix [ListName] at the beginning
// of the line (and the spaces around the result)
//
static
String RemoveListPrefix(const String &subject)
{
   String s = subject;
   if ( !s.empty() )
   {
      if ( s[0u] == '[' )
      {
         s = s.AfterFirst(']');
         s.Trim(false /* trim from left */);
      }
   }

   return s;
}

#endif // !wxUSE_REGEX


#if defined(JWZ_USE_REGEX)
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
#if defined(JWZ_USE_REGEX)
   *(that->m_simplifiedSubject) = m_hi->GetSubject();
   if (!replyRemover)
   {
      that->m_isReply = false;
      return *that->m_simplifiedSubject;
   }
   size_t len = that->m_simplifiedSubject->Len();
   if (replyRemover->Replace(that->m_simplifiedSubject, replacementString, 1))
      // XNOFIXME: we should have a much more complicated test, because
      // this one gives false positives whenever a space or a list prefix
      // is removed (e.g. the replaceement string is empty)
      that->m_isReply = (that->m_simplifiedSubject->Len() != len);
   else
      that->m_isReply = 0;
#else
   *that->m_simplifiedSubject =
      strutil_removeAllReplyPrefixes(m_hi->GetSubject(),
                                     that->m_isReply);
   if (removeListPrefix)
      *that->m_simplifiedSubject = RemoveListPrefix(*that->m_simplifiedSubject);
#endif
   return *that->m_simplifiedSubject;
}


#if defined(JWZ_USE_REGEX)
bool Threadable::subjectIsReply(wxRegEx *replyRemover,
                                const String &replacementString) const
#else
bool Threadable::subjectIsReply(bool removeListPrefix) const
#endif
{
   Threadable *that = (Threadable *)this;    // Remove constness
   if (that->m_simplifiedSubject == 0) {
#if defined(JWZ_USE_REGEX)
      getSimplifiedSubject(replyRemover, replacementString);
#else
      getSimplifiedSubject(removeListPrefix);
#endif
   }
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


//
// XNOFIXME: the ThreadContainer objects have the same structure than
// the Threadable objects. They should be merged.
//

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

#if !defined(JWZ_NO_SORT)
   /** Returns the index to be used to compare instances
       of ThreadContainer to determine their ordering
       */
   size_t getIndex() const;
#endif // JWZ_NO_SORT

   /** Adds a ThreadContainer to the children of the caller.
       If compiled to do so, it takes into account the indexes
       of those children so that they stay ordered.
       */
#if defined(JWZ_NO_SORT)
   inline
#endif
   void addAsChild(ThreadContainer *c);

   /** Returns true if target is a children (maybe deep in the
       tree) of the caller. If withNext == true, also scans the
       nexts of the caller (which is usefull during the recursive
       search, but not to be used externally)
       */
   bool findChild(ThreadContainer *target, bool withNexts = false) const;

   /** Computes and saves (in the Threadable they store) the indentation
       and the numbering of the messages
       */
   void flush(size_t &threadedIndex, size_t indent, bool indentIfDummyNode);

   /** Recursively destroys the tree of ThreadContainer starting at the
       the calling object.
       */
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
#if defined(DEBUG)
   NumberOfThreadContainers--;
#endif
}


#if defined(JWZ_NO_SORT)
inline
#endif
size_t ThreadContainer::getIndex() const
{
#if defined(JWZ_NO_SORT)
   return 0;
#else
   const size_t foolish = 1000000000;
   static int depth = 0;
   CHECK(depth < MAX_THREAD_DEPTH, foolish,
      _T("Deep recursion in ThreadContainer::getIndex()"));
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
            _T("Negative recursion depth in ThreadContainer::getIndex()"));
         return index;
      }
      else
         return foolish;
   }
#endif
}


void ThreadContainer::addAsChild(ThreadContainer *c)
{
   // Takes into account the indices so that the new kid
   // is inserted in the correct position
#if !defined(JWZ_NO_SORT)
   CHECK_RET(c->getThreadable(), _T("No threadable in ThreadContainer::addAsChild()"));
#endif
   CHECK_RET(!c->findChild(this), _T("Adding our own parent as a child !"));
   ThreadContainer *prev = 0;
   ThreadContainer *current = getChild();

#if !defined(JWZ_NO_SORT)
   size_t cIndex = c->getThreadable()->getIndex();
   // If this part is compiled, the child will be inserted at
   // the correct position so that they all are still ordered
   // by increasing index. This is to be used if the input of
   // the threading algorithm is already sorted, to keep the
   // order. Otherwise, it is completely useless as the result
   // of the JWZ algo will anyway be combined with sorting
   // information.
   // BTW, it may be more efficient to do the sorting at the
   // end instead of incrementally keeping it. Don't know...
   while (current != 0)
   {
      if (current->getIndex() > cIndex)
         break;
      prev = current;
      current = current->getNext();
   }
#endif // !JWZ_NO_SORT

   c->setParent(this);
   c->setNext(current);
   if (prev == 0)
      setChild(c);
   else
      prev->setNext(c);
}


bool ThreadContainer::findChild(ThreadContainer *target, bool withNexts) const
{
   static int depth = 0;
   CHECK(depth < MAX_THREAD_DEPTH, true, _T("Deep recursion in ThreadContainer::findChild()"));
   if (m_child == target)
      return true;
   if (withNexts && m_next == target)
      return true;
   if (m_child != 0)
   {
      depth++;
      bool found = m_child->findChild(target, true);
      depth--;
      CHECK(depth >= 0, true, _T("Negative recursion depth in ThreadContainer::findChild()"));
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


void ThreadContainer::flush(size_t &threadedIndex, size_t indent, bool indentIfDummyNode)
{
   static int depth = 0;
   CHECK_RET(depth < MAX_THREAD_DEPTH, _T("Deep recursion in ThreadContainer::flush()"));
   CHECK_RET(m_threadable != 0, _T("No threadable in ThreadContainer::flush()"));
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
      CHECK_RET(depth >= 0, _T("Negative recursion depth in ThreadContainer::flush()"));
   }
   if (m_threadable != 0)
      m_threadable->setNext(m_next == 0 ? 0 : m_next->getThreadable());
   if (m_next != 0)
      m_next->flush(threadedIndex, indent, indentIfDummyNode);
}


void ThreadContainer::destroy()
{
   static int depth = 0;
   CHECK_RET(depth < MAX_THREAD_DEPTH, _T("Deep recursion in ThreadContainer::destroy()"));
   if (m_child != 0)
   {
      depth++;
      m_child->destroy();
      depth--;
      CHECK_RET(depth >= 0, _T("Negative recursion depth in ThreadContainer::destroy()"));
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
   bool m_breakThreadsOnSubjectChange;
   bool m_indentIfDummyNode;
#if defined(JWZ_USE_REGEX)
   wxRegEx *m_replyRemover;
   String m_replacementString;
#else
   bool m_removeListPrefixGathering;
   bool m_removeListPrefixBreaking;
#endif

public:
   // XNOFIXME: Remove this constructor
   Threader();

   Threader(const ThreadParams& thrParams);

   ~Threader();

   /** Does all the job. Input is a list of messages, output
       is the root of the first thread, with all the roots
       of the other threads as next. Moreover, all the
       Threadable objects will be filled with their threadedIndex
       (the position where they should be displayed, starting
       from 0) and their indentation.

       If shouldGatherSubjects is true, all messages that
       have the same non-empty subject will be considered
       to be part of the same thread.

       If indentIfDummyNode is true, messages under a dummy
       node will be indented.
       */
   Threadable *thread(Threadable *threadableRoot);

   void setGatherSubjects(bool x) { m_gatherSubjects = x; }
   void setBreakThreadsOnSubjectChange(bool x) { m_breakThreadsOnSubjectChange = x; }
   void setIndentIfDummyNode(bool x) { m_indentIfDummyNode = x; }

#if defined(JWZ_USE_REGEX)
   void setSimplifyingRegex(const String &x);
   void setReplacementString(const String &x) { m_replacementString = x; }
#else
   void setRemoveListPrefixGathering(bool x) { m_removeListPrefixGathering = x; }
   void setRemoveListPrefixBreaking(bool x) { m_removeListPrefixBreaking = x; }
#endif

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


Threader::Threader()
   : m_root(0)
   , m_idTable(0)
   , m_bogusIdCount(0)
   , m_gatherSubjects(true)
   , m_breakThreadsOnSubjectChange(true)
   , m_indentIfDummyNode(false)
#if defined(JWZ_USE_REGEX)
   , m_replyRemover(0)
   , m_replacementString()
#else
   , m_removeListPrefixGathering(true)
   , m_removeListPrefixBreaking(true)
#endif
{}


Threader::Threader(const ThreadParams& thrParams)
   : m_root(0)
   , m_idTable(0)
   , m_bogusIdCount(0)
   , m_gatherSubjects(thrParams.gatherSubjects)
   , m_breakThreadsOnSubjectChange(thrParams.breakThread)
   , m_indentIfDummyNode(thrParams.indentIfDummyNode)
#if defined(JWZ_USE_REGEX)
   , m_replyRemover(0)
   , m_replacementString()
#else
   , m_removeListPrefixGathering(true)
   , m_removeListPrefixBreaking(true)
#endif
{
#if defined(JWZ_USE_REGEX)
   setSimplifyingRegex(thrParams.simplifyingRegex);
   setReplacementString(thrParams.replacementString);
#endif
}


Threader::~Threader()
{
#if defined(JWZ_USE_REGEX)
   delete m_replyRemover;
#endif
}


#if defined(JWZ_USE_REGEX)
void Threader::setSimplifyingRegex(const String &x)
{
   if (!m_replyRemover) {
      // Create one
      m_replyRemover = new wxRegEx(x);
   } else {
      // Reuse the existing one
      m_replyRemover->Compile(x);
   }
}
#endif


//
// Returns true if the arguement seems to be a prime number
//
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

//
// Find a (seemingly) prime number greater than n
//
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
   hash_add(hTable, strdup(str.utf8_str()), container, 0);
}


ThreadContainer *Threader::lookUp(HASHTAB *hTable, const String &s) const
{
   void** data = hash_lookup(hTable,
                              const_cast<char *>((const char *)s.utf8_str()));
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
         free(ent->name);
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
      VERIFY(th->getChild() == 0, _T("Bad input list in Threader::thread()"));
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
   //  - Store all those containers in the hash-table, indexed by their Message-Id
   //  - Build their parent/child relations
   for (th = threadableRoot; th != 0; th = th->getNext())
   {
      VERIFY(th->getIndex() < thCount, _T("Too big in Threader::thread()"));
      // As things are now, dummy messages won't get past the algorithm
      // and won't be displayed. Thus we should not get them back when
      // re-threading this folder.
      // When this changes, we will have to take care to skip them.
      VERIFY(!th->isDummy(), _T("Dummy message in Threader::thread()"));
      buildContainer(th);
   }

   // Create a dummy root ThreadContainer (stored in m_root) and
   // scan the content of the hash-table to make all parent-less
   // containers (i.e. first message in a thread) some children of
   // this root.
   findRootSet();
   ASSERT(m_root->getNext() == 0); // root node has a next ?!

   // We are finished with this hash-table
   destroy(&m_idTable);

   // Remove all the useless containers (e.g. those that are not
   // in the root-set, have no message, but have a child)
   pruneEmptyContainers(m_root, false);
   ASSERT(m_root->getNext() == 0); // root node has a next ?!

   // Build dummy messages for the nodes that have no message.
   // Those can only appear in the root set.
   ThreadContainer *thr;
   for (thr = m_root->getChild(); thr != 0; thr = thr->getNext())
      if (thr->getThreadable() == 0)
         thr->setThreadable(thr->getChild()->getThreadable()->makeDummy());

   wxLogTrace(TRACE_JWZ, _T("Entering BreakThreads"));
   if (m_breakThreadsOnSubjectChange)
      breakThreads(m_root);
   wxLogTrace(TRACE_JWZ, _T("Leaving BreakThreads"));

   // If asked to, gather all the messages that have the same
   // non-empty subject in the same thread.
   if (m_gatherSubjects) {
      gatherSubjects();
      ASSERT(m_root->getNext() == 0); // root node has a next ?!
   }

   // Prepare the result to be returned: the root of the
   // *Threadable* tree that we will build by flushing the
   // ThreadContainer tree structure.
   Threadable *result = (m_root->getChild() == 0
      ? 0
      : m_root->getChild()->getThreadable());

   // Compute the index of each message (the line it will be
   // displayed to when threaded) and its indentation. And copy
   // the parent/child relations from ThreadContainers to the
   // Threadable objects.
   size_t threadedIndex = 0;
   if (m_root->getChild() != 0)
      m_root->getChild()->flush(threadedIndex, 0, m_indentIfDummyNode);

   // Destroy the ThreadContainer structure.
   m_root->destroy();
   delete m_root;

#if defined(DEBUG)
   ASSERT(NumberOfThreadContainers == 0); // Some ThreadContainers leak
#endif

   return result;
}



void Threader::buildContainer(Threadable *th)
{
   // Look for an already existing container that would correspond
   // to the id of this message. If this message was referenced by
   // a previously seen message, then the corresponding container
   // has been built.
   String id = th->messageThreadID();
   ASSERT(!id.empty());
   ThreadContainer *container = lookUp(m_idTable, id);

   if (container != 0)
   {
      if (container->getThreadable() == 0)
      {
         container->setThreadable(th);
      } else {
         // Oops, this container already has a message.
         // This must be because we have two messages with
         // the same id. Let's give a new id to this message,
         // and create a container for it.
         id = String(_T("<bogusId:")) << m_bogusIdCount++ << _T(">");
         container = 0;
      }
   }

   if (container == 0)
   {
      // Create a container and store it (with id as key)
      // in the hash-table
      container = new ThreadContainer(th);
      add(m_idTable, id, container);
   }

   // Let's have a look to the references of this message.
   // For each reference found, we will create a container
   // (without message) if it does not exist.
   //
   // parentRefCont is a pointer to the last referenced container
   // Kepp in mind that references are given from the root to the
   // leaf. So the last reference is the direct parent of the current
   // message
   ThreadContainer *parentRefCont = 0;
   StringList refs = th->messageThreadReferences();
   for ( StringList::iterator i = refs.begin(); i != refs.end(); i++)
   {
      String ref = *i;
      ThreadContainer *refCont = lookUp(m_idTable, ref);
      if (refCont == 0)
      {
         // No container with this id. Create one.
         refCont = new ThreadContainer();
         add(m_idTable, ref, refCont);
      }

      // If one container was found during last iteration, it must
      // become the parent of the current one (refCont) as the two
      // ids follow each other.
      if ((parentRefCont != 0) &&
          (refCont->getParent() == 0) &&
          (parentRefCont != refCont) &&
          !parentRefCont->findChild(refCont))
      {
         // They are not already linked as parentRefCont being
         // the parent of refCont, which may be the case if a previous
         // message in the same thread has been processed.
         if (!refCont->findChild(parentRefCont))
         {
            // And they are not linked the other way round: do the linkage.
#if defined(JWZ_NO_SORT)
            parentRefCont->addAsChild(refCont);
#else
            refCont->setParent(parentRefCont);
            refCont->setNext(parentRefCont->getChild());
            parentRefCont->setChild(refCont);
#endif
         }
         // else: they are linked as refCont being a parent of parentRefCont.
         // This must be because the references of this message contradict
         // those of another previous message. Let's consider the first one
         // is the right one (minimizes work ;) )
      }
      parentRefCont = refCont;
   }

   // Now, parentRefCont, if it exists, must become the parent of the
   // current message.
   if ((parentRefCont != 0) &&
       ((parentRefCont == container) ||
        container->findChild(parentRefCont))) {
      // Oops, the inverse link already exists, or we reference ourself.
      // Anyway, parentRefCont is obviously wrong (or contradicts a previous
      // References header).
      parentRefCont = 0;
   }

   // If container already has a parent, remove the link between container
   // and its parent. We must have had some References that were missing
   // some links.
   if (container->getParent() != 0) {
      if (container->getParent() == parentRefCont) {
         // No need to do anything. Reset parentRefCont
         // so that nothing will be done after.
         parentRefCont = 0;
      } else {
         // Find (in prev) the last children of our parent, just before us
         ThreadContainer *rest, *prev;
         for (prev = 0, rest = container->getParent()->getChild();
              rest != 0;
              prev = rest, rest = rest->getNext()) {
            if (rest == container) break;
         }
         ASSERT(rest != 0); // Did not find container as a child of its parent !?
         if (prev == 0)
            container->getParent()->setChild(container->getNext());
         else
            prev->setNext(container->getNext());
         container->setNext(0);
         container->setParent(0);
      }
   }

   // Build the link between parentRefCont and its new child
   if (parentRefCont != 0)
   {
      ASSERT(!container->findChild(parentRefCont));
      container->setParent(parentRefCont);
      container->setNext(parentRefCont->getChild());
      parentRefCont->setChild(container);
   }
}


void Threader::findRootSet()
{
   wxLogTrace(TRACE_JWZ, _T("Entering Threader::findRootSet()"));
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
#if defined(JWZ_NO_SORT)
            mProot->addAsChild(container);
#else
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
#endif
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
      if (c->getChild() != 0 && !fromBreakThreads) {   
         pruneEmptyContainers(c, false);
      }
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
      }
   }
}


// If one message in the root set has the same subject than another
// message (not necessarily in the root-set), merge them.
// This is so that messages which don't have Reference headers at all
// still get threaded (to the extent possible, at least.)
//
void Threader::gatherSubjects()
{
   wxLogTrace(TRACE_JWZ, _T("Entering GatherSubjects"));
   size_t count = 0;
   ThreadContainer *c = m_root->getChild();
   for (; c != 0; c = c->getNext())
      count++;

   // Make the hash-table large enough. Let's consider
   // that there are not too many (not more than one per
   // thread) subject changes.
   HASHTAB *subjectTable = create(count*2);

   wxLogTrace(TRACE_JWZ, _T("Entering collectSubjects"));
   // Collect the subjects in all the tree
   count = collectSubjects(subjectTable, m_root, true);
   wxLogTrace(TRACE_JWZ, _T("Leaving collectSubjects"));

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
      if (th == 0)   // might be a dummy message
      {
          th = c->getChild()->getThreadable();
          ASSERT(th != NULL);
      }

#if defined(JWZ_USE_REGEX)
      String subject = th->getSimplifiedSubject(m_replyRemover,
                                                m_replacementString);
#else
      String subject = th->getSimplifiedSubject(m_removeListPrefixGathering);
#endif

      // Don't thread together all subjectless messages; let them dangle.
      if (subject.empty())
         continue;

      ThreadContainer *old = lookUp(subjectTable, subject);
      if (old == c)    // oops, that's us
         continue;

      if (c->findChild(old)) {
         // It is possible that the message we just found is a child of
         // the one we are trying to merge. This happens e.g. if the References
         // give us a tree like this one
         //   Re: Subject1
         //     Subject1
         continue;
      }

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
#if defined(JWZ_USE_REGEX)
               c->getThreadable()->subjectIsReply(m_replyRemover,
                                                  m_replacementString) &&   // c has "Re:"
                !old->getThreadable()->subjectIsReply(m_replyRemover,
                                                      m_replacementString))) // and old does not
#else
               c->getThreadable()->subjectIsReply(m_removeListPrefixGathering) &&   // c has "Re:"
                !old->getThreadable()->subjectIsReply(m_removeListPrefixGathering))) // and old does not
#endif
      {
         // Make this message be a child of the other.

         // XNOFIXME: if c is a dummy node, shouldn't we include its children
         // instead of the node itself ? It works corectly as is but I suspect
         // that dummy dones are strictly useless in the ThreadData, and only
         // make the code more complicated. To be studied.
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

#if !defined(JWZ_NO_SORT)
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
#endif
            // Make old point to its kids
            old->setChild(c);
            c->setNext(newC);

            // Now that we know who is the child of the new node, we can build
            // its dummy threadable
            old->setThreadable(c->getThreadable()->makeDummy());
         }
         else
            old->getParent()->addAsChild(c);
      }

      // We've done a merge, so keep the same 'prev' next time around.
      c = prev;
   }

   destroy(&subjectTable);
   wxLogTrace(TRACE_JWZ, _T("Leaving GatherSubjects"));
}


size_t Threader::collectSubjects(HASHTAB *subjectTable,
                                 ThreadContainer *parent,
                                 bool recursive)
{
   static int depth = 0;
   ASSERT_MSG(depth < MAX_THREAD_DEPTH, _T("Deep recursion in Threader::collectSubjects()"));
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

#if defined(JWZ_USE_REGEX)
      String subject = th->getSimplifiedSubject(m_replyRemover,
                                                m_replacementString);
#else
      String subject = th->getSimplifiedSubject(m_removeListPrefixGathering);
#endif
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
#if defined(JWZ_USE_REGEX)
           old->getThreadable()->subjectIsReply(m_replyRemover,
                                                m_replacementString) &&
#else
           old->getThreadable()->subjectIsReply(m_removeListPrefixGathering) &&
#endif
           cTh != 0 &&
#if defined(JWZ_USE_REGEX)
           !cTh->subjectIsReply(m_replyRemover, m_replacementString)
#else
           !cTh->subjectIsReply(m_removeListPrefixGathering)
#endif
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
         VERIFY(depth >= 0, _T("Negative recursion depth in Threader::collectSubjects()"));
      }
   }

   // Return number of subjects found
   return count;
}


void Threader::breakThreads(ThreadContainer* c)
{
   static int depth = 0;
   CHECK_RET(depth < MAX_THREAD_DEPTH, _T("Deep recursion in Threader::breakThreads()"));

   // Dummies should have been built
   CHECK_RET((c == m_root) || (c->getThreadable() != NULL),
             _T("No threadable in Threader::breakThreads()"));

   // If there is no parent, there is no thread to break.
   if (c->getParent() != NULL)
   {
      // Dummies should have been built
      CHECK_RET(c->getParent()->getThreadable() != NULL,
                _T("No parent's threadable in Threader::breakThreads()"));

      ThreadContainer *parent = c->getParent();

#if defined(JWZ_USE_REGEX)
      if (c->getThreadable()->getSimplifiedSubject(m_replyRemover,
                                                   m_replacementString) !=
          parent->getThreadable()->getSimplifiedSubject(m_replyRemover,
                                                        m_replacementString))
#else
      if (c->getThreadable()->getSimplifiedSubject(m_removeListPrefixGathering) !=
          parent->getThreadable()->getSimplifiedSubject(m_removeListPrefixGathering))
#endif
      {
         // Subject changed. Break the thread
         if (parent->getChild() == c)
            parent->setChild(c->getNext());
         else
         {
            ThreadContainer *prev = parent->getChild();
            CHECK_RET(parent->findChild(c),
                      _T("Not child of its parent in Threader::breakThreads()"));
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
      VERIFY(depth >= 0, _T("Negative recursion depth in Threader::breakThreads()"));

      kid = next;
   }
}

// Build the input struture for the threader: a list
// of all the messages to thread.
static Threadable *BuildThreadableList(const HeaderInfoList *hilp)
{
   wxLogTrace(TRACE_JWZ, _T("Entering BuildThreadableList"));
   Threadable *root = 0;
   size_t count = hilp->Count();
   for (size_t i = 0; i < count; ++i) {
      HeaderInfo *hi = hilp->GetItemByIndex(i);
      Threadable *item = new Threadable(hi, i);
      item->setNext(root);
      root = item;
   }
   wxLogTrace(TRACE_JWZ, _T("Leaving BuildThreadableList"));
   return root;
}


// Scan the Threadable tree and fill the arrays needed to
// correctly display the threads
//
static size_t FlushThreadable(Threadable *t,
                            MsgnoType *indices,
                            size_t *indents,
                            MsgnoType *children
#if defined(DEBUG)
                            , bool seen[]
#endif
                            )
{
  size_t number = 0;    // For this one
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

   if (t->getChild()) {
      number +=
#if defined(DEBUG)
         FlushThreadable(t->getChild(), indices, indents, children, seen);
#else
         FlushThreadable(t->getChild(), indices, indents, children);
#endif
   }
   if (!t->isDummy())
   {
      children[t->getIndex()] = number;
      number++;  // Count oneself
   }
   if (t->getNext()) {
     number +=
#if defined(DEBUG)
         FlushThreadable(t->getNext(), indices, indents, children, seen);
#else
         FlushThreadable(t->getNext(), indices, indents, children);
#endif
   }
   return number;
}


//
// Copy the tree structure to a THREADNODE structure
//
static THREADNODE *MapToThreadNode(Threadable* root)
{
   if ( !root )
      return NULL;

   // we must allocate THREADNODEs with fs_get() as they're freed by cclient
   THREADNODE *thrNode = mail_newthreadnode(NULL);

   // +1 for getting a msgno
   thrNode->num = root->isDummy() ? 0 : root->getIndex()+1;
   thrNode->next = MapToThreadNode(root->getChild());
   thrNode->branch = MapToThreadNode(root->getNext());

   return thrNode;
}



// ----------------------------------------------------------------------------
// our public API
// ----------------------------------------------------------------------------

extern void JWZThreadMessages(const ThreadParams& thrParams,
                              const HeaderInfoList *hilp,
                              ThreadData *thrData)
{
   wxLogTrace(TRACE_JWZ, _T("Entering JWZThreadMessages"));
   Threadable *threadableRoot = BuildThreadableList(hilp);

   Threader *threader = new Threader(thrParams);

   // Do the work
   threadableRoot = threader->thread(threadableRoot);

   if ( threadableRoot )
   {
      // Map to needed output format
      thrData->m_root = MapToThreadNode(threadableRoot);

      // Clean up
      threadableRoot->destroy();
      delete threadableRoot;
   }

   delete threader;
   wxLogTrace(TRACE_JWZ, _T("Leaving JWZThreadMessages"));
}

#endif // TEST_SUBJECT_NORMALIZE/!TEST_SUBJECT_NORMALIZE

