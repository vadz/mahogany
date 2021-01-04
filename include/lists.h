/*-*- c++ -*-********************************************************
 * lists.h : a double-linked list                                   *
 *                                                                  *
 * (C) 2000 by Greg Noel (GregNoel@san.rr.com)                      *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifndef   LISTS_H
#define   LISTS_H

#ifndef ASSERT
#  ifdef wxASSERT
#     define   ASSERT(x) wxASSERT(x)
#  else
#     include   <assert.h>
#     define   ASSERT(x) assert(x);
#  endif
#endif

// disable some VC++ warnings
#ifdef _MSC_VER
#  pragma warning(disable:4284) // return type for 'identifier::operator->' is
                                // not a UDT or reference to a UDT. Will
                                // produce errors if applied using infix
                                // notation
#  pragma warning(disable:4355) // 'this': used in base member initializer list
#endif // Visual C++

/**@name An implementation of STL-like list container class using
    circularly double-linked entries. */
//@{

/** Macro to define a self-validating node entry when debugging.
*/
#ifdef DEBUG
#define M_LIST_BASE \
struct ListNodeBase \
{ \
   ListNode *prev, *next; \
   unsigned magic; \
   inline ListNodeBase(ListNode *iprev, ListNode *inext) \
      : prev(iprev), next(inext), magic(0xBA5EBA5E) \
      {} \
   inline ListNodeBase(void) \
      : prev((ListNode*)this), next((ListNode*)this), magic(0xDEADBEEF) \
      {} \
   inline ~ListNodeBase(void) \
      { magic = 0xDECEA5ED; } \
   inline void NodeCheck(void) const \
      { ASSERT(magic == 0xBA5EBA5E); } \
   inline void NodeRefCheck(void) const \
      { ASSERT(magic == 0xBA5EBA5E || magic == 0xDEADBEEF); } \
};
#else
#define M_LIST_BASE \
struct ListNodeBase \
{ \
   ListNode *prev, *next; \
   inline ListNodeBase(ListNode *iprev, ListNode *inext) \
      : prev(iprev), next(inext) \
      {} \
   inline ListNodeBase(void) \
      : prev((ListNode*)this), \
        next((ListNode*)this) \
      {} \
   inline void NodeCheck(void) const {} \
   inline void NodeRefCheck(void) const {} \
};
#endif

/** Macro to define a list node used internally by the container
    class.  It represents a single element in the list. It is not
    intended for general use outside container class functions.
*/
#define M_LIST_NODE \
struct ListNode; /* forward decl */ \
M_LIST_BASE \
struct ListNode : public ListNodeBase \
{ \
   value_type element; \
   /** Constructor - it automatically links the node into the list, if \
       the iprev, inext parameters are given. \
       @param ielement reference to the data for this node \
       @param iprev previous element in list \
       @param inext next element in list \
   */ \
   inline ListNode(const_reference e, \
               ListNode *iprev, ListNode *inext) \
      : ListNodeBase(iprev, inext), element(e) \
      { \
         prev->next = this; \
         next->prev = this; \
      } \
   /** Destructor. \
       remove the node from the list \
   */ \
   inline ~ListNode(void) \
      { \
         NodeCheck(); \
         prev->next = next; \
         next->prev = prev; \
      } \
}

