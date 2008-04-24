///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MimePartCCBase.h
// Purpose:     MimePart implementation using c-client data structures
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-07-13
// CVS-ID:      $Id$
// Copyright:   (c) 2001-2004 Vadim Zeitlin <vadim@wxwindows.org>
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
   #include "Mcommon.h"

   #include "Mcclient.h"
#endif

#include <wx/fontmap.h>

#include "mail/MimeDecode.h"
#include "MimePartCCBase.h"
#include "MailFolder.h"         // for DecodeHeader()

// ============================================================================
// MimePartCCBase implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ctor/dtor
// ----------------------------------------------------------------------------

void MimePartCCBase::Init()
{
   m_parent =
   m_nested =
   m_next = NULL;

   m_body = NULL;

   m_parameterList =
   m_dispositionParameterList = NULL;

   m_content = NULL;
   m_lenContent = 0;
   m_ownsContent = false;

   m_encoding = wxFONTENCODING_SYSTEM;
}

void
MimePartCCBase::Create(BODY *body, MimePartCCBase *parent, size_t nPart)
{
   m_parent = parent;
   m_body = body;

   /*
      Nasty hack: c-client (and so probably IMAP as well) doesn't seem to
      number the multipart parts whose parent part is the message, so when
      assigning the part spec to their children we should skip the parent and
      use the grandparent spec as the base.

      I'd like to really understand the rule to be used here one of these
      days...
    */
   if ( m_parent && m_parent->GetParent() )
   {
      String specParent;

      MimeType::Primary mt = m_parent->GetType().GetPrimary();
      if ( mt == MimeType::MULTIPART )
      {
         MimePartCCBase *grandparent = m_parent->m_parent;

         // it shouldn't be the top level part (note that it is non NULL
         // because of the test in the enclosing if)
         if ( grandparent->m_parent )
         {
            mt = grandparent->GetType().GetPrimary();

            if ( mt == MimeType::MESSAGE )
            {
               // our parent part doesn't have its own part number, use the
               // grand parent spec as the base
               specParent = grandparent->m_spec;
            }
         }
      }

      if ( specParent.empty() )
      {
         specParent = m_parent->m_spec;
      }

      m_spec << specParent << '.';
   }

   m_spec << wxString::Format(_T("%lu"), (unsigned long)nPart);
}

MimePartCCBase::~MimePartCCBase()
{
   delete m_next;
   delete m_nested;

   delete m_parameterList;
   delete m_dispositionParameterList;

   if ( m_ownsContent )
      fs_give(&m_content);
}

size_t MimePartCCBase::GetPartsCount() const
{
   // multipart parts don't count here, we're only interested in parts
   // containign something
   size_t count = m_body->type == TYPEMULTIPART ? 0 : 1;
   if ( m_next )
      count += m_next->GetPartsCount();
   if ( m_nested )
      count += m_nested->GetPartsCount();

   return count;
}

// ----------------------------------------------------------------------------
// body attributes access
// ----------------------------------------------------------------------------

MimeType MimePartCCBase::GetType() const
{
   // cast is ok as we use the same values in MimeType as c-client
   return MimeType(static_cast<MimeType::Primary>(m_body->type),
                   wxString::FromAscii(m_body->subtype));
}

String MimePartCCBase::GetDescription() const
{
   // FIXME: we lose the encoding info here - but we don't have any way to
   //        return it from here currently
   return MIME::DecodeHeader(wxString::From8BitData(m_body->description));
}

MimeXferEncoding MimePartCCBase::GetTransferEncoding() const
{
   // cast is ok as we use the same values for MimeXferEncoding as c-client
   return (MimeXferEncoding)m_body->encoding;
}

size_t MimePartCCBase::GetSize() const
{
   return m_body->size.bytes;
}

String MimePartCCBase::GetPartSpec() const
{
   return m_spec;
}

// ----------------------------------------------------------------------------
// parameters access
// ----------------------------------------------------------------------------

String MimePartCCBase::GetFilename() const
{
   // try hard to find an acceptable name for this part
   String filename = GetDispositionParam(_T("filename"));

   if ( filename.empty() )
      filename = GetParam(_T("filename"));

   if ( filename.empty() )
      filename = GetParam(_T("name"));

   return filename;
}

String MimePartCCBase::GetDisposition() const
{
   return wxString::From8BitData(m_body->disposition.type);
}

