/////////////////////////////////////////////////////////////////////////////
// Name:        common/mimetype.cpp
// Purpose:     classes and functions to manage MIME types
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.09.98
// RCS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows license (part of wxExtra library)
/////////////////////////////////////////////////////////////////////////////

#ifdef    __GNUG__
    #pragma implementation "mimetype.h"
#endif

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// wxWindows
#ifndef WX_PRECOMP
    #include  "wx/string.h"
#endif //WX_PRECOMP

#include "wx/log.h"
#include "wx/intl.h"

#ifdef __WXMSW__
    #include "wx/msw/registry.h"
#else  // Unix
    #include "wx/textfile.h"
#endif // OS

#include "wx/mimetype.h"

// other standard headers
#include <ctype.h>

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// implementation classes, platform dependent
#ifdef __WXMSW__

// These classes use Windows registry to retrieve the required information.
//
// Keys used (not all of them are documented, so it might actually stop working
// in futur versions of Windows...):
//  1. "HKCR\MIME\Database\Content Type" contains subkeys for all known MIME
//     types, each key has a string value "Extension" which gives (dot preceded)
//     extension for the files of this MIME type.
//
//  2. "HKCR\.ext" contains
//   a) unnamed value containing the "filetype"
//   b) value "Content Type" containing the MIME type
//
// 3. "HKCR\filetype" contains
//   a) unnamed value containing the description
//   b) subkey "DefaultIcon" with single unnamed value giving the icon index in
//      an icon file
//   c) shell\open\command and shell\open\print subkeys containing the commands
//      to open/print the file (the positional parameters are introduced by %1,
//      %2, ... in these strings, we change them to %s ourselves)

class wxFileTypeImpl
{
public:
    // ctor
    wxFileTypeImpl() { }

    // initialize us with our file type name
    void SetFileType(const wxString& strFileType)
        { m_strFileType = strFileType; }
    void SetExt(const wxString& ext)
        { m_ext = ext; }

    // implement accessor functions
    bool GetExtensions(wxArrayString& extensions);
    bool GetMimeType(wxString *mimeType) const;
    bool GetIcon(wxIcon *icon) const;
    bool GetDescription(wxString *desc) const;
    bool GetOpenCommand(wxString *openCmd) const
        { return GetCommand(openCmd, "open"); }
    bool GetPrintCommand(wxString *printCmd) const
        { return GetCommand(printCmd, "print"); }

private:
    // helper function
    bool GetCommand(wxString *command, const char *verb) const;

    wxString m_strFileType, m_ext;
};

class wxMimeTypesManagerImpl
{
public:
    // nothing to do here, we don't load any data but just go and fetch it from
    // the registry when asked for
    wxMimeTypesManagerImpl() { }

    // implement containing class functions
    wxFileType *GetFileTypeFromExtension(const wxString& ext);
    wxFileType *GetFileTypeFromMimeType(const wxString& mimeType);

    // this are NOPs under Windows
    void ReadMailcap(const wxString& filename) { }
    void ReadMimeTypes(const wxString& filename) { }
};

#else  // Unix

// this class uses both mailcap and mime.types to gather information about file
// types.
//
// FIXME any docs with real descriptions of these files??
//
// Format of mailcap file: spaces are ignored, each line is either a comment
// (starts with '#') or a line of the form <mime type>;<command>;<flags>. A
// backslash can be used to quote semicolons and newlines (and probably anything
// else as well). Any of 2 parts of the MIME type may be '*' used as a wildcard.
//
// There are 2 possible formats for mime.types file, one entry per line (used
// for global mime.types) and "expanded" format where an entry takes multiple
// lines (used for users mime.types).
//
// For both formats spaces are ignored and lines starting with a '#' are
// comments. Each record has one of two following forms:
//  a) for "brief" format:  <mime type>  <space separated list of extensions>
//  b) for "expanded" format:
//      type=<mime type> \ 
//      desc="<description>" \ 
//      exts="ext"
//
// We try to autodetect the format of mime.types: if a non-comment line starts
// with "type=" we assume the second format, otherwise the first one.
class wxMimeTypesManagerImpl
{
friend class wxFileTypeImpl; // give it access to m_aXXX variables

public:
    // ctor loads all info into memory for quicker access later on
    // @@ it would be nice to load them all, but parse on demand only...
    wxMimeTypesManagerImpl();

