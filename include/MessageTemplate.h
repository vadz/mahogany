///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   MessageTemplate.h - working with message templates
// Purpose:     classes and functions to parse the message templates (see the
//              description of what these things are below).
// Author:      Vadim Zeitlin
// Modified by:
// Created:     11.07.99
// CVS-ID:      $Id$
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

/* About message templates.

   They may be configured by user to be used for message composition (different
   ones for a mail message and a news article) and reply/forward. They contain
   text which is copied as is into the new message on its creation and also the
   variables expansion (or rather function calls because this expansion may
   involve all kind of things). The variables are substituted with their value
   and this value is copied into the mail message.

   There are also pseudovariables which don't expand to anything, but rather
   have some side-effect when expanded - for example, $CURSOR is a useful
   directive telling the program to leave the cursor in this place after the
   end of expansion.

   Variable expansion has the following syntax (pseudo BN notation ahead):

      $([category:]name[(arguments)][{+|-|=}<number>[!]])

   (the parentheses around all this may be replaced with accolades). For the
   simplest cases (no category, no arguments, no justficiation tail) the
   parentheses may be omitted entirely, as in $DATE.

   The "category" is one of the elements of gs_templateVarCategories elements.
   The "name"s are in gs_templateVarNames array. Both of them are "words", i.e.
   are sequences of alphabetic characters.

   The optional tail {+|-|=}<number> may be used to justify the value: + aligns
   it to the right, - (default) to the left and = centers it in the text field
   of width "number". If the number is followed by '!', the value is truncated
   if it doesn't fit into the given width instead of taking as much place as is
   required for it (default behaviour).

 */

#ifndef _MESSAGETEMPLATE_H_
#define _MESSAGETEMPLATE_H_

// ----------------------------------------------------------------------------
// MessageTemplateSink is the ABC for classes which receive the generated (from
// template message) output.
// ----------------------------------------------------------------------------

class MessageTemplateSink
{
public:
   // add a string to the output
   virtual bool Output(const String& text) = 0;

   // virtual dtor in any base class
   virtual ~MessageTemplateSink();
};

// ----------------------------------------------------------------------------
// MessageTemplateVarExpander is the ABC for classes doing variable expansion.
// Each of its descendants must define Expand() function which is used by
// MessageTemplateParser.
// ----------------------------------------------------------------------------

class MessageTemplateVarExpander
{
public:
   // expands the variable 'name' in the category 'category' returning the
   // value in 'value' if the variable is known to us - otherwise just return
   // FALSE without changing 'value' at all.
   virtual bool Expand(const String& category,
                       const String& name,
                       String *value) const = 0;

   // virtual dtor in any base class
   virtual ~MessageTemplateVarExpander();
};

// ----------------------------------------------------------------------------
// MessageTemplateParser is the class which does the parsing of the templates.
// It may be used for just checking template for the syntax correctness or to
// generate the message from this template, but for this it needs a pointer to
// MessageTemplateVarExpander object whose Expand() function will be called to
// actually perform the variable expansion.
// ----------------------------------------------------------------------------

class MessageTemplateParser
{
public:
   // ctor takes in the text of the template, the filename for the error
   // messages and, optionally, a MessageTemplateVarExpander object which will
   // be used in Parse() (otherwise, Parse() won't generate any output).
   MessageTemplateParser(const String& templateText,
                         const String& filename,
                         MessageTemplateVarExpander *expander)
      : m_templateText(templateText), m_filename(filename)
   {
      m_expander = expander;
   }

   // parses the template and, if we have a MessageTemplateVarExpander,
   // generate the output. Returns FALSE if the template syntax is incorrect.
   bool Parse(MessageTemplateSink& sink) const;

private:
   MessageTemplateVarExpander *m_expander;
   String m_templateText, m_filename;
};

#endif // _MESSAGETEMPLATE_H_