/* static */
String
MimePartCCBase::FindParam(const MimeParameterList& list, const String& name)
{
   String value;

   MimeParameterList::iterator i;
   for ( i = list.begin(); i != list.end(); i++ )
   {
      // parameter names are not case-sensitive, i.e. "charset" == "CHARSET"
      if ( name.CmpNoCase(i->name) == 0 )
      {
         // found
         value = i->value;
         break;
      }
   }

   return MIME::DecodeHeader(value);
}

String MimePartCCBase::GetParam(const String& name) const
{
   return FindParam(GetParameters(), name);
}

String MimePartCCBase::GetDispositionParam(const String& name) const
{
   return FindParam(GetDispositionParameters(), name);
}

// ----------------------------------------------------------------------------
// parameter lists handling
// ----------------------------------------------------------------------------

/* static */
void MimePartCCBase::InitParamList(MimeParameterList *list, PARAMETER *par)
{
   while ( par )
   {
      list->push_back(new MimeParameter(par->attribute, par->value));

      par = par->next;
   }
}

const MimeParameterList& MimePartCCBase::GetParameters() const
{
   if ( !m_parameterList )
   {
      ((MimePartCCBase *)this)->m_parameterList = new MimeParameterList;
      InitParamList(m_parameterList, m_body->parameter);
   }

   return *m_parameterList;
}

const MimeParameterList& MimePartCCBase::GetDispositionParameters() const
{
   if ( !m_dispositionParameterList )
   {
      ((MimePartCCBase *)this)->m_dispositionParameterList = new MimeParameterList;
      InitParamList(m_dispositionParameterList, m_body->disposition.parameter);
   }

   return *m_dispositionParameterList;
}

// ----------------------------------------------------------------------------
// text part additional info
// ----------------------------------------------------------------------------

size_t MimePartCCBase::GetNumberOfLines() const
{
   return m_body->size.lines;
}

wxFontEncoding MimePartCCBase::GetTextEncoding() const
{
   if ( m_encoding == wxFONTENCODING_SYSTEM )
   {
      String charset = GetParam(_T("charset"));

      if ( !charset.empty() )
         m_encoding = wxFontMapper::Get()->CharsetToEncoding(charset);

      // CharsetToEncoding() returns this for US-ASCII, treat it as Latin1 here
      // as it's a strict subset; and if we don't have any charset information
      // it's still the right choice
      if ( m_encoding == wxFONTENCODING_SYSTEM ||
            m_encoding == wxFONTENCODING_DEFAULT )
      {
         m_encoding = wxFONTENCODING_ISO8859_1;
      }

      // special case: many broken programs generate messages with UTF-8
      // charset but without properly encoding the contents in UTF-8 which
      // results in "s" being empty and not showing the text parts at all
      //
      // so if the conversion failed, try to show the text at least somehow
      // using latin1 (and the user will be able to change the encoding
      // manually from the menu later which would be impossible if we returned
      // an empty string from GetTextContent())
      if ( m_encoding == wxFONTENCODING_UTF8 ||
            m_encoding == wxFONTENCODING_UTF7 )
      {
         // check if we really have valid UTF-x
         unsigned long len;
         const char *p = reinterpret_cast<const char *>(GetContent(&len));

         if ( p &&
               wxCSConv(m_encoding).ToWChar(NULL, 0, p, len) == wxCONV_FAILED )
         {
            m_encoding = wxFONTENCODING_ISO8859_1;
         }
      }
   }

   return m_encoding;
}

// ----------------------------------------------------------------------------
// data access
// ----------------------------------------------------------------------------

const void *MimePartCCBase::GetContent(unsigned long *lenptr) const
{
   MimePartCCBase * const self = const_cast<MimePartCCBase *>(this);

   if ( m_ownsContent )
   {
      ASSERT_MSG( m_content, "must have some content to own" );

      // if we have the old contents pointer which we own, we can be simply
      // reuse it
      *lenptr = m_lenContent;

      return m_content;
   }
   //else: if we don't own the content, we can't suppose that it's valid even
   //      if it's non-NULL because it's a pointer into a per-stream (in
   //      c-client sense) buffer and so could be overwritten by an operation
   //      on another part

   const void *cptr = GetRawContent(lenptr);
   if ( !cptr || !*lenptr )
      return NULL;

   return self->DecodeRawContent(cptr, lenptr);
}

