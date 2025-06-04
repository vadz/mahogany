/*-*- c++ -*-********************************************************
 * wxlparser.h : parsers,  import/export for wxLayoutList           *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/
#ifndef WXLPARSER_H
#   define   WXLPARSER_H

#include <wx/fontenc.h> // enum wxFontEncoding declaration

#include "gui/wxllist.h"

#ifndef   NULL
#   define   NULL 0
#endif

enum wxLayoutExportType
{
   WXLO_EXPORT_TEXT,
   WXLO_EXPORT_HTML,
   WXLO_EXPORT_OBJECT,
   // this can be caused by empty lines:
   WXLO_EXPORT_EMPTYLINE
};

enum wxLayoutExportMode
{
   WXLO_EXPORT_AS_TEXT = 0x00,
   WXLO_EXPORT_AS_TEXT_AND_COMMANDS = 0x01,
   WXLO_EXPORT_AS_HTML = 0x02,
   WXLO_EXPORT_AS_OBJECTS = 0x03,

   // non 0:
   WXLO_EXPORT_WITH_CRLF = 0x10,
   WXLO_EXPORT_WITH_LF_ONLY = 0x20
};

struct wxLayoutExportObject
{
   wxLayoutExportType type;
   union
   {
      wxString           *text;
      wxLayoutObject *object;
   }content;
   ~wxLayoutExportObject()
      {
         if(type == WXLO_EXPORT_TEXT || type == WXLO_EXPORT_HTML)
            delete content.text;
      }
};


struct wxLayoutExportStatus
{
   wxLayoutExportStatus(wxLayoutList *list);
   wxLayoutLine      * m_line;
   wxLOiterator        m_iterator;
   wxLayoutStyleInfo   m_si;
   bool                m_FirstTime;
   inline bool NULLIT(void) const { return m_iterator == m_line->NULLIT(); }
};

#ifdef OS_WIN
#   define WXLO_DEFAULT_EXPORT_MODE WXLO_EXPORT_WITH_CRLF
#else // Unix
#   define WXLO_DEFAULT_EXPORT_MODE WXLO_EXPORT_WITH_LF_ONLY
#endif // Win/Unix

/// import text in given encoding into a wxLayoutList (including linefeeds):
extern void wxLayoutImportText(wxLayoutList *list,
                               wxString const &str,
                               wxFontEncoding encoding = wxFONTENCODING_SYSTEM);

/// import text in given encoding into a wxLayoutList (including linefeeds):
extern void wxLayoutImportHTML(wxLayoutList *list,
                               wxString const &str,
                               wxFontEncoding encoding = wxFONTENCODING_SYSTEM);

/// export text in a given format
wxLayoutExportObject *wxLayoutExport(wxLayoutExportStatus *status,
                                     int mode = WXLO_EXPORT_AS_TEXT,
                                     int flags = WXLO_DEFAULT_EXPORT_MODE);

#endif //WXLPARSER_H
