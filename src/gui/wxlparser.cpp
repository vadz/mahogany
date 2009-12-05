/*-*- c++ -*-********************************************************
 * wxlparser.cpp : parsers,  import/export for wxLayoutList           *
 *                                                                  *
 * (C) 1998,1999 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"

#  include <wx/wxchar.h>               // for wxPrintf/Scanf
#endif // USE_PCH

#include "gui/wxllist.h"
#include "gui/wxlparser.h"

#include <wx/fontmap.h>
#include <wx/encconv.h>

#define   BASE_SIZE 12

static inline bool LayoutParserIsEndOfLine(const wxChar *p)
{
   // the end of line is either just '\n' or "\r\n" or even '\r' - we
   // understand Unix, DOS and Mac conventions here as we get all kinds of text
   // in the emails
   return (*p == '\n') || (*p == '\r');
}

static void SetEncoding(wxLayoutList *list,
                        wxFontEncoding encoding,
                        bool *useConverter,
                        wxEncodingConverter *conv)
{
   // check that we have fonts available for this encoding if it is a non
   // default one
   *useConverter = FALSE;
   if ( encoding != wxFONTENCODING_SYSTEM )
   {
      if ( !wxFontMapper::Get()->IsEncodingAvailable(encoding) )
      {
         // try to find another encoding
         wxFontEncoding encAlt;
         if ( wxFontMapper::Get()->GetAltForEncoding(encoding, &encAlt) )
         {
            if ( conv->Init(encoding, encAlt) )
            {
               *useConverter = TRUE;
               encoding = encAlt;
            }
            else
            {
               // TODO give an error message here

               // don't attempt to do anything with this encoding
               encoding = wxFONTENCODING_SYSTEM;
            }
         }
      }

      if ( encoding != wxFONTENCODING_SYSTEM )
      {
         // set the font for this encoding
         list->SetFontEncoding(encoding);
      }
   }

}

static void wxLayoutImportTextInternal(wxLayoutList *list,
                                       const wxString& str,
                                       bool useConverter,
                                       wxEncodingConverter &conv)
{
   wxString s;
   s.Alloc(str.length());

   for ( const wxChar *cptr = str.c_str(); *cptr; cptr++ )
   {
      while ( *cptr && !LayoutParserIsEndOfLine(cptr) )
      {
         s += *cptr++;
      }

      // wxLayoutWindow doesn't show tabs correctly, so turn them into spaces
      s.Replace(_T("\t"), _T("        "));

      if ( useConverter )
         list->Insert(conv.Convert(s));
      else
         list->Insert(s);

      if ( !*cptr )
      {
         // it was the end of text
         break;
      }

      s.clear();

      // what kind of end of line did we find?
      if ( *cptr == '\r' )
      {
         // I've seen some weird messages with "\r\r\n" which should be
         // indicated as one line break, not 2 in this case
         if ( cptr[1] == '\r' )
         {
            cptr++;
         }

         // if it was "\r\n", skip the following '\n'
         if ( cptr[1] == '\n' )
         {
            cptr++;
         }
      }

      list->LineBreak();
   }
}

void wxLayoutImportHTML(wxLayoutList *list,
                        wxString const &str,
                        wxFontEncoding encoding)
{
  // Strip URLs:
  wxString filtered;
  const wxChar *cptr = str.c_str();
  bool inTag = FALSE;
  while(*cptr)
  {
     if(*cptr == '<')
     {
        inTag = TRUE;
        cptr++;
        // process known tags
        continue;
     }
     if(inTag && *cptr == '>')
     {
        inTag = FALSE;
        cptr++;
        continue;
     }
     if(*cptr == '&') // ignore it
     {
        if( wxStrncmp(cptr+1, _T("nbsp;"), 5) == 0)
        {
           cptr += 6;
           filtered += ' ';
           continue;
        }
        while(*cptr && *cptr != ';')
           cptr++;
        if(*cptr == ';')
           cptr++;
        continue;
     }
     if(!inTag)
        filtered += *cptr;
     cptr++;
  }
  wxLayoutImportText(list, filtered, encoding);
}

void wxLayoutImportText(wxLayoutList *list,
                        const wxString& str,
                        wxFontEncoding encoding)
{
   if ( str.empty() )
      return;

   bool useConverter = FALSE;
   wxEncodingConverter conv;
   SetEncoding(list, encoding, &useConverter, &conv);
   wxLayoutImportTextInternal(list, str, useConverter, conv);
}

static
wxString wxLayoutExportCmdAsHTML(wxLayoutObjectCmd const & cmd,
                                 wxLayoutStyleInfo *styleInfo,
                                 bool firstTime)
{
   static wxChar buffer[20];
   wxString html;

   wxLayoutStyleInfo *si = cmd.GetStyle();

   int size, sizecount;

   html += _T("<font ");

   if(si->m_fg_valid)
   {
      html += _T("color=");
      wxSprintf(buffer, _T("\"#%02X%02X%02X\""), si->m_fg.Red(),si->m_fg.Green(),si->m_fg.Blue());
      html += buffer;
   }

   if(si->m_bg_valid)
   {
      html += _T(" bgcolor=");
      wxSprintf(buffer, _T("\"#%02X%02X%02X\""), si->m_bg.Red(),si->m_bg.Green(),si->m_bg.Blue());
      html += buffer;
   }

   switch(si->family)
   {
   case wxSWISS:
   case wxMODERN:
      html += _T(" face=\"Arial,Helvetica\""); break;
   case wxROMAN:
      html += _T(" face=\"Times New Roman, Times\""); break;
   case wxTELETYPE:
      html += _T(" face=\"Courier New, Courier\""); break;
   default:
      ;
   }

   size = BASE_SIZE; sizecount = 0;
   while(size < si->size && sizecount < 5)
   {
      sizecount ++;
      size = (size*12)/10;
   }
   while(size > si->size && sizecount > -5)
   {
      sizecount --;
      size = (size*10)/12;
   }
   html += _T("size=");
   wxSprintf(buffer, _T("%+1d"), sizecount);
   html += buffer;

   html += _T(">");

   if(styleInfo != NULL && ! firstTime)
      html = _T("</font>") + html; // terminate any previous font command

   if((si->weight == wxBOLD) && ( (!styleInfo) || (styleInfo->weight != wxBOLD)))
      html += _T("<b>");
   else
      if(si->weight != wxBOLD && ( styleInfo && (styleInfo->weight == wxBOLD)))
         html += _T("</b>");

   if(si->style == wxSLANT)
      si->style = wxITALIC; // the same for html

   if((si->style == wxITALIC) && ( (!styleInfo) || (styleInfo->style != wxITALIC)))
      html += _T("<i>");
   else
      if(si->style != wxITALIC && ( styleInfo && (styleInfo->style == wxITALIC)))
         html += _T("</i>");

   if(si->underline && ( (!styleInfo) || ! styleInfo->underline))
      html += _T("<u>");
   else if(si->underline == false && ( styleInfo && styleInfo->underline))
      html += _T("</u>");


   *styleInfo = *si; // update last style info

   return html;
}



wxLayoutExportStatus::wxLayoutExportStatus(wxLayoutList *list)
{
   m_si = list->GetDefaultStyleInfo();
   m_line = list->GetFirstLine();
   m_iterator = m_line->GetFirstObject();
   m_FirstTime = TRUE;
}



#define   WXLO_IS_TEXT(type) \
( type == WXLO_TYPE_TEXT \
  || (type == WXLO_TYPE_CMD \
      && mode == WXLO_EXPORT_AS_HTML))


wxLayoutExportObject *wxLayoutExport(wxLayoutExportStatus *status,
                                     int mode, int flags)
{
// FIXME: this badly needs to be re-written
   wxASSERT(status);
   wxLayoutExportObject * exp;

   if (!status->m_line)
      return NULL;
   if(status->NULLIT()) // end of line
   {
      if(status->m_line->GetNextLine() == NULL)
         // reached end of list
         return NULL;
   }
   exp = new wxLayoutExportObject();
   wxLayoutObjectType type;
   if(!status->NULLIT())
   {
      type = (** status->m_iterator).GetType();
      if( mode == WXLO_EXPORT_AS_OBJECTS || ! WXLO_IS_TEXT(type)) // simple case
      {
         exp->type = WXLO_EXPORT_OBJECT;
         exp->content.object = *status->m_iterator;
         status->m_iterator++;
         return exp;
      }
   }
   else
   {  // iterator == NULLIT
      if(mode == WXLO_EXPORT_AS_OBJECTS)
      {
         exp->type = WXLO_EXPORT_EMPTYLINE;
         exp->content.object = NULL; //empty line
         status->m_line = status->m_line->GetNextLine();
         if(status->m_line)
            status->m_iterator = status->m_line->GetFirstObject();
         return exp;
      }
      else
         type = WXLO_TYPE_TEXT;
   }

   wxString *str = new wxString();
   // text must be concatenated
   for(;;)
   {
      while(status->NULLIT())
      {
         if(mode & WXLO_EXPORT_AS_HTML)
            *str += _T("<br>");
         if(flags & WXLO_EXPORT_WITH_CRLF)
            *str += _T("\r\n");
         else
            *str += _T('\n');

         status->m_line = status->m_line->GetNextLine();
         if(!status->m_line)
            break; // end of list
         status->m_iterator = status->m_line->GetFirstObject();
      }
      if(!status->m_line)  // reached end of list, fall through
         break;
      type = (** status->m_iterator).GetType();
      if(type == WXLO_TYPE_ICON)
         break;
      switch(type)
      {
      case WXLO_TYPE_TEXT:
         *str += ((wxLayoutObjectText *)*status->m_iterator)->GetText();
         break;
      case WXLO_TYPE_CMD:
         if(mode == WXLO_EXPORT_AS_HTML)
            *str += wxLayoutExportCmdAsHTML(
               *(wxLayoutObjectCmd const *)*status->m_iterator,
               & status->m_si, status->m_FirstTime);
         status->m_FirstTime = FALSE;
         break;
      default:  // ignore icons
         ;
      }
      status->m_iterator++;
   }
   exp->type = (mode == WXLO_EXPORT_AS_HTML)
      ?  WXLO_EXPORT_HTML : WXLO_EXPORT_TEXT;
   exp->content.text = str;
   return exp;
}

