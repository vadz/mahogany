///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   UUDecode.cpp: implementation of UU decoding viewer filter
// Purpose:     Decoding attachments
// Author:      Xavier Nodet
// Modified by:
// Created:     11.05.04
// CVS-ID:      $Id$
// Copyright:   (c) 2004 Xavier Nodet <xavier.nodet@free.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#   include "Mcommon.h"
#endif //USE_PCH

#include "MessageViewer.h"
#include "ViewFilter.h"
#include "ClickAtt.h"

#include <wx/bitmap.h>



#include "MimePart.h"
#include <wx/mimetype.h>

// MimePartRaw
//
// A MIME part built from raw data. This class does not know about sub-parts, 
// or other MIME stuff.
//
// It should be possible to re-use this class to handle the result of decrypting
// (part of) a message.

class MimePartRaw : public MimePart
{
public:
  MimePartRaw(const String& fileName, 
              const String& mimeType, 
              const String& content,
              const String& disposition) 
    : m_fileName(fileName)
    , m_mimeType(mimeType)
    , m_content(content)
    , m_disposition(disposition)
  {}
public:
   virtual MimePart *GetParent() const {return 0;}
   virtual MimePart *GetNext() const {return 0;}
   virtual MimePart *GetNested() const {return 0;}

   virtual MimeType GetType() const {
      return MimeType(m_mimeType);
   }
   virtual String GetDescription() const {return String();}
   virtual String GetFilename() const {return m_fileName;}
   virtual String GetDisposition() const {return m_disposition;}
   virtual String GetPartSpec() const {return String();}
   virtual String GetParam(const String& name) const {return String();}
   virtual String GetDispositionParam(const String& name) const {return String();}
   virtual const MimeParameterList& GetParameters() const {return m_parameters;}
   virtual const MimeParameterList& GetDispositionParameters() const {return m_dispositionParameters;}

   /// get the raw (un-decoded) contents of this part
   virtual const void *GetRawContent(unsigned long *len = NULL) const {
      // We actually return the only thing we have: the decoded file.
      return GetContent(len);
   }
   virtual const void *GetContent(unsigned long *len) const {
     if (len) *len = m_content.Length();
     return m_content.c_str();
   }
   virtual String GetTextContent() const {
      // Same as for MimePartCC
      unsigned long len;
      const char *p = reinterpret_cast<const char *>(GetContent(&len));
      if ( !p )
         return wxGetEmptyString();

#if wxUSE_UNICODE
#warning "We need the original encoding here, TODO"
      return wxConvertMB2WX(p);
#else // ANSI
      return wxString(p, len);
#endif // Unicode/ANSI
   }


   virtual String GetHeaders() const {return String();}
   virtual MimeXferEncoding GetTransferEncoding() const {return MIME_ENC_BINARY;}
   virtual size_t GetSize() const {return m_content.length();}

   virtual wxFontEncoding GetTextEncoding() const {return wxFONTENCODING_DEFAULT;}
   virtual size_t GetNumberOfLines() const {return 0;}

private:
  MimeParameterList m_parameters;
  MimeParameterList m_dispositionParameters;

  String m_fileName;
  String m_mimeType;
  String m_content;
  String m_disposition;
};





// ----------------------------------------------------------------------------
// UUDecodeFilter itself
// ----------------------------------------------------------------------------

class UUDecodeFilter : public ViewFilter
{
public:
   UUDecodeFilter(MessageView *msgView, ViewFilter *next, bool enable);

protected:
   virtual void DoProcess(String& text,
                          MessageViewer *viewer,
                          MTextStyle& style);
};

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// all UUencoded data start with a line whose prefix is this
#define UU_BEGIN_PREFIX _T("begin ")

// end of the UU encoded data is with a line equal to this
#define UU_END_PREFIX _T("end")

// ============================================================================
// UUDecodeFilter implementation
// ============================================================================

IMPLEMENT_VIEWER_FILTER(UUDecodeFilter,
                        ViewFilter::Priority_High - 5,
                        true,      // initially disabled
                        gettext_noop("UUdecode"),
                        _T("(c) 2004 Xavier Nodet <xavier.nodet@free.fr>"));

// ----------------------------------------------------------------------------
// UUDecodeFilter ctor
// ----------------------------------------------------------------------------

UUDecodeFilter::UUDecodeFilter(MessageView *msgView, ViewFilter *next, bool enable)
         : ViewFilter(msgView, next, enable)
{}

// ----------------------------------------------------------------------------
// UUDecodeFilter work function
// ----------------------------------------------------------------------------


#define DEC(c)	(((c) - ' ') & 077)

int UUdecodeLine(const wxChar* input, wxChar* output, const wxChar** endOfLine) {
  if  (input[0] > ' ' && input[0] <= '`') {
    char * cv_ptr = output;
    register const wxChar * p=input;

    int cv_len = DEC(*p++);

    /* Actually decode the uue data; ensure characters are in range. */
    for (int i=0; i<cv_len; i+=3, p+=4) {
      if ( (p[0]<' ' || p[0]>'`') ||
            (p[1]<' ' || p[1]>'`') ||
            (p[2]<' ' || p[2]>'`') ||
            (p[3]<' ' || p[3]>'`') ) {
        return -1;
      }
      *cv_ptr++ = DEC(p[0]) << 2 | DEC(p[1]) >> 4;
      *cv_ptr++ = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
      *cv_ptr++ = DEC(p[2]) << 6 | DEC(p[3]);
    }
    if (*p != '\r' && *p != '\n' && *p != '\0')
      return -1;
    //else  *p=0;

    *endOfLine = p;
    return cv_len;
  } else 
    return -1;
}

