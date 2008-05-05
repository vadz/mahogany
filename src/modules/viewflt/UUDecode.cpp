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
#   include <wx/filefn.h>      // for wxSplitPath
#endif //USE_PCH

#include <wx/bitmap.h>
#include <wx/mimetype.h>

#include "MessageViewer.h"
#include "ViewFilter.h"
#include "ClickAtt.h"

#include "MimePartVirtual.h"

// strlen("\r\n")
static const size_t lenEOL = 2;

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

// all UUencoded data start with a line whose prefix is "begin" and preceded by
// a blank line
#define UU_BEGIN_PREFIX "begin "

// UUencoded data ends at a line consisting solely of "end"
#define UU_END_PREFIX "\r\nend\r\n"

// ============================================================================
// UUDecodeFilter implementation
// ============================================================================

IMPLEMENT_VIEWER_FILTER(UUDecodeFilter,
                        ViewFilter::Priority_High - 5,
                        true,      // initially enabled
                        gettext_noop("UUdecode"),
                        "(c) 2004 Xavier Nodet <xavier.nodet@free.fr>");

// ----------------------------------------------------------------------------
// UUDecodeFilter ctor
// ----------------------------------------------------------------------------

UUDecodeFilter::UUDecodeFilter(MessageView *msgView,
                               ViewFilter *next,
                               bool enable)
              : ViewFilter(msgView, next, enable)
{
}

// ----------------------------------------------------------------------------
// UUDecodeFilter work function
// ----------------------------------------------------------------------------

namespace
{

// check if c is valid in uuencoded text
inline bool IsUUValid(char c)
{
   return c >= ' ' && c <= '`';
}

static const int MAX_UU_LINE_LEN = 45;

// "decode" a single character
#define UUdec(c)    (((c) - ' ') & 077)

int UUdecodeLine(const char *input, char *output, const char **endOfLine)
{
   if ( !IsUUValid(*input) )
      return -1;

   const int cv_len = UUdec(*input++);
   if ( cv_len > MAX_UU_LINE_LEN )
      return -1;

   // Actually decode the uue data; ensure characters are in range.
   for ( int i = 0; i < cv_len; i += 3, input += 4 )
   {
      if ( !IsUUValid(input[0]) || !IsUUValid(input[1]) ||
           !IsUUValid(input[2]) || !IsUUValid(input[3]) )
      {
         return -1;
      }

      *output++ = UUdec(input[0]) << 2 | UUdec(input[1]) >> 4;
      *output++ = UUdec(input[1]) << 4 | UUdec(input[2]) >> 2;
      *output++ = UUdec(input[2]) << 6 | UUdec(input[3]);
   }

   if ( *input != '\r' && *input != '\n' && *input != '\0' )
      return -1;

   *endOfLine = input;
   return cv_len;
}

bool
UUdecodeFile(const char *input,
             wxMemoryBuffer& output,
             const char **endOfEncodedStream)
{
   static const size_t allocationSize = 10000;

   size_t totalAllocatedBytes = 0;

   size_t totalDecodedBytes = 0;
   int decodedBytesInLine;
   const char *startOfLine = input;
   const char *endOfLine = 0; // init not needed
   char buffer[MAX_UU_LINE_LEN];
   while ( (decodedBytesInLine =
               UUdecodeLine(startOfLine, buffer, &endOfLine)) > 0 )
   {
      // We've just decoded a line
      totalDecodedBytes += decodedBytesInLine;
      if ( output.GetDataLen() + decodedBytesInLine >= totalAllocatedBytes )
      {
         totalAllocatedBytes += allocationSize;
         output.GetWriteBuf(totalAllocatedBytes);
      }

      output.AppendData(buffer, decodedBytesInLine);

      ASSERT_MSG( endOfLine[0] == '\r', "Line has no '\\r' at the end!" );
      ASSERT_MSG( endOfLine[1] == '\n', "Line has no '\\n' at the end!" );

      startOfLine = endOfLine + lenEOL;
   }

   if ( decodedBytesInLine < 0 )
   {
      wxLogWarning(_("Invalid character in uuencoded text."));
      return false;
   }

   *endOfEncodedStream = endOfLine;

   return true;
}

} // anonymous namespace


