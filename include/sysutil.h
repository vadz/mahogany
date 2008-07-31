///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   include/sysutil.h
// Purpose:     utility functions for various OS-level functionality
// Author:      Karsten Ballüder, Vadim Zeitlin
// Created:     1999
// CVS-ID:      $Id$
// Copyright:   (c) 1999 Karsten Ballüder
//              (c) 2000-2006 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef M_SYSUTIL_H
#define M_SYSUTIL_H

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
   /**
      Ctor creates a temporary file name and possibly associates it with the
      specified file.

      If the temp file name is used for writing some data to it, the file
      parameter must be specified as otherwise the operation wouldn't be atomic
      and race conditions could occur. Do close the file before this object
      goes out of scope in this case though as otherwise we could fail to
      delete the temp file under Windows.

      @param file if non-NULL, the file to open (for writing) with the
                  temporary file name
    */
   MTempFileName(wxFile *file = NULL)
      : m_name(wxFileName::CreateTempFileName(_T("Mahogany"), file))
   {
      m_keepFile = false;
   }

   /**
      Ctor from an existing temporary file name.

      The specified file will be deleted when this object is destroyed unless
      Ok() is called.

      @param name of the existing temporary file
    */
   MTempFileName(const String& name) : m_name(name)
   {
      m_keepFile = false;
   }

   /// Returns false if temp file name couldn't be generated
   bool IsOk() const { return !m_name.empty(); }

   /// Get the name of the temp file
   const String& GetName() const { return m_name; }

   /// Tells us not to delete the temp file
   void Ok() { m_keepFile = true; }

   ~MTempFileName()
   {
      if ( !m_keepFile && !m_name.empty() )
      {
         if ( wxRemove(m_name) != 0 )
         {
            wxLogDebug(_T("Stale temp file '%s' left."), m_name.c_str());
         }
      }
   }

private:
   String m_name;
   bool   m_keepFile;
};

/**
   Helper function to create the full command string from the name of an
   external command and the given parameter.

   @param command
      The external command. It may include "%s" to be replaced by the parameter
      value or not, in the latter case the parameter is appended to the end of
      the command.
   @param parameter
      The parameter to be passed to the external command.
 */
String ExpandExternalCommand(String command, const String& parameter);

//@}

#endif // M_SYSUTIL_H