    // implement containing class functions
    wxFileType *GetFileTypeFromExtension(const wxString& ext);
    wxFileType *GetFileTypeFromMimeType(const wxString& mimeType);

    void ReadMailcap(const wxString& filename);
    void ReadMimeTypes(const wxString& filename);

    // accessors
        // get the string containing space separated extensions for the given
        // file type
    wxString GetExtension(size_t index) { return m_aExtensions[index]; }

private:
    wxArrayString m_aTypes,         // MIME types
                  m_aCommands,      // commands to handle them
                  m_aDescriptions,  // descriptions (just some text)
                  m_aExtensions;    // space separated list of extensions
};

class wxFileTypeImpl
{
public:
    // initialization functions
    void Init(wxMimeTypesManagerImpl *manager, size_t index)
        { m_manager = manager; m_index = index; }

    // accessors
    bool GetExtensions(wxArrayString& extensions);
    bool GetMimeType(wxString *mimeType) const
        { *mimeType = m_manager->m_aTypes[m_index]; return TRUE; }
    bool GetIcon(wxIcon *icon) const
        { return FALSE; }   // @@ maybe with Gnome/KDE integration...
    bool GetDescription(wxString *desc) const
        { *desc = m_manager->m_aDescriptions[m_index]; return TRUE; }
    bool GetOpenCommand(wxString *openCmd) const
        { *openCmd = m_manager->m_aCommands[m_index]; return TRUE; }
    bool GetPrintCommand(wxString *printCmd) const
        { return FALSE; }   // @@ how to print files under Unix?

private:
    wxMimeTypesManagerImpl *m_manager;
    size_t                  m_index; // in the wxMimeTypesManagerImpl arrays
};

#endif // OS type

// ============================================================================
// implementation of the wrapper classes
// ============================================================================

// ----------------------------------------------------------------------------
// wxFileType
// ----------------------------------------------------------------------------

wxString wxFileType::ExpandCommand(const wxString& command,
                                   const wxString& filename,
                                   const wxString& mimetype)
{
    wxString str;
    for ( const char *pc = command.c_str(); *pc != '\0'; pc++ )
    {
        if ( *pc == '%' )
        {
            switch ( *++pc ) {
                case 's':
                    // '%s' expands into file name (quoted because it might
                    // contain spaces) - except if there are already quotes
                    // there because otherwise some programs may get confused by
                    // double double quotes
                    if ( *(pc - 2) == '"' )
                        str << filename;
                    else
                        str << '"' << filename << '"';
                    break;

                case 't':
                    // '%t' expands into MIME type (quote it too just to be
                    // consistent)
                    str << '"' << mimetype << '"';
                    break;

                default:
                    str << *pc;
            }
        }
        else
        {
            str << *pc;
        }
    }

    return str;
}

wxFileType::wxFileType()
{
    m_impl = new wxFileTypeImpl;
}

wxFileType::~wxFileType()
{
    delete m_impl;
}

bool wxFileType::GetExtensions(wxArrayString& extensions)
{
    return m_impl->GetExtensions(extensions);
}

bool wxFileType::GetMimeType(wxString *mimeType) const
{
    return m_impl->GetMimeType(mimeType);
}

bool wxFileType::GetIcon(wxIcon *icon) const
{
    return m_impl->GetIcon(icon);
}

bool wxFileType::GetDescription(wxString *desc) const
{
    return m_impl->GetDescription(desc);
}

bool wxFileType::GetOpenCommand(wxString *openCmd) const
{
    return m_impl->GetOpenCommand(openCmd);
}

bool wxFileType::GetPrintCommand(wxString *printCmd) const
{
    return m_impl->GetPrintCommand(printCmd);
}

// ----------------------------------------------------------------------------
// wxMimeTypesManager
// ----------------------------------------------------------------------------

