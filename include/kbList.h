/*-*- c++ -*-********************************************************
 * kbList.h : a double linked list                                  *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$          *
 *                                                                  *
 * $Log$
 * Revision 1.2  1998/05/18 17:48:22  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
 * Revision 1.1  1998/05/13 19:01:42  KB
 * added kbList, adapted MimeTypes for it, more python, new icons
 *
 *******************************************************************/

#ifndef   KBLIST_H
#   define   KBLIST_H

#ifdef __GNUG__
#   pragma interface "kbList.h"
#endif

#ifndef   NULL
#   define   NULL   0
#endif

struct kbListNode
{
   struct kbListNode *next;
   struct kbListNode *prev;
   void *element;
   kbListNode( void *ielement,
               kbListNode *iprev = NULL,
               kbListNode *inext = NULL);
   ~kbListNode();
};

class kbListIterator
{
protected:
   kbListNode *node;
   kbListNode * Node(void);
   friend class kbList;
public:
   kbListIterator(kbListNode *n = NULL);
   void * operator*();
   kbListIterator & operator++();
   kbListIterator & operator--();
   kbListIterator & operator++(int); //postfix
   kbListIterator & operator--(int); //postfix
   bool operator !=(kbListIterator const &) const; 
   bool operator ==(kbListIterator const &) const; 
};

class kbList
{
public:
   kbList(bool ownsEntriesFlag = true);
   ~kbList();

   // tell list whether it owns objects:
   void ownsObjects(bool ownsflag = true)
      { ownsEntries = ownsflag; }

   bool ownsObjects(void)
      { return ownsEntries; }
   
   void push_back(void *element);
   void push_front(void *element);

   void *pop_back(void);
   void *pop_front(void);

   void insert(kbListIterator & i, void *element);
   void erase(kbListIterator & i);
   
   kbListIterator begin(void) const;
   kbListIterator end(void) const;
   kbListIterator tail(void) const;

   unsigned size(void) const;
   bool empty(void) const
      { return first == NULL ; }
//   unsigned count(void);
private:
   bool
      ownsEntries;
   kbListNode
      *first,
      *last;
   kbList(kbList const &foo);
   kbList& operator=(const kbList& foo);
};

// cast an iterator to a pointer:
#define   kbListICast(type, iterator)   ((type *)*iterator)

// cast an iterator to a const pointer:
#define   kbListIcCast(type, iterator)   ((type const *)*iterator)
   
#endif // KBLIST_H