void
UUDecodeFilter::DoProcess(String& text,
                          MessageViewer *viewer,
                          MTextStyle& style)
{
   // do we have something looking like UUencoded data?
   static const size_t lenBegin = wxStrlen(UU_BEGIN_PREFIX);

   const char *start = text.c_str();
   const char *nextToOutput = start;
   while ( *start )
   {
      bool hasBegin = wxStrncmp(start, UU_BEGIN_PREFIX, lenBegin) == 0;
      if ( hasBegin )
      {
         // check that we're either at the start of the text or after a blank
         // line
         switch ( start - text.c_str() )
         {
            case 0:
               // leave hasBegin at true
               break;

            case 1:
            case 3:
               // some garbage before "begin": can't be one or 2 EOLs
               hasBegin = false;
               break;

               // 2 or 4 and more characters after the start
            default:
               // only blank line allowed before
               if ( start[-1] != '\n' || start[-2] != '\r' )
                  hasBegin = false;
               if ( start - text.c_str() > 2 &&
                        (start[-3] != '\n' || start[-4] != '\r') )
                  hasBegin = false;
         }
      }

      if ( !hasBegin )
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

      const char *startBeginLine = start;
      start += lenBegin;

      // Let's check that the next 4 chars after the 'begin ' are
      // 3 octal digits and space
      //
      // NB: 4 digit mode masks are in theory possible but shouldn't be used
      //     with uuencoded files
      if ( !(    start[0] >= '0' && start[0] <= '7'
              && start[1] >= '0' && start[1] <= '7'
              && start[2] >= '0' && start[2] <= '7'
              && start[3] == ' ' ) )
      {
         wxLogWarning(_("The BEGIN line is not correctly formed."));
         continue;
      }

      const char *startName = start+4;     // skip mode and space
      const char *endName = startName;
      // Rest of the line is the name
      while ( *endName != '\r' )
      {
         if ( !*endName++ )
         {
            wxLogWarning(_("The BEGIN line is unterminated."));
            String prolog(nextToOutput);
            if ( !prolog.empty() )
            {
               m_next->Process(prolog, viewer, style);
            }
            return;   // no more text
         }
      }

      ASSERT_MSG( endName[1] == '\n',
                     "'begin' line does not end with \"\\r\\n\"?" );

      const String fileName(startName, endName);
      const char *start_data = endName + lenEOL;


      // buffer with the entire virtual part contents
      wxMemoryBuffer virtData;

      // start with the header for the uudecoded virtual part
      String header("Mime-Version: 1.0\r\n"
                    //"Content-Transfer-Encoding: uuencode\r\n"
                    "Content-Disposition: attachment; filename=\"" +
                    fileName + "\"\r\n");

      // get a mimeType from the extention
      String mimeType;
      String ext;
      wxSplitPath(fileName, NULL, NULL, &ext);
      if ( !ext.empty() )
      {
         wxFileType *
            ft = mApplication->GetMimeManager().GetFileTypeFromExtension(ext);
         if ( ft )
            ft->GetMimeType(&mimeType);
         delete ft;
      }

      if ( mimeType.empty() )
         mimeType = "APPLICATION/OCTET-STREAM";
      header += "Content-Type: " + mimeType;
      header += "; name=\"" + fileName + "\"\r\n";
      header += "\r\n"; // blank line separating the header from body

      virtData.AppendData(header.ToAscii(), header.length());

      const char *endOfEncodedStream = 0;
      bool ok = UUdecodeFile(start_data, virtData, &endOfEncodedStream);
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

         // create and process the virtual part containing uudecoded contents
         MimePartVirtual * const mimepart = new MimePartVirtual(virtData);
         m_msgView->AddVirtualMimePart(mimepart);
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
