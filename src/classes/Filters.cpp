/*-*- c++ -*-********************************************************
 * Filters - Filtering code for M                                   *
 *                                                                  *
 * (C) 1999 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "Filters.h"
#endif

#include   "Mpch.h"
#ifndef USE_PCH
#   include   "strutil.h"
#   include   "Mcommon.h"
#endif

#include <stdlib.h>
#include <ctype.h>

/// Type of tokens
enum TokenType
{ TT_Invalid = -1, TT_Char, TT_Identifier, TT_String, TT_Number, TT_EOF };

/** Tokens are either an ASCII character or a value greater >255.
 */
class Token
{
public:
   Token() { m_type = TT_Invalid; }
   inline TokenType GetType(void)
      { return m_type; }
   inline char GetChar(void)
      { ASSERT(m_type == TT_Char); return m_char; }
   inline const String & GetString(void)
      { ASSERT(m_type == TT_String); return m_string; }
   inline const String & GetIdentifier(void)
      { ASSERT(m_type == TT_Identifier); return m_string; }
   inline long GetNumber(void)
      { ASSERT(m_type == TT_Number); return m_number; }
   
   inline void SetChar(char c)
      { m_type = TT_Char; m_char = c; }
   inline void SetString(String const &s)
      { m_type = TT_String; m_string = s; }
   inline void SetIdentifier(String const &s)
      { m_type = TT_Identifier; m_string = s; }
   inline void SetNumber(long n)
      { m_type = TT_Number; m_number = n; }
   inline void SetEOF(void)
      { m_type = TT_EOF; }
private:
   TokenType m_type;
   char      m_char;
   String    m_string;
   long      m_number;
};


/** A Parser class.
 */
class Parser : public MObjectRC
{
public:
   virtual class SyntaxNode * Parse(void) = 0;
  /** Reads the next token from the string and removes it from it.
      @param str   string to parse
      @param remove if true, move on in input
  */
   virtual Token GetToken(bool remove = true) = 0;
   /// Returns the next token without removing it from the input.
   virtual Token PeekToken(void) = 0;
   /// Rewinds the input to the given position.
   virtual void Rewind(size_t pos = 0) = 0;
   /// Returns current position in input.
   virtual size_t GetPos(void) const = 0;
   /** Takes an error message and passes it to the program, sets error 
       flag.
       @param error message describing error
   */
   virtual void Error(const String &error) = 0;
   /// virtual destructor
   virtual ~Parser(void) {}
   /** Constructor function, returns a valid Parser object.
       @param input the input string holding the text to parse
   */
   static Parser * Create(const String &input);
};


// ----------------------------------------------------------------------------
// A smart reference to Parser - an easy way to avoid memory leaks
// ----------------------------------------------------------------------------

class Parser_obj
{
public:
   // ctor & dtor
   Parser_obj(const String& name)
      { m_Parser = Parser::Create(name); }
   ~Parser_obj()
      { SafeDecRef(m_Parser); }
   // provide access to the real thing via operator->
   Parser *operator->() const { return m_Parser; }
private:
   // no assignment operator/copy ctor
   Parser_obj(const Parser_obj&);
   Parser_obj& operator=(const Parser_obj&);
   Parser *m_Parser;
};

// ----------------------------------------------------------------------------
// An implementation of the Parser ABC
// ----------------------------------------------------------------------------
class ParserImpl : public Parser
{
public:
   ParserImpl(const String &input)
      {
         m_Input = input;
         Rewind();
      }
   class SyntaxNode * Parse(void);
   class SyntaxNode * ParseExpression(void);
   class FunctionCall *ParseFunctionCall(void);
   virtual size_t GetPos(void) const { return m_Position; }
   virtual inline void Rewind(size_t pos = 0) { m_Position = pos; }
   virtual Token GetToken(bool remove = true);
   virtual inline Token PeekToken(void) { return GetToken(false); }
   virtual void Error(const String &error);
protected:
   inline void EatWhiteSpace(void)
      { while(isspace(m_Input[m_Position])) m_Position++; }
   inline const char Char(void) const
      { return m_Input[m_Position]; }
   inline const char CharInc(void)
      { return m_Input[m_Position++]; }
private:
   String m_Input;
   size_t m_Position;
};

// ----------------------------------------------------------------------------
// ABC for syntax tree nodes.
// ----------------------------------------------------------------------------
/** This class is used to build a syntax tree.
 */
