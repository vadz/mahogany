/////////////////////////////////////////////////////////////////////////////
// Name:        wx/mimetype.h
// Purpose:     classes and functions to manage MIME types
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.09.98
// RCS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows license (part of wxExtra library)
/////////////////////////////////////////////////////////////////////////////

#ifndef   _MIMETYPE_H
#define   _MIMETYPE_H

// fwd decls
class wxIcon;
class wxFileTypeImpl;
class wxMimeTypesManagerImpl;

// the things we really need
#include "wx/string.h"

// This class holds information about a given "file type". File type is the
// same as MIME type under Unix, but under Windows it corresponds more to an
// extension than to MIME type (in fact, several extensions may correspond to a
// file type). This object may be created in many different ways and depending
// on how it was created some fields may be unknown so the return value of all
// the accessors *must* be checked!
class wxFileType
{
friend wxMimeTypesManagerImpl;  // it has access to m_impl
public:
    // ctor
    wxFileType();

    // accessors: all of them return true if the corresponding information
    // could be retrieved/found, false otherwise (and in this case all [out]
    // parameters are unchanged)
        // return the MIME type for this file type
    bool GetMimeType(wxString *mimeType) const;
        // fill passed in array with all extensions associated with this file
        // type
    bool GetExtensions(wxArrayString& extensions);
        // get the icon corresponding to this file type
    bool GetIcon(wxIcon *icon) const;
        // get a brief file type description ("*.txt" => "text document")
    bool GetDescription(wxString *desc) const;
        // get the command to execute the file of given type, returned string
        // always contains exactly one '%s' printf() format specifier
    bool GetOpenCommand(wxString *openCmd) const;
        // get the command to print the file of given type, returned string
        // always contains exactly one '%s' printf() format specifier
    bool GetPrintCommand(wxString *printCmd) const;

    // operations
        // expand a string in the format of GetOpenCommand (which may contain
        // '%s' and '%t' format specificators for the file name and mime type)
    static wxString ExpandCommand(const wxString& command,
                                  const wxString& filename,
                                  const wxString& mimetype);

    // dtor (not virtual, shouldn't be derived from)
    ~wxFileType();

private:
    // no copy ctor/assignment operator
    wxFileType(const wxFileType&);
    wxFileType& operator=(const wxFileType&);

    wxFileTypeImpl *m_impl;
};

// This class accesses the information about all known MIME types and allows
// the application to retrieve information (including how to handle data of
// given type) about them.
//
// NB: currently it doesn't support modifying MIME database (read-only access).
class wxMimeTypesManager
{
public:
    // ctor
    wxMimeTypesManager();

    // Database lookup: all functions return a pointer to wxFileType object
    // whose methods may be used to query it for the information you're
    // interested in. If the return value is !NULL, caller is responsible for
    // deleting it.
        // get file type from file extension
    wxFileType *GetFileTypeFromExtension(const wxString& ext);
        // get file type from MIME type (in format <category>/<format>)
    wxFileType *GetFileTypeFromMimeType(const wxString& mimeType);

    // other operations
        // read in additional file (the standard ones are read automatically)
        // in mailcap format (see mimetype.cpp for description)
    void ReadMailcap(const wxString& filename);
        // read in additional file in mime.types format
    void ReadMimeTypes(const wxString& filename);

    // dtor (not virtual, shouldn't be derived from)
    ~wxMimeTypesManager();

private:
    // no copy ctor/assignment operator
    wxMimeTypesManager(const wxMimeTypesManager&);
    wxMimeTypesManager& operator=(const wxMimeTypesManager&);

    wxMimeTypesManagerImpl *m_impl;
};

#endif  //_MIMETYPE_H
