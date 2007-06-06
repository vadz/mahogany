///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   CacheFile.h: base class for various cache files we have
// Purpose:     currently combines common code of MFCache.cpp and Pop3.cpp
// Author:      Vadim Zeitlin
// Modified by:
// Created:     15.10.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _M_CACHEFILE_H_
#define _M_CACHEFILE_H_

class WXDLLEXPORT wxTempFile;
class WXDLLEXPORT wxTextFile;

// ----------------------------------------------------------------------------
// CacheFile
// ----------------------------------------------------------------------------

class CacheFile
{
   // no public methods nor data

protected:
   /// protected ctor
   CacheFile() { }

   /**
     These methods are called by the derived class and call back DoLoad/Save()
     after performing the initial setup.
    */
   //@{

   /// call this to load the file (by calling DoLoad())
   virtual bool Load();

   /// call this to save the file (by calling DoSave())
   virtual bool Save();

   /**
     Check that header version is correct, return a positive value to continue,
     0 to stop right now or -1 to skip reading the file but return success:
     this is what we do if we detect that file has a version more recent than
     the version supported by this code.
    */
   virtual int CheckFormatVersion(const String& hdr, int *version) const;

   //@}

   /**
     Callback methods used to query information about the file format.
    */
   //@{

   /// implement this function to return the (base) name of the cache file
   virtual String GetFileName() const = 0;

   /// implement this function to return the header for the cache file
   virtual String GetFileHeader() const = 0;

   /// get the major and minor versions of the cache file format
   virtual int GetFormatVersion() const = 0;

   //@}

   /**
     Callback methods used to do the real work
    */
   //@{

   /// read all data from file (start from line 1, line 0 is the header)
   virtual bool DoLoad(const wxTextFile& file, int version) = 0;

   /// read all data from file (header had been already written)
   virtual bool DoSave(wxTempFile& file) = 0;

   //@}

   /**
     Static helper functions
    */
   //@{

   /// return the name of the direcotry to use for the cache files
   static String GetCacheDirName();

   /// split an int version into major and minor parts
   static void SplitVersion(int version, int& verMaj, int& verMin);

   /// combine major and minor versions into one int
   static int BuildVersion(int verMaj, int varMin);

   //@}

   /// virtual dtor
   virtual ~CacheFile() { }

private:
   /// copying the objects of this class is forbidden
   CacheFile(const CacheFile&);
   CacheFile& operator=(const CacheFile&);
};

#endif // _M_CACHEFILE_H_

