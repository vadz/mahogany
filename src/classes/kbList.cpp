/*-*- c++ -*-********************************************************
 * kbList.cc : a double linked list                                 *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$          *
 *                                                                  *
 * $Log$
 * Revision 1.2  1998/05/14 16:39:31  VZ
 * fixed SIGSEGV in ~kbList if the list is empty
 *
 * Revision 1.1  1998/05/13 19:02:11  KB
 * added kbList, adapted MimeTypes for it, more python, new icons
 *
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "kbList.h"
#endif

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


kbListIterator::kbListIterator(kbListNode *n)
{
   node = n;
}

void *
kbListIterator::operator*() const
{
   return node->element;
}

kbListNode *
kbListIterator::Node(void)
{
   return node;
}

kbListIterator &
kbListIterator::operator++()
{
   node  = node ? node->next : NULL;
   return *this;
}

kbListIterator &
kbListIterator::operator--()
{
   node = node ? node->prev : NULL; 
   return *this;
}
kbListIterator &
kbListIterator::operator++(int foo)
{
   return operator++();
}

kbListIterator &
kbListIterator::operator--(int bar)
{
   return operator--();
}


bool
kbListIterator::operator !=(kbListIterator const & i) const
{
   return node != i.node;
}

bool
kbListIterator::operator ==(kbListIterator const & i) const
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

void
kbList::insert(kbListIterator & i, void *element)
{   
   if(! i.Node())
      return;
   else if(i.Node() == first)
   {
      push_front(element);
      return;
   }
   else if(i.Node() == last)
   {
      push_back(element);
      return;
   }
   (void) new kbListNode(element, i.Node()->prev, i.Node());
}

void
kbList::erase(kbListIterator & i)
{
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
   else if(node == last)
      last = node->prev;

   // build new links:
   if(prev)
      prev->next = next;
   if(next)
      next->prev = prev;

   // delete this node and contents:
   if(ownsEntries)
      delete *i;
   delete i.Node();

   // change the iterator to next element:
   i = kbListIterator(next);
}

kbList::~kbList()
{
   kbListNode *next;

   while ( first != NULL )
   {
      next = first->next;
      if(ownsEntries)
         delete first->element;
      delete first;
      first = next;
   }
}

kbListIterator
kbList::begin(void)
{
   return kbListIterator(first);
}

kbListIterator
kbList::tail(void)
{
   return kbListIterator(last);
}

kbListIterator
kbList::end(void)
{
   return kbListIterator(NULL); // the one after the last
}

#ifdef   KBLIST_TEST

#include   <iostream.h>

int main(void)
{
   int
      n, *ptr;
   kbList
      l;
   kbListIterator
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
