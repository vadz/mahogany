/*-*- c++ -*-********************************************************
 * kbList.cc : a double linked list                                 *
 *                                                                  *
 * (C) 1998-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *                                                                  *
 *******************************************************************/

#include "Mpch.h"

#ifndef USE_PCH
#  include   "kbList.h"
#endif // USE_PCH

#ifdef   DEBUG
/** Simulate the layout of a list of pointers.  This struct knows
    entirely too much about the list internals for its own good.
*/
class DebugLayout
{
public:
   typedef void* value_type;
   typedef value_type *pointer;
   typedef const value_type *const_pointer;
   typedef value_type &reference;
   typedef const value_type &const_reference;

   M_LIST_NODE;
   M_ITERATOR(DebugLayout);

   /** Function which prints debug information about the iterator.
   */
   static const String DebugIter(iterator *me)
      {
         String debugInfo;
         if(me->node == NULL)
            debugInfo.Printf(_T("iterator::Debug(): %p  Node: NULL"), me);
         else
            debugInfo.Printf(_T("iterator::Debug(): %p  Node: %p  "
                                "Node.next: %p  Node.prev: %p  "
                                "Node.element: %p"),
                              me, me->node,
                              me->node->next,
                              me->node->prev,
                              me->node->element);

         return debugInfo;
      }
};

/** Force the type and use the logic in the simulated layout
    to generate the debugging message.
*/
const String
DebugIterator(const void *me)
{
   return DebugLayout::DebugIter((DebugLayout::iterator *)me);
}
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
