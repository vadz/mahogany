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

//@}

#endif // MISCUTIL_H