bool UUdecodeFile(const wxChar* input, String& output, const wxChar** endOfEncodedStream) {

  static const int allocationSize = 10000;

  String decodedFile;
  size_t totalAllocatedBytes = 0;

  size_t totalDecodedBytes = 0;
  int decodedBytesInLine;
  const wxChar* startOfLine = input;
  const wxChar* endOfLine = 0; // init not needed
  wxChar* buffer = new wxChar[45];
  //memset(buffer, '\0', 45);
  while ((decodedBytesInLine = UUdecodeLine(startOfLine, buffer, &endOfLine)) > 0) {
    // We've just decoded a line
    totalDecodedBytes += decodedBytesInLine;
    String decodedLine(buffer, decodedBytesInLine);
    if (decodedFile.length() + (size_t)decodedBytesInLine >= totalAllocatedBytes) {
      totalAllocatedBytes += allocationSize;
      decodedFile.Alloc(totalAllocatedBytes);
    }
    decodedFile << decodedLine;
    ASSERT_MSG( decodedFile.Length() == totalDecodedBytes, 
      _T("Number of decoded bytes does not match!") );
    ASSERT_MSG( endOfLine[0] == '\r', _T("Line has no '\\r' at the end!") );
    ASSERT_MSG( endOfLine[1] == '\n', _T("Line has no '\\n' at the end!") );
    startOfLine = endOfLine + 2;
  }
  delete buffer;
  if (decodedBytesInLine < 0) {
    return false;
  }
  decodedFile.Shrink();
  output = decodedFile;
  *endOfEncodedStream = endOfLine;
  return true;
}



void
UUDecodeFilter::DoProcess(String& text,
                     MessageViewer *viewer,
                     MTextStyle& style)
{
   // do we have something looking like UUencoded data?
   //
   
   const wxChar *start = text.c_str();
   const wxChar *nextToOutput = start;
   while ( *start )
   {

      if ( wxStrncmp(start, UU_BEGIN_PREFIX, wxStrlen(UU_BEGIN_PREFIX)) != 0 )
      {
         // try the next line (but only if not already at the end)
         start = wxStrchr(start, '\n');
         if ( start )
         {
            start++;  // skip '\n' itself
            continue; // restart with next line
         } 
         else 
         {
            String prolog(nextToOutput);
            if ( !prolog.empty() )
            {
               m_next->Process(prolog, viewer, style);
            }
            return;   // no more text
         }
      }

      ASSERT_MSG( wxStrncmp(start, UU_BEGIN_PREFIX, wxStrlen(UU_BEGIN_PREFIX)) == 0,
         _T("Stopped without a seemingly begin line ?") );

      const wxChar* startBeginLine = start;
      start = start + wxStrlen(UU_BEGIN_PREFIX);
      // Let's check that the next 4 chars after the 'begin ' are
      // digit, digit, digit, space
      if ( !(    start[0] >= '0' && start[0] <= '9'
              && start[1] >= '0' && start[1] <= '9'
              && start[2] >= '0' && start[2] <= '9'
              && start[3] == ' ' ) ) 
      {
#if defined(__WXDEBUG__)
         wxLogWarning(_("The BEGIN line is not correctly formed."));
#endif
         continue;
      }

      const wxChar* startName = start+4;
      const wxChar* endName = startName;
      // Rest of the line is the name
      while (*endName != '\r')
      {
         ++endName;
         ASSERT_MSG( endName < text.c_str()+text.Length(), _T("'begin' line does not end?") );
      }
      ASSERT_MSG( endName[1] == '\n', _T("'begin' line does not end with \"\\r\\n\"?") );
      String fileName = String(startName, endName);
      const wxChar* start_data = endName + 2;

      String decodedFile;
      const wxChar* endOfEncodedStream = 0;
      bool ok = UUdecodeFile(start_data, decodedFile, &endOfEncodedStream);
      if ( ok )
      {
         if ( endOfEncodedStream[0] == '\r' &&
            endOfEncodedStream[1] == '\n' &&
            endOfEncodedStream[2] == 'e' &&
            endOfEncodedStream[3] == 'n' &&
            endOfEncodedStream[4] == 'd' )
         {
            endOfEncodedStream += 5;
         }
         // output the part before the BEGIN line, if any
         String prolog(nextToOutput, startBeginLine-2);
         if ( !prolog.empty() )
         {
            m_next->Process(prolog, viewer, style);
         }

         // Let's get a mimeType from the extention
         wxFileType *fileType = mApplication->GetMimeManager().GetFileTypeFromExtension(fileName.AfterLast('.'));
         String mimeType;
         if (!fileType->GetMimeType(&mimeType)) {
            mimeType = _T("APPLICATION/OCTET-STREAM");
         }
         delete fileType;

         MimePartRaw* decodedMime = 
            new MimePartRaw(fileName, mimeType, decodedFile, _T("uuencoded"));
         m_msgView->ShowPart(decodedMime);

         nextToOutput = start = endOfEncodedStream;
      }
      else
      {
#if defined(__WXDEBUG__)
         wxLogWarning(_("This message seems to contain uuencoded data, but in fact it does not."));
#endif
      }
   }
   String prolog(nextToOutput);
   if ( !prolog.empty() )
   {
      m_next->Process(prolog, viewer, style);
   }
}