/// An iterator class for a list, just like for the STL classes.
#define M_ITERATOR(name) \
class iterator \
{ \
private: \
   friend class name; \
   /* the node to which this iterator points */ \
   ListNode *node; \
protected: \
   reference GetData() \
      { NodeCheck(); return node->element; } \
public: \
   inline void NodeCheck(void) const \
      { node->NodeCheck(); } \
\
   /** Constructors. \
       @param n if not NULL, the node to which to point \
   */ \
   inline iterator(ListNode *n = NULL) : node(n) {} \
   iterator(const iterator &i) = default; \
   iterator& operator=(const iterator &i) = default; \
   /** Dereference operator. \
       @return the data pointer of the node belonging to this \
       iterator \
   */ \
   inline reference operator*(void) { return GetData(); } \
   inline pointer operator->(void) { return &(operator*()); } \
   /** Increment operator - prefix, goes to next node in list. \
       @return itself \
   */ \
   inline iterator &operator++(void) \
      { \
         NodeCheck(); \
         node = node->next; \
         return *this; \
      } \
   /** Decrement operator - prefix, goes to previous node in list. \
       @return itself \
   */ \
   inline iterator &operator--(void) \
      { \
         node->NodeRefCheck(); \
         node = node->prev; \
         return *this; \
      } \
   /** Increment operator - postfix, goes to next node in list. \
       @return original iterator location \
   */ \
   inline const iterator operator++(int) \
      { \
         NodeCheck(); \
         iterator i = *this; \
         operator++(); \
         return i; \
      } \
   /** Decrement operator - postfix, goes to previous node in list. \
       @return original iterator location \
   */ \
   inline const iterator operator--(int) \
      { \
         NodeCheck(); \
         iterator i = *this; \
         operator--(); \
         return i; \
      } \
   /* Comparison operator. \
      @return true if equal \
   */ \
   inline bool operator==(iterator const &i) const \
      { \
         node->NodeRefCheck(); i.node->NodeRefCheck(); \
         return node == i.node; \
      } \
   /** Comparison operator. \
       @return true if not equal. \
   */ \
   inline bool operator!=(iterator const &i) const \
      { return !(operator==(i)); } \
}

/** Macro that defines everything except the destructor, since several
    of the specialized classes need to superceed it.
*/
#define M_LIST_common(name,type) \
class name \
{ \
public: \
   typedef type value_type; \
   typedef value_type *pointer; \
   typedef const value_type *const_pointer; \
   typedef value_type &reference; \
   typedef const value_type &const_reference; \
   M_LIST_NODE; \
protected: \
   ListNodeBase header; \
public: \
   M_ITERATOR(name); \
   inline name(void) {} \
\
   /** Get head of list. \
       @return iterator pointing to head of list \
   */ \
   inline iterator begin(void) const \
      { return header.next; } \
   /** Get end of list. \
       @return iterator pointing after the end of the list. This is an \
       invalid iterator which cannot be dereferenced or decremented. It is \
       only of use in comparisons. NOTE: this is different from STL! \
       @see tail \
   */ \
   inline iterator end(void) const \
      { return (ListNode*)&header; } \
   /* Query whether list is empty. \
      @return true if list is empty \
   */ \
   inline bool empty(void) const \
      { return header.next == (ListNode*)&header; } \
   /* Get the number of elements in the list. \
      @return number of elements in the list \
   */ \
   inline unsigned size(void) const /* inefficient */ \
      { \
         unsigned count = 0; \
         iterator i = begin(); \
         while (i != end()) \
            ++i, ++count; \
         return count; \
      } \
\
   inline reference front(void) const \
      { return *begin(); } \
   inline reference back(void) const \
      { return *iterator(header.prev); } \
\
   /** Erase an element, return iterator to following element. \
       @param i iterator pointing to the element to be deleted \
   */ \
   inline iterator erase(iterator i) \
      { \
         i.NodeCheck(); \
         iterator next = i; ++next; \
         delete i.node; \
         return next; \
      } \
   /** Erase all elements of the list */ \
   inline void clear() \
   { \
      while ( !empty() ) \
         (void)erase(begin()); \
   } \
   /** Delete element at head of list. \
   */ \
   inline void pop_front(void) { erase(begin()); } \
   /** Delete element at end of list. \
   */ \
   inline void pop_back(void) { erase(header.prev); } \
\
   /** Add an entry at the head of the list. \
       @param element pointer to data \
   */ \
   inline void push_front(const_reference element) \
      { \
         new ListNode(element, (ListNode*)&header, header.next); \
      } \
   /** Add an entry at the end of the list. \
       @param element pointer to data \
   */ \
   inline void push_back(const_reference element) \
      { \
         new ListNode(element, header.prev, (ListNode*)&header); \
      } \
   /** Insert an element either before the specified one or, if the iterator \
       is at end of list, as last element of the list. \
   */ \
   inline void insert(iterator &i, const_reference element) \
      { \
         ListNode *node; \
         if ( i == end() ) \
            node = (ListNode *)&header; \
         else \
         { \
            i.NodeCheck(); \
            node = i.node; \
         } \
         i = new ListNode(element, node->prev, node); \
      } \
   inline ~name(void) \
      { \
         clear(); \
      } \
}