const void *
MimePartCCBase::DecodeRawContent(const void *cptr, unsigned long *lenptr)
{
   // total size of this message part
   unsigned long size = GetSize();

   // just for convenience...
   unsigned char *text = (unsigned char *)cptr;

   switch ( GetTransferEncoding() )
   {
      case ENCQUOTEDPRINTABLE:   // human-readable 8-as-7 bit data
         m_content = rfc822_qprint(text, size, lenptr);

         // some broken mailers sent messages with QP specified as the content
         // transfer encoding in the headers but don't encode the message
         // properly - in this case show it as plain text which is better than
         // not showing it at all
         if ( m_content )
         {
            m_ownsContent = true;

            break;
         }
         //else: treat it as plain text

         // it was overwritten by rfc822_qprint() above
         *lenptr = size;

         // fall through

      case ENC7BIT:        // 7 bit SMTP semantic data
      case ENC8BIT:        // 8 bit SMTP semantic data
      case ENCBINARY:      // 8 bit binary data
      case ENCOTHER:       // unknown
      default:
         // nothing to do
         m_content = text;
         break;


      case ENCBASE64:      // base-64 encoded data
         // the size of possible extra non Base64 encoded text following a
         // Base64 encoded part
         const unsigned char *startSlack = NULL;
         size_t sizeSlack = 0;

         // there is a frequent problem with mail list software appending the
         // mailing list footer (i.e. a standard signature containing the
         // instructions about how to [un]subscribe) to the top level part of a
         // Base64-encoded message thus making it invalid - worse c-client code
         // doesn't complain about it but simply returns some garbage in this
         // case
         //
         // we try to detect this case and correct for it: note that neither
         // '-' nor '_' which typically start the signature are valid
         // characters in base64 so the logic below should work for all common
         // cases

         // only check the top level part
         if ( !GetParent() )
         {
            const unsigned char *p;
            for ( p = text; *p; p++ )
            {
               // we do *not* want to use the locale-specific settings here,
               // hence don't use isalpha()
               const unsigned char ch = *p;
               if ( (ch >= 'A' && ch <= 'Z') ||
                     (ch >= 'a' && ch <= 'z') ||
                      (ch >= '0' && ch <= '9') ||
                       (ch == '+' || ch == '/' || ch == '\r' || ch == '\n') )
               {
                  // valid Base64 char
                  continue;
               }

               if ( ch == '=' )
               {
                  p++;

                  // valid, but can only occur at the end of data as padding,
                  // so still break below -- but not before:

                  // a) skipping a possible second '=' (can't be more than 2 of
                  // them)
                  if ( *p == '=' )
                     p++;

                  // b) skipping the terminating "\r\n"
                  if ( p[0] == '\r' && p[1] == '\n' )
                     p += 2;
               }

               // what (if anything) follows can't appear in a valid Base64
               // message
               break;
            }

            size_t sizeValid = p - text;
            if ( sizeValid != size )
            {
               ASSERT_MSG( sizeValid < size,
                           _T("logic error in base64 validity check") );

               // take all the rest verbatim below
               startSlack = p;
               sizeSlack = size - sizeValid;

               // and decode just the (at least potentially) valid part
               size = sizeValid;
            }
         }

         m_content = rfc822_base64(text, size, lenptr);
         if ( !m_content )
         {
            wxLogWarning(_("Failed to decode binary message part, "
                           "message could be corrupted."));

            // use original text: this is better than nothing and can be
            // exactly what we need in case of messages generated with base64
            // encoding in the headers but then sent as 8 it binary (old (circa
            // 2003) versions of Thunderbird did this!)
            m_content = text;
            *lenptr = size;
         }
         else // ok
         {
            // append the non Base64-encoded chunk, if any, to the end of decoded
            // data
            if ( sizeSlack )
            {
               fs_resize(&m_content, *lenptr + sizeSlack);
               memcpy((char *)m_content + *lenptr, startSlack, sizeSlack);

               *lenptr += sizeSlack;
            }

            m_ownsContent = true;
         }
   }

   m_lenContent = *lenptr;

   return m_content;
}

String MimePartCCBase::GetTextContent() const
{
   unsigned long len;
   const char *p = reinterpret_cast<const char *>(GetContent(&len));
   wxString s;
   if ( p )
   {
      s = wxString(p, wxCSConv(GetTextEncoding()), len);
   }

   return s;
}