class SyntaxNode
{
   
};

class FunctionCall : public SyntaxNode
{
public:
   String m_name;
   String m_args;
};


/* static */
Parser *
Parser::Create(const String &input)
{
   return new ParserImpl(input);
}

void
ParserImpl::Error(const String &error)
{
   String tmp;
   tmp.Printf(_("Parse error at input position %lu:\n  %s"),
              (unsigned long) GetPos(), error.c_str());
   ERRORMESSAGE((tmp));
}

/** Reads the next token from the string and removes it from it.
    @param str   string to parse
*/
Token
ParserImpl::GetToken(bool remove)
{
   Token token;
   String tokstr;

   size_t OldPos = GetPos();

   EatWhiteSpace();
   
   if(! Char())
   {
      token.SetEOF();
      return token;
   }

   if(isalpha(Char())) // this must be an identifier
   {
      while(isalpha(Char()) || isdigit(Char()) || Char() == '_')
         tokstr += CharInc();
      token.SetIdentifier(tokstr);
   }

   if(isdigit(Char())) // it is a number
   {
      while(isdigit(Char()))
         tokstr += CharInc();
      token.SetNumber(atol(tokstr.c_str()));
   }

   if(Char() == '"') // a quoted string
   {
      m_Position++;
      bool escaped = false;
      while(Char() && (Char() != '"' || escaped))
      {
         if(Char() == '\\')
         {
            if(! escaped) // first backslash
            {
               escaped = true;
               m_Position++;
               continue; // skip it
            }
         }               
         tokstr += CharInc();
         escaped = false;
      }
      token.SetString(tokstr);
   }
   // All other cases: return the character:
   token.SetChar(CharInc());
                  
   if(! remove)
      Rewind(OldPos);
   return token;
}

SyntaxNode *
ParserImpl::Parse(void)
{
   return ParseExpression();
}

FunctionCall *
ParserImpl::ParseFunctionCall(void)
{
   Token t = GetToken();
   ASSERT(t.GetType() == TT_Identifier);

   FunctionCall *f = new FunctionCall();
   f->m_name = t.GetIdentifier();

   // Need to swallow argument list?
   t = PeekToken();
   if(t.GetType() != TT_Char || t.GetChar() != '(')
   {
      String err;
      err.Printf(_("Functioncall expected '(' after '%s'."),
                 f->m_name.c_str());
      Error(err);
      delete f;
      return NULL;
   }
   (void) GetToken(); // swallow '('
   EatWhiteSpace();
   bool escaped = false;
   while(Char() && (Char() != ')' || escaped))
   {
      if(Char() == '\\')
      {
         if(escaped)
            f->m_args += CharInc();
         else
            escaped = true;
      }
      else
         f->m_args += CharInc();
   }
   return f;
}

SyntaxNode *
ParserImpl::ParseExpression(void)
{
   /* EXPRESSION :=
        | IDENTIFIER( ARGLIST )
        | ( EXPRESSION )
        | EXPRESSION LOGICAL_OPERATOR EXPRESSION
   */
   // For now, we do only a simple function call for a test.
   Token t = PeekToken();
   if(t.GetType() == TT_Identifier)
   {
      // IDENTIFIER ( ARGLIST )
      return ParseFunctionCall();
   }
   else if(t.GetType() == TT_Char && t.GetChar() == '(')
   {
      // ( EXPRESSION )
   }
   else
   {
      Error(_("No valid expression."));
      return NULL;
   }
   return NULL;
}

/** A FUNCTIONCALL has a logical value:
    FUNCTIONCALL:
       IDENTIFIER ( args )
*/

class Functioncall : public SyntaxNode
{
public:
   static Functioncall *Create(Token &token, Parser &p);
private:
   String m_name;
};


/** A CONDITION has a logical value:
    CONDITION:
       FUNCTIONCALL
       ( CONDITION )
       ( CONDITION OPERATOR CONDITION )
*/

class Condition : public SyntaxNode
{
public:
   /** Create a condition from token+str */
   static Condition *Create(Token &token, Parser &p);
private:
};

Condition *
Condition::Create(Token &token, Parser &p)
{
   ASSERT(token.GetType() == TT_Identifier ||
          token.GetType() == TT_Char && token.GetChar() == '(');
   
   return NULL;
}

