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

class MimePartUUencode : public MimePart
{
public:
  MimePartUUencode(const String& fileName, const String& mimeType, const String& content) 
    : m_fileName(fileName)
    , m_mimeType(mimeType)
    , m_content(content)
  {}
public:
   /// get the parent MIME part (NULL for the top level one)
  virtual MimePart *GetParent() const {return 0;}

   /// get the next MIME part (NULL if this is the last part of multipart)
   virtual MimePart *GetNext() const {return 0;}

   /// get the first child part (NULL if not multipart or message)
   virtual MimePart *GetNested() const {return 0;}

   //@}

   /** @name Headers access

       Get the information from "Content-Type" and "Content-Disposition"
       headers.
    */
   //@{

   /// get the MIME type of the part
   virtual MimeType GetType() const {
      return MimeType(m_mimeType);
   }

   /// get the description of the part, if any
   virtual String GetDescription() const {return String();}

   /// get the filename if the part is an attachment
   virtual String GetFilename() const {return m_fileName;}

   /// get the disposition specified in Content-Disposition
   virtual String GetDisposition() const {return _T("uuencoded");}

   /// get the IMAP part spec (of the #.#.#.# form)
   virtual String GetPartSpec() const {return String();}

   /// get the value of the specified Content-Type parameter
   virtual String GetParam(const String& name) const {return String();}

   /// get the value of the specified Content-Disposition parameter
   virtual String GetDispositionParam(const String& name) const {return String();}

   /// get the list of all parameters (from Content-Type)
   virtual const MimeParameterList& GetParameters() const {return m_parameters;}

   /// get the list of all disposition parameters (from Content-Disposition)
   virtual const MimeParameterList& GetDispositionParameters() const {return m_dispositionParameters;}

   //@}

   /** @name Data access

       Allows to get the part data. Note that it is always returned in the
       decoded form (why would we need it otherwise?) so its size in general
       won't be the same as the number returned by GetSize()
    */
   //@{

   /// get the raw (un-decoded) contents of this part
   virtual const void *GetRawContent(unsigned long *len = NULL) const {
      return GetContent(len);
   }

   /**
       get the decoded contents of this part

       WARNING: the pointer is not NULL terminated, len must be used!
    */
   virtual const void *GetContent(unsigned long *len) const {
     if (len) *len = m_content.Length();
     return m_content.c_str();
   }

