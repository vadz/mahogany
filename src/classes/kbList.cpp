/*-*- c++ -*-********************************************************
 * kbList.cc : a double linked list                                 *
 *                                                                  *
 * (C) 1998-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
 *                                                                  *
 * $Id$         
 *                                                                  *
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "kbList.h"
#endif

#include   "Mconfig.h"
#include   "kbList.h"


kbListNode::kbListNode( void *ielement,
                        kbListNode *iprev,
                        kbListNode *inext)
{
   next = inext;
   prev = iprev;
   if(prev)
      prev->next = this;
   if(next)
      next->prev = this;
   element = ielement;
}

kbListNode::~kbListNode()
{
   if(prev)
      prev->next = next;
   if(next)
      next->prev = prev;
}


kbList::iterator::iterator(kbListNode *n)
{
   node = n;
}

void *
kbList::iterator::operator*() 
{
   return node->element;
}

kbList::iterator &
kbList::iterator::operator++()
{
   ASSERT(node != NULL);
   node  = node ? node->next : NULL;
   return *this;
}

kbList::iterator &
kbList::iterator::operator--()
{
   ASSERT(node != NULL);
   node = node ? node->prev : NULL; 
   return *this;
}
kbList::iterator &
kbList::iterator::operator++(int /* foo */)
{
   ASSERT(node != NULL);
   return operator++();
}

kbList::iterator &
kbList::iterator::operator--(int /* bar */)
{
   ASSERT(node != NULL);
   return operator--();
}


bool
kbList::iterator::operator !=(kbList::iterator const & i) const
{
   return node != i.node;
}

bool
kbList::iterator::operator ==(kbList::iterator const & i) const
{
   return node == i.node;
}

kbList::kbList(bool ownsEntriesFlag)
{
   first = NULL;
   last = NULL;
   ownsEntries = ownsEntriesFlag;
}

void
kbList::push_back(void *element)
{
   if(! first) // special case of empty list
   {
      first = new kbListNode(element);
      last = first;
      return;
   }
   else
      last = new kbListNode(element, last);
}

void
kbList::push_front(void *element)
{
   if(! first) // special case of empty list
   {
      push_back(element);
      return;
   }
   else
      first = new kbListNode(element, NULL, first);
}

void *
kbList::pop_back(void)
{
   iterator i;
   void *data;
   bool ownsFlagBak = ownsEntries;
   i = tail();
   if(! i.Node())
      return NULL;
   data = *i;
   ownsEntries = false;
   erase(i);
   ownsEntries = ownsFlagBak;
   return data;
}

void *
kbList::pop_front(void)
{
   iterator i;
   void *data;
   bool ownsFlagBak = ownsEntries;
   
   i = begin();
   if(! i.Node())
      return NULL;
   data = *i;
   ownsEntries = false;
   erase(i);
   ownsEntries = ownsFlagBak;
   return data;
   
}

void
kbList::insert(kbList::iterator & i, void *element)
{   
   ASSERT(i.Node());
   if(! i.Node())
      return;
   else if(i.Node() == first)
   {
      push_front(element);
      i = first;
      return;
   }
   i = kbList::iterator(new kbListNode(element, i.Node()->prev, i.Node()));
}

void
kbList::doErase(kbList::iterator & i)
{
   ASSERT(i.Node());
   kbListNode
      *node = i.Node(),
      *prev, *next;
   
   if(! node) // illegal iterator
      return;

   prev = node->prev;
   next = node->next;
   
   // correct first/last:
   if(node == first)
      first = node->next;
   if(node == last)  // don't put else here!
      last = node->prev;

   // build new links:
   if(prev)
      prev->next = next;
   if(next)
      next->prev = prev;

   // delete this node and contents:
   // now done separately
   //if(ownsEntries)
   //delete *i;
   delete i.Node();

   // change the iterator to next element:
   i = kbList::iterator(next);
}

kbList::~kbList()
{
   kbListNode *next;

   if(ownsEntries)
   {
      /* The base class handling only void * cannot properly delete
         anything. This code should never really be called. */
      ASSERT(0);
      /* delete first->element; */
   }
   while ( first != NULL )
   {
      next = first->next;
      delete first;
      first = next;
   }
}

kbList::iterator
kbList::begin(void) const
{
   return kbList::iterator(first);
}

kbList::iterator
kbList::tail(void) const
{
   return kbList::iterator(last);
}

kbList::iterator
kbList::end(void) const
{
   return kbList::iterator(NULL); // the one after the last
}

unsigned
kbList::size(void) const // inefficient
{
   unsigned count = 0;
   kbList::iterator i;
   for(i = begin(); i != end(); i++, count++)
      ;
   return count;
}


#ifdef   DEBUG
/** Buffer to hold debug information for systems which haven't
    got stdout/stderr visible */
char
kbList::iterator::ms_debuginfo[512];
#endif





#ifdef   KBLIST_TEST

#include   <iostream.h>

KBLIST_DEFINE(kbListInt,int);
   
int main(void)
{
   int
      n, *ptr;
   kbListInt
      l;
   kbListInt::iterator
      i;
   
   for(n = 0; n < 10; n++)
   {
      ptr = new int;
      *ptr = n*n;
      l.push_back(ptr);
   }

   i = l.begin(); // first element
   i++; // 2nd
   i++; // 3rd
   i++; // 4th, insert here:
   ptr = new int;
   *ptr = 4444;
   l.insert(i,ptr);

   // this cannot work, because l.end() returns NULL:
   i = l.end(); // behind last
   i--;  // still behind last
   l.erase(i);  // doesn't do anything

   // this works:
   i = l.tail(); // last element
   i--;
   --i;
   l.erase(i); // erase 3rd last element (49)
   
   for(i = l.begin(); i != l.end(); i++)
      cout << *i << '\t' << *((int *)*i) << endl;

   
   return 0;
}
#endif
