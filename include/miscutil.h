/*-*- c++ -*-********************************************************
 * miscutil.h : miscellaneous utility functions                     *
 *                                                                  *
 * (C) 1999-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/


#ifndef MISCUTIL_H
#define MISCUTIL_H

#ifndef  USE_PCH
#endif

class WXDLLEXPORT wxArrayString;
class ASMailFolder;
class UIdArray;

/**@name Miscellaneous utility functions */
//@{

class wxFrame;
class MailFolder;

/// set the title and statusbar to show the number of messages in folder
extern void UpdateTitleAndStatusBars(const String& title,
                                     const String& status,
                                     wxFrame *frame,
                                     const MailFolder *folder);

class wxColour;

/// get the colour by name which may be either a colour or RGB specification
extern bool ParseColourString(const String& name, wxColour* colour = NULL);

/// get the colour name - pass it to ParseColorString to get the same colour
extern String GetColourName(const wxColour& color);

/// get the colour by name and fallback to default (warning the user) if failed
extern void GetColourByName(wxColour *colour,
                            const String& name,
                            const String& defaultName);

class BoundArrayCommon
{
public:
   BoundArrayCommon() : m_size(0) {}
   
   size_t Size() const { return m_size; }

protected:
   size_t m_size;
};

#define BOUND_ARRAY(type,name) \
   class name : public BoundArrayCommon \
   { \
   public: \
      typedef type HostType; \
   \
      name() : m_array(NULL) {} \
      ~name() { Destroy(); } \
   \
      void Initialize(size_t count); \
      type *Get() { return m_array; } \
      type& At(size_t offset); \
      type& operator[](size_t offset) { return At(offset); } \
   \
   private: \
      void Destroy(); \
   \
      type *m_array; \
   }

#define IMPLEMENT_BOUND_ARRAY(name) \
   void name::Destroy() { delete[] m_array; } \
   \
   void name::Initialize(size_t count) \
   { \
      ASSERT( !m_array ); \
      m_array = new name::HostType[m_size = count]; \
   } \
   \
   name::HostType& name::At(size_t offset) \
   { \
      ASSERT( offset < m_size ); \
      return m_array[offset]; \
   } \

#define BOUND_POINTER(type,name) \
   class name \
   { \
   public: \
      typedef type HostType; \
   \
      name() : m_pointer(NULL) {} \
      ~name() { Destroy(); } \
   \
      void Initialize(); \
      type *operator->() { return m_pointer; } \
   \
   private: \
      void Destroy(); \
   \
      type *m_pointer; \
   }

#define IMPLEMENT_BOUND_POINTER(name) \
   void name::Destroy() { delete m_pointer; } \
   \
   void name::Initialize() \
   { \
      ASSERT( !m_pointer ); \
      m_pointer = new name::HostType; \
   } \


class TreeIteratorNode
{
public:
   virtual ~TreeIteratorNode() {}
   
   virtual TreeIteratorNode *GetChild(size_t order) = 0;
   virtual TreeIteratorNode *GetNext() = 0;
};

WX_DEFINE_ARRAY(TreeIteratorNode *,TreeIteratorResult);

class TreeIterator
{
public:
   ~TreeIterator();
   
   bool End() { return m_offset == m_result.GetCount(); }
   void operator++() { ++m_offset; }
   
protected:
   void Initialize(TreeIteratorNode *start);
   TreeIteratorNode *ActualCommon() { return m_result[m_offset]; }

private:
   void Walk(TreeIteratorNode *tree);
   
   size_t m_offset;
   TreeIteratorResult m_result;
};

#define DECLARE_TREE_ITERATOR(name,driver) \
   class TreeIteratorNode_##name : public TreeIteratorNode \
   { \
   public: \
      TreeIteratorNode_##name(driver::Type value) : m_value(value) {} \
   \
      virtual TreeIteratorNode *GetChild(size_t order) \
         { return CheckNull(m_driver.GetChild(m_value,order)); } \
      virtual TreeIteratorNode *GetNext() \
         { return CheckNull(m_driver.GetNext(m_value)); } \
   \
      driver::Type m_value; \
   \
   private: \
      TreeIteratorNode_##name *CheckNull(driver::Type node) \
      { \
         return m_driver.IsNull(node) ? NULL \
            : new TreeIteratorNode_##name(node); \
      } \
   \
      driver m_driver; \
   }; \
   \
   class name : public TreeIterator \
   { \
   public: \
      name(driver::Type start) \
         { Initialize(new TreeIteratorNode_##name(start)); } \
      driver::Type Actual() \
         { return ((TreeIteratorNode_##name *)ActualCommon())->m_value; } \
   }

//@}

#endif // MISCUTIL_H

