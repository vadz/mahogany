///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   MessageTemplate.cpp - working with message templates
// Purpose:     classes and functions to parse the message templates (see
//              MessageTemplate.h for more details)
// Author:      Vadim Zeitlin
// Modified by:
// Created:     11.07.99
// CVS-ID:      $Id$
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
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
#   include "Profile.h"

//#   include <wx/string.h>
#   include <wx/intl.h>
#   include <wx/log.h>
#endif // USE_PCH

#include <wx/textfile.h>

#include "Mdefaults.h"

#include "MessageTemplate.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MessageTemplateSink
// ----------------------------------------------------------------------------

MessageTemplateSink::~MessageTemplateSink()
{
}

// ----------------------------------------------------------------------------
// MessageTemplateVarExpander
// ----------------------------------------------------------------------------

MessageTemplateVarExpander::~MessageTemplateVarExpander()
{
}

// ----------------------------------------------------------------------------
// MessageTemplateParser
// ----------------------------------------------------------------------------

bool MessageTemplateParser::Parse(MessageTemplateSink& sink) const
{
   // as this is used only for diagnostic messages, start counting from 1 - as
   // people like it (unlike the programmers)
   size_t nLine = 1;

   // only generate something if we can
   bool doOutput = m_expander != NULL;

   // the template text may be coming from various sources, so make sure that
   // it doesn't have some weird newline convention
   wxString templateText = wxTextFile::Translate(m_templateText,
                                                 wxTextFileType_Unix);
   const char *pc = templateText.c_str();
   const char *pStartOfLine = pc;
   while ( *pc )
   {
      // find next '$'
      while ( *pc && *pc != '$' )
      {
         if ( doOutput )
         {
            // normal text goes to output
            sink.Output(*pc);
         }

         if ( *pc == '\n' )
         {
            nLine++;
            pStartOfLine = pc;
         }

         pc++;
      }

      if ( !*pc )
         break;

      // what kind of brackets do we have? some of them imply the category
      // (like $`...` is the same as $(cmd: ...))
      String category;
      char bracketClose, bracketOpen = *++pc;
      switch ( bracketOpen )
      {
         case '\'':
            bracketClose = '\'';
            category = "cmd";
            break;

         case '<':
            bracketClose = '<';
            category = "file";
            break;

         case '(':
            bracketClose = ')';
            break;

         case '{':
            bracketClose = '}';
            break;

         case '$':
            // it's just escaped '$' and not start of the expansion at all
            if ( doOutput )
               sink.Output('$');
            pc++;
            continue;

         case '\0':
            wxLogWarning(_("Unexpected end of file '%s'."), m_filename.c_str());
            return FALSE;

         default:
            if ( isalpha(bracketOpen) )
            {
               // it wasn't a bracket at all, give back one char
               pc--;
               bracketClose = '\0';
            }
            else
            {
               wxLogWarning(_("Unexpected character at position %d in line "
                              "%d in the file '%s'."),
                            pc - pStartOfLine + 1,
                            nLine,
                            m_filename.c_str());

               return FALSE;
            }
      }

      pc++;

      // extract the next word
      bool quoted = *pc == '"';
      if ( quoted )
         pc++;

      String word;
      while ( *pc && (*pc != '\n') &&
              ((quoted && *pc && *pc != '"') ||
              (isalnum(*pc) && *pc != bracketClose && *pc)) )
      {
         word += *pc++;
      }

      if ( quoted && *pc == '"' )
      {
         // skip the closing quote
         pc++;
      }

      // decide what we've got
      enum
      {
         None,
         Left,
         Right,
         Center
      } alignment = None;
      unsigned int alignWidth = 0;
      bool truncate = FALSE;
      String name;

      if ( !bracketClose )
      {
         // there was no opening bracket, so it must be just name - whatever it
         // may be followed by
         name = word;
      }
      else
      {
         bool stop = FALSE;
         while ( !stop )
         {
            switch ( *pc )
            {
               case ':':
                  // ':' separates name from category
                  if ( !category )
                  {
                     category = word;

                     pc++; // skip ':'
                     while ( isalpha(*pc) )
                     {
                        name += *pc++;
                     }
                  }
                  else
                  {
                     // the message is the same as below, that's why we pass
                     // ':' as an argument instead of embedding it into the
                     // message directly
                     wxLogWarning(_("Unexpected character '%c' at line "
                                    "%d, position %d in the file '%s'."),
                                  ':',
                                  nLine,
                                  pc - pStartOfLine,
                                  m_filename.c_str());

                     return FALSE;
                  }
                  break;

               case '+':
                  alignment = Right;
                  // fall through

               case '=':
                  if ( alignment == None )
                     alignment = Center;

               case '1':
               case '2':
               case '3':
               case '4':
               case '5':
               case '6':
               case '7':
               case '8':
               case '9':
                  // fall through

               case '-':
                  if ( alignment == None )
                     alignment = Left;
                  // fall through

                  // alignment tail - so the preceding word was the name
                  name = word;
                  if ( !isdigit(*pc) )
                     pc++;

                  // extract the number (should be non zero)
                  if ( (sscanf(pc, "%u", &alignWidth) != 1) || !alignWidth )
                  {
                     wxLogWarning(_("Incorrect alignment width value at line "
                                    "%d, position %d in the file '%s'."),
                                  nLine,
                                  pc - pStartOfLine,
                                  m_filename.c_str());

                     return FALSE;
                  }

                  // skip until the end of number
                  while ( isdigit(*pc) )
                     pc++;

                  if ( *pc == '!' )
                  {
                     // truncate the field to fit in given width
                     truncate = TRUE;
                     pc++;
                  }
                  break;

               default:
                  if ( *pc == bracketClose )
                  {
                     // end of expression reached
                     stop = TRUE;

                     // the extracted word was the name
                     if ( !name )
                     {
                         name = word;
                     }

                     // skip the closing bracker
                     pc++;

                     break;
                  }
                  else
                  {
                     wxLogWarning(_("Unexpected character '%c' at line "
                                    "%d, position %d in the file '%s'."),
                                  *pc,
                                  nLine,
                                  pc - pStartOfLine,
                                  m_filename.c_str());

                     return FALSE;
                  }
            }
         }
      }

      // we have all the info, so we may ask the expander for the value - but
      // skip all the rest if we're not generating output
      if ( !doOutput )
         continue;

      String value;
      if ( !m_expander->Expand(category, name, &value) )
      {
         // don't log the message if the value is not empty - this means that
         // the variable *is* known, but that the expansion, for some reason,
         // failed.
         if ( !value )
         {
            wxLogWarning(_("Unknown variable '%s' at line %d, position %d "
                           "in the file '%s'."),
                         name.c_str(),
                         nLine,
                         pc - pStartOfLine - name.length(),
                         m_filename.c_str());
         }
         //else: message should have been already given

         return FALSE;
      }

      // align if necessary
      if ( alignment != None )
      {
         size_t len = value.length();
         switch ( alignment )
         {
            case Left:
               if ( alignWidth > len )
               {
                  // add some spaces
                  value += wxString(' ', alignWidth - len);
               }
               else if ( (len > alignWidth) && truncate )
               {
                  value.Truncate(alignWidth);
               }
               //else: value is already wide enough, but we don't truncate it
               break;

            case Right:
               if ( alignWidth > len )
               {
                  // prepend some spaces
                  value = wxString(' ', alignWidth - len) + value;
               }
               else if ( (len > alignWidth) && truncate )
               {
                  value = value.c_str() + len - alignWidth;
               }
               //else: value is already wide enough, but we don't truncate it
               break;

            case Center:
               if ( alignWidth > len )
               {
                  // prepend and append some spaces
                  size_t n1 = (alignWidth - len) / 2,
                         n2 = alignWidth - n1;
                  value = wxString(' ', n1) + value + wxString(' ', n2);
               }
               else if ( (len > alignWidth) && truncate )
               {
                  // truncate a bit at right and a bit at left side
                  value = value.c_str() + (len - alignWidth) / 2;
                  value.Truncate(alignWidth);
               }
               //else: value is already wide enough, but we don't truncate it
               break;

            default:
               FAIL_MSG("unknown alignment value");
         }
      }

      sink.Output(value);
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

// the helper for Get/SetMessageTemplate
static String GetMessageTemplateProfilePath(MessageTemplateKind kind)
{
   String templateKey = M_TEMPLATE_SECTION;

   switch ( kind )
   {
      case MessageTemplate_NewMessage:
         templateKey += MP_TEMPLATE_NEWMESSAGE;
         break;

      case MessageTemplate_NewArticle:
         templateKey += MP_TEMPLATE_NEWARTICLE;
         break;

      case MessageTemplate_Reply:
         templateKey += MP_TEMPLATE_REPLY;
         break;

      case MessageTemplate_Followup:
         templateKey += MP_TEMPLATE_FOLLOWUP;
         break;

      case MessageTemplate_Forward:
         templateKey += MP_TEMPLATE_FORWARD;
         break;

      default:
         FAIL_MSG("unknown template kind");
   }

   return templateKey;
}

// get the value of the message template for the given profile (inherits value
// from the parent profile)
extern String
GetMessageTemplate(MessageTemplateKind kind, ProfileBase *profile)
{
   // the templates contain '$'s so disable variable expansion for now
   ProfileEnvVarSave noEnvVarExpansion(profile);

   String value = profile->readEntry(GetMessageTemplateProfilePath(kind), "");
   if ( !value )
   {
      // we have the default templates for reply, follow-up and forward
      switch ( kind )
      {
         case MessageTemplate_Reply:
            value = _("On $(ORIGINAL:DATE) you wrote:\n\n$QUOTE\n$CURSOR");
            break;

         case MessageTemplate_Forward:
            value = "$CURSOR\n$QUOTE822";
            break;

         case MessageTemplate_Followup:
            value = _("On $(ORIGINAL:DATE) $SENDER wrote in "
                      "$(ORIGINAL:NEWSGROUPS)\n\n$QUOTE\n$CURSOR");
            break;

         default:
            // nothing to do, but put it here to slience gcc warnings
            ;
      }
   }

   return value;
}

extern void
SetMessageTemplate(const String& value,
                   MessageTemplateKind kind,
                   ProfileBase *profile)
{
   (void)profile->writeEntry(GetMessageTemplateProfilePath(kind), value);
}

