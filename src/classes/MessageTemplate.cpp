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

#   include <wx/string.h>       // for wxString
#   include <wx/log.h>          // for wxLogWarning
#   include <wx/wxchar.h>       // for wxPrintf/Scanf
#endif // USE_PCH

#include <wx/textfile.h>        // for wxTextFileType_Unix

#include "MessageTemplate.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the name of the standard template - i.e. the one which is used by default
#define STANDARD_TEMPLATE_NAME "Standard"

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// parser helper: return the word (sequence of alnum characters until
// endOfWordMarker) from ppc and adjust ppc pointer to point to the char after
// the end of word
//
// if quoted is true, we behave as if there were double quotes surrounding the
// word
static String ExtractWord(const wxChar **ppc,
                          wxChar endOfWordMarker,
                          bool quoted = false);

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

// parse the template expansion starting at the given position, return false if
// an error was encountered while processing it
bool
MessageTemplateParser::ExpandTemplate(const wxChar **ppc, String *value) const
{
   const wxChar *pc = *ppc;

   ASSERT_MSG( *pc == '$', _T("we should be called for $expression only") );

   // what kind of brackets do we have? some of them imply the category
   // (like $`...` is the same as $(cmd: ...))
   String category;

   // also, $`...` and $<...< imply implicit quoting as there are almost
   // surely going to be non-alnum characters inside them (they should
   // still be quoted explicitly if they contain some special token)
   bool quoted = false;

   wxChar bracketClose,
        bracketOpen = *++pc;
   switch ( bracketOpen )
   {
      case '\'': // this one is kept for compatibility only, it was a typo...
      case '`':
         bracketClose = bracketOpen;
         category = _T("cmd");
         quoted = true;
         break;

      case '<':
         bracketClose = '<';
         category = _T("file");
         quoted = true;
         break;

      case '(':
         bracketClose = ')';
         break;

      case '{':
         bracketClose = '}';
         break;

      case '$':
         // it's just escaped '$' and not start of the expansion at all
         if ( m_expander )
            *value = _T('$');
         *ppc = ++pc;
         return TRUE;

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
                         pc - m_pStartOfLine + 1,
                         m_nLine,
                         m_filename.c_str());

            return FALSE;
         }
   }

   pc++;

   // extract the next word
   String word = ExtractWord(&pc, bracketClose, quoted);

   // decide what we've got
   const int None   = 0;
   const int Left   = 1;
   const int Right  = 2;
   const int Center = 3;
   int alignment = None;
   unsigned int alignWidth = 0;
   bool truncate = FALSE;
   String name;
   wxArrayString arguments;

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
                  name = ExtractWord(&pc, bracketClose);
               }
               else
               {
                  wxLogWarning(_("Unexpected \":\" at line "
                                 "%d, position %d in the file '%s'"),
                               m_nLine,
                               pc - m_pStartOfLine,
                               m_filename.c_str());

                  return FALSE;
               }
               break;

            case '?':
               // list of arguments ahead
               {
                  String arg;
                  do
                  {
                     arg.clear();

                     // initially skip '?' (first time) or ',' (subsequent ones)
                     pc++;

                     // quoted argument?
                     bool quotedArg = *pc == '"';
                     if ( quotedArg )
                        pc++;

                     // stop on some speical chars if not quoted, otherwise
                     // only stop at the closing quote
                     while ( *pc &&
                              (quotedArg ? *pc != '"'
                                         : !strchr("+-=, ", *pc) &&
                                             *pc != bracketClose) )
                     {
                        if ( *pc == '\\' )
                        {
                           // quoted character, take as is
                           arg += *++pc;
                        }
                        else if ( *pc == '$' )
                        {
                           String subarg;
                           if ( !ExpandTemplate(&pc, &subarg) )
                           {
                              return FALSE;
                           }

                           pc--; // compensate for the increment below
                           arg += subarg;
                        }
                        else // simple char
                        {
                           arg += *pc;
                        }

                        pc++;
                     }

                     if ( quotedArg )
                     {
                        // skip closing quote or complain about missing one
                        if ( *pc == '"' )
                           pc++;
                        else
                           wxLogWarning(_("Expected closing quote at line "
                                          "%d, position %d in the file '%s'"),
                                        m_nLine,
                                        pc - m_pStartOfLine,
                                        m_filename.c_str());
                     }

                     arguments.Add(arg);
                  }
                  while ( *pc == ',' );
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
               if ( (wxSscanf(pc, _T("%u"), &alignWidth) != 1) || !alignWidth )
               {
                  wxLogWarning(_("Incorrect alignment width value at line "
                                 "%d, position %d in the file '%s'."),
                               m_nLine,
                               pc - m_pStartOfLine,
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
                                 "%d, position %d in the file '%s' "
                                 "(expected \"%c\" instead)."),
                               *pc,
                               m_nLine,
                               pc - m_pStartOfLine,
                               m_filename.c_str(),
                               bracketClose);

                  return FALSE;
               }
         }
      }
   }

   // we have all the info, so we may ask the expander for the value - but
   // skip all the rest if we're not generating output
   if ( m_expander )
   {
      if ( !m_expander->Expand(category, name, arguments, value) )
      {
         // don't log the message if the value is not empty - this means that
         // the variable *is* known, but that the expansion, for some reason,
         // failed.
         if ( value->empty() )
         {
            wxLogWarning(_("Unknown variable '%s' at line %zu, position %zu "
                           "in the file '%s'."),
                         name.c_str(),
                         m_nLine,
                         pc - m_pStartOfLine - name.length(),
                         m_filename.c_str());
         }
         //else: message should have been already given

         return FALSE;
      }

      // align if necessary
      if ( alignment != None )
      {
         size_t len = value->length();
         switch ( alignment )
         {
            case Left:
               if ( alignWidth > len )
               {
                  // add some spaces
                  *value += wxString(' ', alignWidth - len);
               }
               else if ( (len > alignWidth) && truncate )
               {
                  value->Truncate(alignWidth);
               }
               //else: value is already wide enough, but we don't truncate it
               break;

            case Right:
               if ( alignWidth > len )
               {
                  // prepend some spaces
                  value->Prepend(wxString(' ', alignWidth - len));
               }
               else if ( (len > alignWidth) && truncate )
               {
                  *value = value->c_str() + (len - alignWidth);
               }
               //else: value is already wide enough, but we don't truncate it
               break;

            case Center:
               if ( alignWidth > len )
               {
                  // prepend and append some spaces
                  size_t n1 = (alignWidth - len) / 2,
                         n2 = alignWidth - n1;
                  *value = wxString(' ', n1) + *value + wxString(' ', n2);
               }
               else if ( (len > alignWidth) && truncate )
               {
                  // truncate a bit at right and a bit at left side
                  *value = value->c_str() + (len - alignWidth) / 2;
                  value->Truncate(alignWidth);
               }
               //else: value is already wide enough, but we don't truncate it
               break;

            default:
               FAIL_MSG(_T("unknown alignment value"));
         }
      }
   }

   *ppc = pc;

   return TRUE;
}