wxMimeTypesManager::wxMimeTypesManager()
{
    m_impl = new wxMimeTypesManagerImpl;
}

wxMimeTypesManager::~wxMimeTypesManager()
{
    delete m_impl;
}

wxFileType *
wxMimeTypesManager::GetFileTypeFromExtension(const wxString& ext)
{
    return m_impl->GetFileTypeFromExtension(ext);
}

wxFileType *
wxMimeTypesManager::GetFileTypeFromMimeType(const wxString& mimeType)
{
    return m_impl->GetFileTypeFromMimeType(mimeType);
}

// ============================================================================
// real (OS specific) implementation
// ============================================================================

#ifdef __WXMSW__

bool wxFileTypeImpl::GetCommand(wxString *command, const char *verb) const
{
    // suppress possible error messages
    wxLogNull nolog;
    wxString strKey;
    strKey << m_strFileType << "\\shell\\" << verb << "\\command";
    wxRegKey key(wxRegKey::HKCR, strKey);

    if ( key.Open() ) {
        // it's the default value of the key
        if ( key.QueryValue("", *command) ) {
            // transform it from '%1' to '%s' style format string
            // @@ we don't make any attempt to verify that the string is valid,
            //    i.e. doesn't contain %2, or second %1 or .... But we do make
            //    sure that we return a string with _exactly_ one '%s'!
            size_t len = command->Len();
            for ( size_t n = 0; n < len; n++ ) {
                if ( command->GetChar(n) == '%' &&
                     (n + 1 < len) && command->GetChar(n + 1) == '1' ) {
                    // replace it with '%s'
                    command->SetChar(n + 1, 's');

                    return TRUE;
                }
            }

            // we didn't find any '%1'!
            // @@@ hack: append the filename at the end, hope that it will do
            *command << " %s";

            return TRUE;
        }
    }

    // no such file type or no value
    return FALSE;
}

// @@ this function is half implemented
bool wxFileTypeImpl::GetExtensions(wxArrayString& extensions)
{
    if ( m_ext.IsEmpty() ) {
        // the only way to get the list of extensions from the file type is to
        // scan through all extensions in the registry - too slow...
        return FALSE;
    }
    else {
        extensions.Empty();
        extensions.Add(m_ext);

        // it's a lie too, we don't return _all_ extensions...
        return TRUE;
    }
}

