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
#   include "MApplication.h"
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
   virtual const void *GetRawContent(unsigned long *len = NULL) const
   {
      // We actually return the only thing we have: the decoded file.
      return GetContent(len);
   }
   virtual const void *GetContent(unsigned long *len) const
   {
     if (len)
        *len = m_content.Length();
     return m_content.c_str();
   }
   virtual String GetTextContent() const
   {
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

// strlen("\r\n")
static const size_t lenEOL = 2;



// ----------------------------------------------------------------------------
// UUDecodeFilter itself
// ----------------------------------------------------------------------------

class UUDecodeFilter : public ViewFilter
{
public:
   UUDecodeFilter(MessageView *msgView, ViewFilter *next, bool enable);
   virtual ~UUDecodeFilter();

protected:
   virtual void DoProcess(String& text,
                          MessageViewer *viewer,
                          MTextStyle& style);

   // all MimePartRaw we create: we add them to this array to free them later
   //
   // unfortunately this is totally bogus as we're normally destroyed only when
   // the program terminates so even if it doesn't appear as the memory leak in
   // the end, it still is one: the MimePartRaws should have been destroyed
   // much earlier... unfortunately it's not clear when, probably they should
   // be attached to the Message and destroyed with it but this is so ugly...
   wxArrayPtrVoid m_mimeParts;
};

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// all UUencoded data start with a line whose prefix is "begin" and preceded by
// a blank line
#define UU_BEGIN_PREFIX _T("\r\nbegin ")

// UUencoded data ends at a line consisting solely of "end"
#define UU_END_PREFIX _T("\r\nend\r\n")

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

UUDecodeFilter::UUDecodeFilter(MessageView *msgView,
                               ViewFilter *next,
                               bool enable)
              : ViewFilter(msgView, next, enable)
{
}

UUDecodeFilter::~UUDecodeFilter()
{
   const size_t count = m_mimeParts.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      delete static_cast<MimePartRaw *>(m_mimeParts[n]);
   }
}

// ----------------------------------------------------------------------------
// UUDecodeFilter work function
// ----------------------------------------------------------------------------

// check if c is valid in uuencoded text
static inline IsUUValid(wxChar c)
{
   return c >= _T(' ') && c <= _T('`');
}

// "decode" a single character
#define DEC(c)    (((c) - ' ') & 077)

int UUdecodeLine(const wxChar* input, wxChar* output, const wxChar** endOfLine)
{
   // "input[0] - space" contains the count of characters in this line, check
   // that it is valid
   if ( input[0] <= _T(' ') || input[0] > _T('`') )
   {
      // invalid uu char
      return -1;
   }

   wxChar * cv_ptr = output;
   register const wxChar * p = input;

   const int cv_len = DEC(*p++);

   // Actually decode the uue data; ensure characters are in range.
   for (int i=0; i<cv_len; i+=3, p+=4)
   {
      if ( !IsUUValid(p[0]) ||
           !IsUUValid(p[1]) ||
           !IsUUValid(p[2]) ||
           !IsUUValid(p[3]) )
      {
         return -1;
      }

      *cv_ptr++ = DEC(p[0]) << 2 | DEC(p[1]) >> 4;
      *cv_ptr++ = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
      *cv_ptr++ = DEC(p[2]) << 6 | DEC(p[3]);
   }
   if (*p != _T('\r') && *p != _T('\n') && *p != _T('\0'))
      return -1;
   //else  *p=0;

   *endOfLine = p;
   return cv_len;
}

