/*-*- c++ -*-********************************************************
 * kbList.h : a double linked list                                  *
 *                                                                  *
 * (C) 1998-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifndef   KBLIST_H
#   define   KBLIST_H

#include "lists.h"

/* backward compatibility; derive kbList from M_LIST */

/* In the long run, we will be converting to STL, so this macro will
   be going away.  It has some features that aren't supported by STL
   so in the short run, kbLists should be converted to an equivalent
   M_LIST type (which has more STL-like semantics).

   If what you really want is a container class completely owning an
   object, try M_LIST(MyObj, obj).  MyObj is a true container of objs;
   no separate obj is allocated.

   If what you really want is a container class of pointers that do not
   own the underlying object (and will continue to exist after the list
   is destroyed), try M_LIST_PTR(MyPtr, obj).  This is much the same as
   a kbList with ownsEntries set to false.

   If what you really want is a container class of pointers that take
   ownership of the object, try M_LIST_OWN(MyOwn, obj).  This is much
   the same as a kbList with ownsEntries set to true.

   Finally, if what you want is a container class of pointers that take
   ownership of a reference-counted object, try M_LIST_RC(MyRef, obj).
   This is similar to M_LIST_OWN in that orphaned objects (i.e., objects
   not retrieved back from the list) are automatically DecRef()ed, which
   may cause the object to be deleted.

   The iterator of all of these classes also acts as a smart pointer;
   that is, iter->field operates on the field of the underlying object.
*/

/** Function which prints debug information about the iterator.
*/
#ifdef DEBUG
extern const String DebugIterator(const void *me);
#define M_ITERATOR_DEBUG \
inline const String Debug(void) const { return ::DebugIterator(this); }
#else
#define M_ITERATOR_DEBUG
#endif

/** Macro to define a kbList with a given name, having elements of
    pointer to the given type; i.e. KBLIST_DEFINE(kbInt,int) will
    create a kbInt type holding int pointers.
*/
#define KBLIST_DEFINE(name,type) \
M_LIST_common(name##_common,type*); \
class name : public name##_common \
{ \
protected: \
   /* if true, list owns entries */ \
   bool ownsEntries; \
public: \
   class iterator : public name##_common::iterator \
   { \
   public: \
      M_ITERATOR_DEBUG \
      inline iterator(ListNode *n = NULL) \
         : name##_common::iterator(n) {} \
      iterator(const name##_common::iterator &i) \
         : name##_common::iterator(i) {} \
      inline value_type operator->(void) { return operator*(); } \
   }; \
   /** Constructor. \
       @param ownsEntriesFlag if true, the list owns the entries and \
       will issue a delete on each of them when erasing them.  If \
       false, the entries themselves will not get deleted.  Do not \
       use this with array types! \
   */ \
   inline name(bool ownsEntriesFlag = true) \
      : ownsEntries(ownsEntriesFlag) {} \
\
   inline iterator tail(void) const \
      { return header.prev; } \
   inline value_type remove(iterator &i) \
      { \
         i.NodeCheck(); \
         value_type v = *i; \
         i = name##_common::erase(i); \
         return v; \
      } \
   inline iterator erase(iterator i) \
      { \
         i.NodeCheck(); \
         if (ownsEntries) \
            delete *i; \
         return name##_common::erase(i); \
      } \
   inline value_type pop_front(void) \
      { \
         value_type p = front(); \
         name##_common::pop_front(); \
         return p; \
      } \
   inline value_type pop_back(void) \
      { \
         value_type p = back(); \
         name##_common::pop_back(); \
         return p; \
      } \
\
   /** Destructor. \
   */ \
   inline ~name(void) \
      { \
         while (!empty()) \
            erase(begin()); \
      } \
}

#ifdef MCONFIG_H
/// define the most commonly used list type once:
KBLIST_DEFINE(kbStringList, String);
#endif
//@}

#endif // KBLIST_H