/** Macro to define a list with a given name, having elements of the given
    type; i.e. M_LIST(Int,int) would create an Int type holding ints.
*/
#define M_LIST(name,type) \
   M_LIST_common(name,type)

/** Macro to define a list with a given name, having elements of pointer
    to the given dynamic type; i.e. M_LIST_PTR(MyPtr,obj) will create a
    MyPtr type holding pointers-to-obj.  The list does NOT take ownership
    of any pointers in it; the objects exist independently of the list.
*/
#define M_LIST_PTR(name,type) \
M_LIST_common(name##_common,type*); \
class name : public name##_common \
{ \
public: \
   class iterator : public name##_common::iterator \
   { \
   public: \
      iterator(ListNode *n = NULL) : name##_common::iterator(n) {} \
      iterator(const name##_common::iterator &i) \
         : name##_common::iterator(i) {} \
      inline value_type operator*(void) \
         { return GetData(); } \
      inline value_type operator->(void) \
         { return GetData(); } \
   }; \
}

/** Macro to define a list with a given name, having elements of pointer
    to the given dynamic type; i.e. M_LIST_OWN(MyOwn,obj) will create a
    MyOwn type holding pointers-to-obj.  The list will take ownership of
    any pointer in it and will delete any orphaned objects.
*/
#define M_LIST_OWN(name,type) \
M_LIST_common(name##_common,type*); \
class name : public name##_common \
{ \
public: \
   class iterator : public name##_common::iterator \
   { \
   public: \
      typedef name##_common::iterator iterator_common; \
      \
      iterator(ListNode *n = NULL) : name##_common::iterator(n) {} \
      iterator(const iterator_common &i) : name##_common::iterator(i) {} \
      inline value_type operator->(void) \
         { return GetData(); } \
      inline value_type operator*(void) \
         { return GetData(); } \
   }; \
   inline iterator erase(iterator i) \
      { \
         i.NodeCheck(); \
         delete i.operator->(); \
         return (iterator)name##_common::erase(i); \
      } \
   inline void clear() \
   { \
      while ( !empty() ) \
         (void)erase(begin()); \
   } \
   inline ~name(void) \
      { \
         clear(); \
      } \
}

/// common part of M_LIST_RC and M_LIST_RC_ABSTRACT
#define M_LIST_RC_common(name,type,operator_star_and_inline) \
M_LIST_common(name##_common,type*); \
class name : public name##_common \
{ \
public: \
   class iterator : public name##_common::iterator \
   { \
   public: \
      iterator(ListNode *n = NULL) : name##_common::iterator(n) {} \
      iterator(const name##_common::iterator &i) \
         : name##_common::iterator(i) {} \
      operator_star_and_inline \
      value_type operator->(void) { return GetData(); } \
   }; \
   inline iterator erase(iterator i) \
      { \
         i.NodeCheck(); \
         i->DecRef(); \
         return name##_common::erase(i); \
      } \
   inline void clear() \
   { \
      while ( !empty() ) \
         (void)erase(begin()); \
   } \
   inline ~name(void) \
      { \
         clear(); \
      } \
}

/// helper for M_LIST_RC
#define M_LIST_RC_operator_star(name, type)                                \
      inline type operator*(void)                                          \
         { return *(name##_common::iterator::operator*()); }               \
      inline

/** Macro to define a list with a given name, having elements of pointer
    to the given reference-counted type; i.e. M_LIST_RC(MyRef,obj) will
    create a MyRef type holding pointers-to-obj.  The list will take
    ownership of any pointer in it and will DecRef any orphaned objects.
*/
#define M_LIST_RC(name,type)                                               \
      M_LIST_RC_common(name, type, M_LIST_RC_operator_star(name, type))

/**
   M_LIST_RC_ABSTRACT is just like M_LIST_RC but for abstract classes.

   M_LIST_RC doesn't work for abstract classes because we can't have a function
   returning objects of such classes and operator*() does just that. So we
   define a macro which does exactly the same thing as M_LIST_RC except that it
   omits the definition of operator*().
 */
#define M_LIST_RC_ABSTRACT(name,type) M_LIST_RC_common(name, type, inline)

#endif // LISTS_H

