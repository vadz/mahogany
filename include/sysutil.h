/*-*- c++ -*-********************************************************
 * sysutil.h : utility functions for various OS functionality       *
 *                                                                  *
 * (C) 1999 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/


#ifndef SYSUTIL_H
#define SYSUTIL_H

#ifndef  USE_PCH
#endif // USE_PCH

#include <wx/filename.h>

/**@name Operating system helper functions */
//@{

/** Compares two files for being identical, using stat().
    If files do not exist and cannot be temporarily created, it will
    fall back to strutil_comparefilenames() instead.
    @param file1 first file name
    @param file2 second file name
    @return true if the files are identical
*/
bool sysutil_compare_filenames(String const &file1, String const &file2);

/** Tiny class which opens a temp file in ctor and removes it in dtor unless
    Ok() has been called. It is useful in functions where a temp file is
    opened in the beginning and then many things are done each of which may
    fail. Instead of closing the file manually in the case of each failure,
    use an object of this class and call Ok() once at the very end if there
    were no errors.
*/
class MTempFileName
{
public:
   // def ctor: creates the temp file and doesn't delete it unless told to do
   // so (i.e. Ok() is called)
   MTempFileName() : m_name (wxFileName::CreateTempFileName(_T("Mahogany")))
      { m_ok = false; }

   // ctor which takes a temp file name: still won't be deleted unless Ok() is
   // called
   MTempFileName(const String& name) : m_name(name) { m_ok = false; }

   /// returns FALSE if temp file name couldn't be generated
   bool IsOk() const { return !!m_name; }

   /// get the name of the temp file
   const String& GetName() const { return m_name; }

   /// tells us not to delete the temp file
   void Ok() { m_ok = true; }

   ~MTempFileName()
   {
      if ( !m_ok && !m_name.empty() )
      {
         if ( wxRemove(m_name) != 0 )
         {
            wxLogDebug(_T("Stale temp file '%s' left."), m_name.c_str());
         }
      }
   }

private:
   String m_name;
   bool   m_ok;
};

//@}
#endif