   /**
       get contents of this part as text.

    */
   virtual String GetTextContent() const {
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


   /// get all headers as one string
   virtual String GetHeaders() const {return String();}

   /// get the encoding of the part
   virtual MimeXferEncoding GetTransferEncoding() const {return MIME_ENC_BINARY;}

   /// get the part size in bytes
   virtual size_t GetSize() const {return m_content.length();}

   //@}

   /** @name Text part methods

       These methods can only be called for the part of type MimeType::TEXT or
       MESSAGE (if the message has a single text part)
    */
   //@{

   /// get the encoding of the text (only if GetType() == TEXT!)
   virtual wxFontEncoding GetTextEncoding() const {return wxFONTENCODING_DEFAULT;}

   /// get the size in lines
   virtual size_t GetNumberOfLines() const {return 0;}

   //@}

private:
  MimeParameterList m_parameters;
  MimeParameterList m_dispositionParameters;

  String m_fileName;
  String m_mimeType;
  String m_content;
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

bool UUdecodeFile(const wxChar* input, String& output) {

  static const int allocationSize = 10000;

  String decodedFile;
  size_t totalAllocatedBytes = 0;

  size_t totalDecodedBytes = 0;
  int decodedBytesInLine;
  const wxChar* startOfLine = input;
  const wxChar* endOfLine = 0; // init not needed
  wxChar* buffer = new wxChar[45];
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
  return true;
}



void
UUDecodeFilter::DoProcess(String& text,
                     MessageViewer *viewer,
                     MTextStyle& style)
{
   // do we have something looking like UUencoded data?
   //
   // there should be a BEGIN line near the start of the message
   bool foundBegin = false;
   const wxChar *start = text.c_str();
   const wxChar* start_data = 0;
   for ( size_t numLines = 0; numLines < 100; numLines++ )
   {
      if ( wxStrncmp(start, UU_BEGIN_PREFIX, wxStrlen(UU_BEGIN_PREFIX)) == 0 )
      {
         foundBegin = true;
         break;
      }

      // try the next line (but only if not already at the end)
      if ( *start )
         start = wxStrchr(start, '\n');
      else
         break;
      if ( start )
         start++; // skip '\n' itself
      else
         break; // no more text
   }

   if ( foundBegin )
   {
      const wxChar *tail = start + wxStrlen(UU_BEGIN_PREFIX);
      // this flag tells us if everything is ok so far -- as soon as it becomes
      // false, we skip all subsequent steps
      bool ok = true;

      // Let's check that the next 4 chars after the 'begin ' are
      // digit, digit, digit, space
      if ( ok && !(    tail[0] >= '0' && tail[0] <= '9'
                    && tail[1] >= '0' && tail[1] <= '9'
                    && tail[2] >= '0' && tail[2] <= '9'
                    && tail[3] == ' ' ) )
      {
         wxLogWarning(_("The BEGIN line is not correctly formed."));

         ok = false;
      }

      String fileName;
      if ( ok )
      {
         const wxChar* startName = tail+4;
         tail = startName;
         // Rest of the line is the name
         while (*tail != '\r')
         {
           ++tail;
           ASSERT_MSG( tail < text.c_str()+text.Length(), _T("'begin' line does not end?") );
         }
         ASSERT_MSG( tail[1] == '\n', _T("'begin' line does not end with \"\\r\\n\"?") );
         fileName = String(startName, tail);
         start_data = tail + 2;
      }

      // end of the UU part
      const wxChar *end = NULL; // unneeded but suppresses the compiler warning
      if ( ok ) // ok, it starts with something valid
      {
         // now locate the end line
         const wxChar *pc = text.c_str() + text.Length() - 1;

         bool foundEnd = false;
         for ( ;; )
         {
            // optimistically suppose that this line will be the END one
            end = pc + 2;

            // find the beginning of this line
            while ( *pc != '\n' && pc >= start )
            {
               pc--;
            }

            // we took one extra char
            pc++;

            if ( wxStrncmp(pc, UU_END_PREFIX, wxStrlen(UU_END_PREFIX)) == 0 )
            {
               tail = pc + wxStrlen(UU_END_PREFIX);

               foundEnd = true;
               break;
            }

            // undo the above
            pc--;

            if ( pc < start )
            {
               // we exhausted the message without finding the END line, leave
               // foundEnd at false and exit the loop
               break;
            }

            pc--;
            ASSERT_MSG( *pc == '\r', _T("line doesn't end in\"\\r\\n\"?") );
         }

         if ( !foundEnd )
         {
            wxLogWarning(_("END line not found."));

            ok = false;
         }
      }

      // if everything was ok so far, continue with decoding/verifying
      if ( ok )
      {
         // output the part before the BEGIN line, if any
         String prolog(text.c_str(), start);
         if ( !prolog.empty() )
         {
            m_next->Process(prolog, viewer, style);
         }

         String decodedFile;
         bool ok = UUdecodeFile(start_data, decodedFile);

         if ( ok )
         {
#if 1
            // Let's get a fileType
            wxFileType *fileType = mApplication->GetMimeManager().GetFileTypeFromExtension(fileName.AfterLast('.'));
            String mimeType;
            if (!fileType->GetMimeType(&mimeType)) {
              mimeType = _T("APPLICATION/OCTET-STREAM");
            }
            delete fileType;

            MimePartUUencode* decodedMime = new MimePartUUencode(fileName, mimeType, decodedFile);

            m_msgView->ShowPart(decodedMime);
#else
            viewer->InsertText(_T("UUencoded file named \""), style);
            viewer->InsertText(fileName, style);
            viewer->InsertText(_T("\"\n"), style);
#endif
            // output the part after the END line, if any
            String epilog(end);
            if ( !epilog.empty() )
            {
                m_next->Process(epilog, viewer, style);
            }
         }
      }

      if ( ok )
      {
         // skip the normal display below
         return;
      }

      wxLogWarning(_("Part of this message seems to be UU encoded "
                    "but in fact is not."));
   }

   m_next->Process(text, viewer, style);
}






































#if 0
/*
* Might it be data ? This _should_be_ the most common line type so
* we need to make it run as fast as we can. 
*
* First try a fast UUdecode. This only allows perfectly correct data
* from a modern uuencoder.
*/

if  (buf[0] > ' ' && buf[0] <= '`') {
  int  i, ok = 1;
  char * cv_ptr = cv_buf;
  register unsigned char * p=buf;

  cv_len = DEC(*p++);

  /* Actually decode the uue data; ensure characters are in range. */
  if (ok) for (i=0; i<cv_len; i+=3, p+=4) {
    if ( (p[0]<=' ' || p[0]>'`') ||
      (p[1]<=' ' || p[1]>'`') ||
      (p[2]<=' ' || p[2]>'`') ||
      (p[3]<=' ' || p[3]>'`') ) {
        ok = 0;
        break;
      }
      *cv_ptr++ = DEC(*p) << 2 | DEC(p[1]) >> 4;
      *cv_ptr++ = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
      *cv_ptr++ = DEC(p[2]) << 6 | DEC(p[3]);
  }
  if (*p != '\r' && *p != '\n' && *p != '\0')
    ok=0;
  else  *p=0;

  if (ok) return cv_len ? LINE_UUDATA : LINE_EOD;
}




         /* 
         * This decodes any line as a data line (like an ancient decoder would)
         * it may be required for broken uuencoders.
         *
         * If the line is the wrong length it will still complain about corruption
         * but unlike the normal decoder it will still classify the line as data.
         *
         * Unlike the original BSD decoder if the line is too short it assumes
         * that trailing spaces have been clipped.
         *
         */
         int decode_uue_line(char * buf, int data_type)
         {
           int len,i;
           register unsigned char * p;

           /* Remove EOL marker - any style. */
           for(len=strlen(buf); len>0 && (buf[len-1]=='\r' || buf[len-1]=='\n') ;) 
             buf[--len] = 0;

           if (data_type == LINE_DATA || data_type == LINE_BEGIN) {
             char * cv_ptr = cv_buf;
             int expected;

             cv_len = buf[0] ? DEC(buf[0]) :0;

             /* Check for stripped trailing spaces. */
             expected = (((cv_len + 2) / 3) << 2) + 1;

             /* Warn about corruption */
             if (len != expected) corruption = 1;

             /* Add any stripped spaces */
             for (; len < expected; ) buf[len++] = ' '; buf[len] = 0;

             for (i=0,p=buf+1; i<cv_len; i+=3, p+=4) {
               *cv_ptr++ = DEC(*p) << 2 | DEC(p[1]) >> 4;
               *cv_ptr++ = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
               *cv_ptr++ = DEC(p[2]) << 6 | DEC(p[3]);
             }
             return cv_len?LINE_DATA:LINE_EOD;
           }
           if (strncmp(buf, "begin ", 6) == 0) return LINE_BEGIN;
           if (strcmp(buf, "end") == 0) return LINE_END;

           return LINE_JUNK;
         }






         /* A more normal uudecoder, it's stricter than most but allows some
         * varients. */
         if  (len<89 && buf[0] >= ' ' && buf[0] <= '`') {
           int  expected, ok = 1;
           char * cv_ptr = cv_buf;
           register unsigned char * p=buf+1;

           cv_len = DEC(buf[0]);

           /* The expected length may be either enough for all the byte triples
           * or just enough characters for all the bits.
           */
           expected = (((cv_len + 2) / 3) << 2) + 1;
           if (ok && len != expected) {

             /* If the line is one byte too long try for a checksum */
             if (len == expected + 1 || len == (cv_len*8 + 5)/6 + 2) {
               int csum = 0;
               for(i=1; i<len-1; i++) csum += DEC(buf[i]);
               if ((csum & 077) != DEC(buf[len-1])) ok = 0;
               else len--;
             }
             /* Is the line the precise length needed for the bits ? */
             else if (len != (cv_len*8 + 5)/6 + 1)
               ok = 0;

             if (ok) {
               /* Pad to where we originally expected the data. */
               strcpy(lbuf, buf); p = lbuf + 1;
               llen = len;
               while (llen < expected) lbuf[llen++] = '`'; lbuf[llen] = 0;
             }
           }

           /* Actually decode the uue data; ensure characters are in range. */
           if (ok) for (i=1; i<expected; i+=4, p+=4) {
             if ( (p[0]<' ' || p[0]>'`') ||
               (p[1]<' ' || p[1]>'`') ||
               (p[2]<' ' || p[2]>'`') ||
               (p[3]<' ' || p[3]>'`') ) {
                 ok = 0;
                 break;
               }
               *cv_ptr++ = DEC(*p) << 2 | DEC(p[1]) >> 4;
               *cv_ptr++ = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
               *cv_ptr++ = DEC(p[2]) << 6 | DEC(p[3]);
           }
           if (ok) return cv_len ? LINE_UUDATA : LINE_EOD;
         }

#endif