bool wxFileTypeImpl::GetMimeType(wxString *mimeType) const
{
    // suppress possible error messages
    wxLogNull nolog;
    wxRegKey key(wxRegKey::HKCR, m_strFileType);
    if ( key.Open() && key.QueryValue("Content Type", *mimeType) ) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

bool wxFileTypeImpl::GetIcon(wxIcon *icon) const
{
    wxString strIconKey;
    strIconKey << m_strFileType << "\\DefaultIcon";

    // suppress possible error messages
    wxLogNull nolog;
    wxRegKey key(wxRegKey::HKCR, strIconKey);

    if ( key.Open() ) {
        wxString strIcon;
        // it's the default value of the key
        if ( key.QueryValue("", strIcon) ) {
            // the format is the following: <full path to file>, <icon index>
            // NB: icon index may be negative as well as positive and the full
            //     path may contain the environment variables inside '%'
            wxString strFullPath = strIcon.Before(','),
            strIndex = strIcon.Right(',');

            // index may be omitted, in which case Before(',') is empty and
            // Right(',') is the whole string
            if ( strFullPath.IsEmpty() ) {
                strFullPath = strIndex;
                strIndex = "0";
            }

            wxString strExpPath = wxExpandEnvVars(strFullPath);
            int nIndex = atoi(strIndex);

            HICON hIcon = ExtractIcon(GetModuleHandle(NULL), strExpPath, nIndex);
            switch ( (int)hIcon ) {
                case 0: // means no icons were found
                case 1: // means no such file or it wasn't a DLL/EXE/OCX/ICO/...
                    wxLogDebug("incorrect registry entry '%s': no such icon.",
                               key.GetName().c_str());
                    break;

                default:
                    icon->SetHICON((WXHICON)hIcon);
                    return TRUE;
            }
        }
    }

    // no such file type or no value or incorrect icon entry
    return FALSE;
}

bool wxFileTypeImpl::GetDescription(wxString *desc) const
{
    // suppress possible error messages
    wxLogNull nolog;
    wxRegKey key(wxRegKey::HKCR, m_strFileType);

    if ( key.Open() ) {
        // it's the default value of the key
        if ( key.QueryValue("", *desc) ) {
            return TRUE;
        }
    }

    return FALSE;
}

// extension -> file type
wxFileType *
wxMimeTypesManagerImpl::GetFileTypeFromExtension(const wxString& ext)
{
    // add the leading point if necessary
    wxString str;
    if ( ext[0u] != '.' ) {
        str = '.';
    }
    str << ext;

    // suppress possible error messages
    wxLogNull nolog;

    wxString strFileType;
    wxRegKey key(wxRegKey::HKCR, str);
    if ( key.Open() ) {
        // it's the default value of the key
        if ( key.QueryValue("", strFileType) ) {
            // create the new wxFileType object
            wxFileType *fileType = new wxFileType;
            fileType->m_impl->SetFileType(strFileType);
            fileType->m_impl->SetExt(ext);

            return fileType;
        }
    }

    // unknown extension
    return NULL;
}

// MIME type -> extension -> file type
wxFileType *
wxMimeTypesManagerImpl::GetFileTypeFromMimeType(const wxString& mimeType)
{
    // @@@ I don't know of any official documentation which mentions this
    //     location, but as a matter of fact IE uses it, so why not we?
    static const char *szMimeDbase = "MIME\\Database\\Content Type\\";

    wxString strKey = szMimeDbase;
    strKey << mimeType;

    // suppress possible error messages
    wxLogNull nolog;

    wxString ext;
    wxRegKey key(wxRegKey::HKCR, strKey);
    if ( key.Open() ) {
        if ( key.QueryValue("Extension", ext) ) {
            return GetFileTypeFromExtension(ext);
        }
    }

    // unknown MIME type
    return NULL;
}

#else  // Unix

bool wxFileTypeImpl::GetExtensions(wxArrayString& extensions)
{
    wxString strExtensions = m_manager->GetExtension(m_index);
    extensions.Empty();

    wxString strExt;    // one extension in the space delimitid list
    size_t len = strExtensions.Len();
    for ( size_t n = 0; n < len; n++ ) {
        char ch = strExtensions[n];
        if ( ch == ' ' ) {
            if ( !strExt.IsEmpty() ) {
                extensions.Add(strExt);
                strExt.Empty();
            }
            //else: repeated spaces (shouldn't happen, but it's not that
            //      important if it does happen)
        }
        else if ( ch == '.' ) {
            // remove the dot from extension (but only if it's the first char)
            if ( !strExt.IsEmpty() ) {
                strExt += '.';
            }
            //else: no, don't append it
        }
        else {
            strExt += ch;
        }
    }

    return TRUE;
}

// read system and user mailcaps (TODO implement mime.types support)
wxMimeTypesManagerImpl::wxMimeTypesManagerImpl()
{
    // directories where we look for mailcap and mime.types by default
    static const char *aStandardLocations[] =
    {
        "/etc",
        "/usr/etc",
        "/usr/local/etc",
    };

    // first read the system wide file(s)
    for ( size_t n = 0; n < WXSIZEOF(aStandardLocations); n++ ) {
        wxString dir = aStandardLocations[n];

        wxString file = dir + "/mailcap";
        if ( wxFile::Exists(file) ) {
            ReadMailcap(file);
        }

        file = dir + "/mime.types";
        if ( wxFile::Exists(file) ) {
            ReadMimeTypes(file);
        }
    }

    wxString strHome = getenv("HOME");

    // and now the users mailcap
    wxString strUserMailcap = strHome + "/.mailcap";
    if ( wxFile::Exists(strUserMailcap) ) {
        ReadMailcap(strUserMailcap);
    }

    // read the users mime.types
    wxString strUserMimeTypes = strHome + "/.mime.types";
    if ( wxFile::Exists(strUserMimeTypes) ) {
        ReadMimeTypes(strUserMimeTypes);
    }
}

wxFileType *
wxMimeTypesManagerImpl::GetFileTypeFromExtension(const wxString& ext)
{
    wxFAIL_MSG("not implemented (must parse mime.types)");

    return NULL;
}

wxFileType *
wxMimeTypesManagerImpl::GetFileTypeFromMimeType(const wxString& mimeType)
{
    // first look for an exact match
    int index = m_aTypes.Index(mimeType);
    if ( index == NOT_FOUND ) {
        // then try to find "text/*" as match for "text/plain" (for example)
        wxString strCategory = mimeType.Before('/');
        wxCHECK_MSG( !strCategory.IsEmpty(), NULL,
                     "MIME type in wrong format" );

        size_t nCount = m_aTypes.Count();
        for ( size_t n = 0; n < nCount; n++ ) {
            if ( (m_aTypes[n].Before('/').CmpNoCase(strCategory) == 0) &&
                 m_aTypes[n].Right('/') == "*" ) {
                    index = n;
                    break;
            }
        }
    }

    if ( index != NOT_FOUND ) {
        wxFileType *fileType = new wxFileType;
        fileType->m_impl->Init(this, index);

        return fileType;
    }
    else {
        // not found...
        return NULL;
    }
}

void wxMimeTypesManagerImpl::ReadMimeTypes(const wxString& strFileName)
{
    wxTextFile file(strFileName);
    if ( !file.Open() )
        return;

    size_t nLineCount = file.GetLineCount();
    for ( size_t nLine = 0; nLine < nLineCount; nLine++ ) {
        // now we're at the start of the line
        const char *pc = file[nLine].c_str();

        // skip whitespace
        while ( isspace(*pc) )
            pc++;

        // comment?
        if ( *pc == '#' )
            continue;

        // the information we extract
        wxString strMimeType, strDesc, strExtensions;

        // detect file format
        const char *pEqualSign = strchr(pc, '=');
        if ( pEqualSign == NULL ) {
            // brief format
            // ------------

            // first field is mime type
            for ( strMimeType.Empty(); !isspace(*pc) && *pc != '\0'; pc++ ) {
                strMimeType += *pc;
            }

            // skip whitespace
            while ( isspace(*pc) )
                pc++;

            // take all the rest of the string
            strExtensions = pc;

            // no description...
            strDesc.Empty();
        }
        else {
            // expanded format
            // ---------------

            // the string on the left of '='
            wxString strLHS(pc, pEqualSign - pc);

            // get the RHS
            wxString strRHS;
            bool bInQuotes = FALSE;
            for ( pc = pEqualSign + 1; *pc != '\0' && !isspace(*pc); pc++ ) {
                if ( *pc == '"' ) {
                    if ( bInQuotes ) {
                        // ok, it's the end of the quoted string
                        pc++;
                        bInQuotes = FALSE;
                        break;
                    }
                    else {
                        // the string is quoted
                        bInQuotes = TRUE;
                    }
                }
                else {
                    strRHS += *pc;
                }
            }

            // check that it's more or less what we're waiting for
            if ( bInQuotes ) {
                wxLogWarning(_("Unterminated quote in file %s, line %d."),
                             strFileName.c_str(), nLine);
            }
            else {
                // only '\' should be left on the line
                while ( isspace(*pc) )
                    pc++;

                // only '\\' may be left on the line normally
                if ( (*pc != '\n') && ((*pc != '\\') || (*++pc != '\n')) ) {
                    wxLogWarning(_("Extra characters in file %s, line %d."),
                                 strFileName.c_str(), nLine);
                }
            }

            // now see what we got
            if ( strLHS == "type" ) {
                strMimeType = strLHS;
            }
            else if ( strLHS == "desc" ) {
                strDesc = strLHS;
            }
            else if ( strLHS == "exts" ) {
                strExtensions = strLHS;
            }
            else {
                wxLogWarning(_("Unknown field in file %s, line %d: '%s'."),
                             strFileName.c_str(), nLine, strLHS.c_str());
            }

            // as we don't reset strMimeType, the next line in this entry will
            // be interpreted correctly.
        }

        int index = m_aTypes.Index(strMimeType);
        if ( index == NOT_FOUND ) {
            // add a new entry
            m_aTypes.Add(strMimeType);
            m_aCommands.Add("");
            m_aExtensions.Add(strExtensions);
            m_aDescriptions.Add(strDesc);
        }
        else {
            // modify an existing one
            if ( !strDesc.IsEmpty() ) {
                m_aDescriptions[index] = strDesc;   // replace old value
            }
            m_aExtensions[index] += strExtensions;
        }
    }

    // check our data integriry
    wxASSERT( m_aTypes.Count() == m_aCommands.Count() &&
              m_aTypes.Count() == m_aExtensions.Count() &&
              m_aTypes.Count() == m_aDescriptions.Count() );
}

void wxMimeTypesManagerImpl::ReadMailcap(const wxString& strFileName)
{
    wxTextFile file(strFileName);
    if ( !file.Open() )
        return;

    size_t nLineCount = file.GetLineCount();
    for ( size_t nLine = 0; nLine < nLineCount; nLine++ ) {
        // now we're at the start of the line
        const char *pc = file[nLine].c_str();

        // skip whitespace
        while ( isspace(*pc) )
            pc++;

        // comment?
        if ( *pc == '#' )
            continue;

        // no, do parse

        // what field are we currently in? Invalid means that we went beyond the
        // last known one and there is some garbage (at least for us) left on
        // the line
        enum
        {
            Token_Type,
            Token_Command,
            Token_Flags,
            Token_Invalid
        } currentToken = Token_Type;

        // if it's NOT_FOUND, it means that this entry is new, otherwise we'd
        // already seen it before
        int nIndex = NOT_FOUND;

        wxString strCurrentToken; // accumulator
        for ( ; *pc != '\0'; pc++ ) {
            switch ( *pc ) {
                case '\\':
                    // interpret the next character literally
                    strCurrentToken += *++pc;
                    break;

                case ';':
                    // store this token and start looking for the next one
                    {
                        strCurrentToken.Trim(); // whitespaces
                        switch ( currentToken ) {
                            case Token_Type:
                                // try to find it first
                                nIndex = m_aTypes.Index(strCurrentToken);
                                if ( nIndex == NOT_FOUND ) {
                                    m_aTypes.Add(strCurrentToken);
                                    m_aExtensions.Add("");
                                }
                                //else: nothing, modify other fields later

                                currentToken = Token_Command;
                                break;

                            // modify the existing entry or add the new one
                            // depending on nIndex variable (set just above)
                            case Token_Command:
                                if ( nIndex == NOT_FOUND ) {
                                    m_aCommands.Add(strCurrentToken);
                                }
                                else {
                                    m_aCommands[nIndex] = strCurrentToken;
                                }

                                currentToken = Token_Flags;
                                break;

                            case Token_Flags:
                                // we ignore the flags

                                currentToken = Token_Invalid;
                                break;

                            case Token_Invalid:
                                wxLogWarning
                                (
                                  _("Mailcap file %s, line %d: extra fields "
                                    "for the MIME type '%s' ignored."),
                                  strFileName.c_str(),
                                  nLine,
                                  (nIndex == NOT_FOUND ? m_aCommands.Last()
                                                       :
                                                       m_aCommands[nIndex]).c_str()
                                );
                                break;

                            default:
                                wxFAIL_MSG("unknown field type in mailcap");
                        }

                        // next token starts immediately after ';'
                        strCurrentToken.Empty();
                    }
                    break;

                default:
                    strCurrentToken += *pc;
            }
        }
    }

    // check our data integriry
    wxASSERT( m_aTypes.Count() == m_aCommands.Count() &&
              m_aTypes.Count() == m_aExtensions.Count() &&
              m_aTypes.Count() == m_aDescriptions.Count() );
}

#endif // OS type

/* vi: set cin tw=80 ts=4 sw=4: */