bool MessageTemplateParser::Parse(MessageTemplateSink& sink) const
{
   // const_cast
   MessageTemplateParser *self = (MessageTemplateParser *)this;

   // as this is used only for diagnostic messages, start counting from 1 - as
   // people like it (unlike the programmers)
   self->m_nLine = 1;

   // the template text may be coming from various sources, so make sure that
   // it doesn't have some weird newline convention
   wxString templateText = wxTextFile::Translate(m_templateText,
                                                 wxTextFileType_Unix);
   const wxChar *pc = templateText.c_str();
   self->m_pStartOfLine = pc;
   while ( *pc )
   {
      // find next '$'
      while ( *pc && *pc != '$' )
      {
         if ( m_expander )
         {
            // normal text goes to the output as is
            sink.Output(*pc);
         }

         if ( *pc == '\n' )
         {
            self->m_nLine++;
            self->m_pStartOfLine = pc;
         }

         pc++;
      }

      if ( !*pc )
         break;

      String value;
      if ( !ExpandTemplate(&pc, &value) )
      {
         // error message already given
         return FALSE;
      }

      sink.Output(value);
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

String ExtractWord(const wxChar **ppc, wxChar endOfWordMarker, bool forceQuotes)
{
   const wxChar *pc = *ppc;

   bool quoted = *pc == '"';
   if ( quoted )
      pc++;

   String word;
   while ( *pc && (*pc != '\n') &&
           ((quoted && *pc != '"') ||
            ((forceQuotes || wxIsalnum(*pc)) && *pc != endOfWordMarker)) )
   {
      if ( quoted && *pc == '\\' )
      {
         // unless it's the last character in the string, backslash quotes
         // the next one (it may be used to insert quotes into templates)
         pc++;
         if ( !*pc || *pc == '\n' )
         {
            // oops... nothing to quote: rollback and insert just '\\'
            pc--;
         }
         //else: we will insert the next character below
      }

      word += *pc++;
   }

   if ( quoted && *pc == '"' )
   {
      // skip the closing quote
      pc++;
   }

   *ppc = pc;

   return word;
}

// returns the name of profile subgroup for the templates of the given kind
static String GetTemplateKindPath(MessageTemplateKind kind)
{
   String path;

   switch ( kind )
   {
      case MessageTemplate_NewMessage:
         path = MP_TEMPLATE_NEWMESSAGE;
         break;

      case MessageTemplate_NewArticle:
         path = MP_TEMPLATE_NEWARTICLE;
         break;

      case MessageTemplate_Reply:
         path = MP_TEMPLATE_REPLY;
         break;

      case MessageTemplate_Followup:
         path = MP_TEMPLATE_FOLLOWUP;
         break;

      case MessageTemplate_Forward:
         path = MP_TEMPLATE_FORWARD;
         break;

      default:
         FAIL_MSG(_T("unknown template kind"));
   }

   return path;
}

// returns the relative profile path for the name of the template of the given
// kind (Template_<kind>)
static String GetTemplateNamePath(MessageTemplateKind kind)
{
   String path;
   path << _T("Template_") << GetTemplateKindPath(kind);

   return path;
}

// returns the profile for the templates of given kind
//
// the pointer must be DecRef()'d by caller if not NULL
static Profile *GetTemplateProfile(MessageTemplateKind kind)
{
   return Profile::CreateTemplateProfile(GetTemplateKindPath(kind));
}

// ----------------------------------------------------------------------------
// public API
// ----------------------------------------------------------------------------

// get the value of the message template for the given profile: it looks up
// Template/<kind> in the given profile and then reads the corresponding
// template value from /Templates/<kind>
extern String
GetMessageTemplate(MessageTemplateKind kind, Profile *profile)
{
   // first read the template name
   String name = profile->readEntry(GetTemplateNamePath(kind),
                                    STANDARD_TEMPLATE_NAME);

   return GetMessageTemplate(kind, name);
}

// read the template value
extern String
GetMessageTemplate(MessageTemplateKind kind, const String& name)
{
   String value;

   Profile *profile(GetTemplateProfile(kind));
   if ( profile )
   {
      // the templates contain '$'s so disable variable expansion for now
      {
         ProfileEnvVarSave noEnvVarExpansion(profile);

         value = profile->readEntry(name);
      } // destroy ProfileEnvVarSave before (maybe) destroying profile

      profile->DecRef();
   }

   if ( value.empty() )
   {
      // we have the default templates for reply, follow-up and forward
      switch ( kind )
      {
         case MessageTemplate_Reply:
            value = _("On $(ORIGINAL:DATE) $SENDER wrote:\n\n$QUOTE\n$CURSOR");
            break;

         case MessageTemplate_Forward:
            value = _("$CURSOR\n\n"
                      "------ Forwarded message ------\n"
                      "From: ${ORIGINAL:FROM}\n"
                      "Date: ${ORIGINAL:DATE}\n"
                      "Subject: ${ORIGINAL:SUBJECT}\n"
                      "To: ${ORIGINAL:TO}\n"
                      "\n"
                      "${ORIGINAL:TEXT}\n"
                      "-------- End of message -------");
            break;

         case MessageTemplate_Followup:
            value = _("On $(ORIGINAL:DATE) $SENDER wrote in "
                      "$(ORIGINAL:NEWSGROUPS)\n\n$QUOTE\n$CURSOR");
            break;

         default:
            FAIL_MSG(_T("unknown template kind"));
            // fall through

         case MessageTemplate_NewMessage:
         case MessageTemplate_NewArticle:
            // put the cursor before the signature
            value = _T("$CURSOR");
      }

      // all default templates include the signature
      value += _T("\n$SIGNATURE");
   }

   return value;
}

extern void
SetMessageTemplate(const String& name,
                   const String& value,
                   MessageTemplateKind kind,
                   Profile *profile)
{
   if ( profile )
   {
      (void)profile->writeEntry(GetTemplateNamePath(kind), name);
   }

   Profile_obj profileTemplate(GetTemplateProfile(kind));
   if ( profileTemplate )
   {
      profileTemplate->writeEntry(name, value);
   }
}

extern bool
DeleteMessageTemplate(MessageTemplateKind kind, const String& name)
{
   // TODO: check that no profile uses it?
   Profile_obj profileTemplate(GetTemplateProfile(kind));
   if ( profileTemplate )
      profileTemplate->DeleteEntry(name);

   return true;
}

extern wxArrayString
GetMessageTemplateNames(MessageTemplateKind kind)
{
   wxArrayString names;

   // always add the "Standard" template to the list as it is always present
   names.Add(STANDARD_TEMPLATE_NAME);

   Profile_obj profileTemplate(GetTemplateProfile(kind));

   wxString name;
   Profile::EnumData cookie;
   bool cont = profileTemplate->GetFirstEntry(name, cookie);
   while ( cont )
   {
      // we already did it for this one above
      if ( name != STANDARD_TEMPLATE_NAME )
      {
         names.Add(name);
      }

      cont = profileTemplate->GetNextEntry(name, cookie);
   }

   return names;
}

class StringTemplateSink : public MessageTemplateSink
{
public:
    virtual bool Output(const String& text)
    {
         m_output += text;
         return true;
    }

    const String& GetOutput() const { return m_output; }

private:
    String m_output;
};

extern String
ParseMessageTemplate(const String& templateText,
                     MessageTemplateVarExpander& expander)
{
   MessageTemplateParser parser(templateText, _("no file"), &expander);

   String text;

   StringTemplateSink sink;
   if ( parser.Parse(sink) )
      text = sink.GetOutput();

   return text;
}