bool UUdecodeFile(const wxChar* input, String& output, const wxChar** endOfEncodedStream)
{
   static const int allocationSize = 10000;

   String decodedFile;
   size_t totalAllocatedBytes = 0;

   size_t totalDecodedBytes = 0;
   int decodedBytesInLine;
   const wxChar* startOfLine = input;
   const wxChar* endOfLine = 0; // init not needed
   wxChar* buffer = new wxChar[45];
   while ( (decodedBytesInLine =
               UUdecodeLine(startOfLine, buffer, &endOfLine)) > 0 )
   {
      // We've just decoded a line
      totalDecodedBytes += decodedBytesInLine;
      String decodedLine(buffer, decodedBytesInLine);
      if (decodedFile.length() + (size_t)decodedBytesInLine >= totalAllocatedBytes)
      {
         totalAllocatedBytes += allocationSize;
         decodedFile.Alloc(totalAllocatedBytes);
      }
      decodedFile += decodedLine;

      ASSERT_MSG( decodedFile.Length() == totalDecodedBytes,
            _T("Number of decoded bytes does not match!") );
      ASSERT_MSG( endOfLine[0] == _T('\r'), _T("Line has no '\\r' at the end!") );
      ASSERT_MSG( endOfLine[1] == _T('\n'), _T("Line has no '\\n' at the end!") );

      startOfLine = endOfLine + lenEOL;
   }

   delete[] buffer;
   if (decodedBytesInLine < 0)
   {
      wxLogWarning(_("Invalid character in uuencoded text."));
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
   static const size_t lenBegin = wxStrlen(UU_BEGIN_PREFIX);

   const wxChar *start = text.c_str();
   const wxChar *nextToOutput = start;
   while ( *start )
   {
      if ( wxStrncmp(start, UU_BEGIN_PREFIX, lenBegin) != 0 )
      {
         // try the next line (but only if not already at the end)
         start = wxStrchr(start, _T('\n'));
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

      const wxChar* startBeginLine = start;
      start += lenBegin;

      // Let's check that the next 4 chars after the 'begin ' are
      // 3 octal digits and space
      //
      // NB: 4 digit mode masks are in theory possible but shouldn't be used
      //     with uuencoded files
      if ( !(    start[0] >= _T('0') && start[0] <= _T('7')
              && start[1] >= _T('0') && start[1] <= _T('7')
              && start[2] >= _T('0') && start[2] <= _T('7')
              && start[3] == _T(' ') ) )
      {
         wxLogWarning(_("The BEGIN line is not correctly formed."));
         continue;
      }

      const wxChar* startName = start+4;     // skip mode and space
      const wxChar* endName = startName;
      // Rest of the line is the name
      while (*endName != _T('\r'))
      {
         ++endName;
         ASSERT_MSG( endName < text.c_str()+text.length(),
                         _T("'begin' line does not end?") );
      }

      ASSERT_MSG( endName[1] == _T('\n'),
                     _T("'begin' line does not end with \"\\r\\n\"?") );

      String fileName = String(startName, endName);
      const wxChar* start_data = endName + lenEOL;

      String decodedFile;
      const wxChar* endOfEncodedStream = 0;
      bool ok = UUdecodeFile(start_data, decodedFile, &endOfEncodedStream);
      if ( ok )
      {
         static const size_t lenEnd = wxStrlen(UU_END_PREFIX);
         if ( wxStrncmp(endOfEncodedStream, UU_END_PREFIX, lenEnd) == 0 )
         {
            endOfEncodedStream += lenEnd;
         }
         else
         {
            wxLogError(_("No \"end\" line at the end of uuencoded text."));
            ok = false;
         }
      }

      if ( ok )
      {
         // output the part before the BEGIN line, if any
         String prolog(nextToOutput, startBeginLine);
         if ( !prolog.empty() )
         {
            m_next->Process(prolog, viewer, style);
         }

         // Let's get a mimeType from the extention
         wxFileType *fileType = mApplication->GetMimeManager().GetFileTypeFromExtension(fileName.AfterLast('.'));
         String mimeType;
         if ( !fileType || !fileType->GetMimeType(&mimeType) )
         {
            mimeType = _T("APPLICATION/OCTET-STREAM");
         }
         delete fileType;

         MimePartRaw *mimepart =
            new MimePartRaw(fileName, mimeType, decodedFile, _T("uuencoded"));
         m_mimeParts.Add(mimepart);
         m_msgView->ShowPart(mimepart);

         nextToOutput =
         start = endOfEncodedStream;
      }
      else
      {
         wxLogWarning(_("Corrupted message with uuencoded attachment."));
      }
   }

   String prolog(nextToOutput);
   if ( !prolog.empty() )
   {
      m_next->Process(prolog, viewer, style);
   }
}
