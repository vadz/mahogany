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

/** Reads the next token from the string and removes it from it.
    @param str   string to parse
*/
Token ReadToken(String &str, bool remove = true)
{
   Token token;
   strutil_delwhitespace(str);
   String tokstr;
   const char *cptr = str.c_str();

   if(strutil_isempty(str))
   {
      token.SetEOF();
      return token;
   }
   if(isalpha(str[0u])) // this must be an identifier
   {
      while(isalpha(*cptr) || isdigit(*cptr) || *cptr == '_')
         tokstr += *(cptr++);
      token.SetIdentifier(tokstr);
   }
   if(isdigit(str[0u])) // it is a number
   {
      while(isdigit(*cptr))
         tokstr += *(cptr++);
      token.SetNumber(atol(tokstr.c_str()));
   }
   if(str[0u] == '"') // a quoted string
   {
      cptr = str.c_str()+1;
      bool escaped = false;
      while(*cptr && (*cptr != '"' || escaped))
      {
         if(*cptr == '\\')
         {
            if(! escaped) // first backslash
            {
               escaped = true;
               cptr++;
               continue; // skip it
            }
         }               
         tokstr += *(cptr++);
         escaped = false;
      }
      token.SetString(tokstr);
   }
   // All other cases: return the character:
   token.SetChar(*(cptr++));
                  
   if(remove)
      str = String(cptr);
   return token;
}

/** This class is used to build a syntax tree.
 */
class FilterNode
{
   
};

/** A FUNCTIONCALL has a logical value:
    FUNCTIONCALL:
       IDENTIFIER ( args )
*/

class Functioncall : public FilterNode
{
public:
   static Functioncall *Create(Token &token, String &str);
private:
   String m_name;
};

Functioncall *
Functioncall::Create(Token &token, String &str)
{
   ASSERT(token.GetType() == TT_Identifier);
   Functioncall *i = new Functioncall();
   i->m_name = token.GetIdentifier();

   // Need to swallow argument list?
   Token t = ReadToken(str, false);
   if(t.GetType() == TT_Char && t.GetChar() == '(')
   {
      (void) ReadToken(str); // swallow it
      do
         t = ReadToken(str);
      while(! (t.GetType() == TT_Char && t.GetChar() == ')'));
   }
   return i;
}

/** A CONDITION has a logical value:
    CONDITION:
       FUNCTIONCALL
       ( CONDITION )
       ( CONDITION OPERATOR CONDITION )
*/

class Condition : public FilterNode
{
public:
   /** Create a condition from token+str */
   static Condition *Create(Token &token, String str);
private:
};

Condition *
Condition::Create(Token &token, String str)
{
   ASSERT(token.GetType() == TT_Identifier ||
          token.GetType() == TT_Char && token.GetChar() == '(');
   
}

/**
   This function is the actual parser. It parses the string passed as
   argument and returns a pointer to the root of the tree representing 
   the expression.
*/
FilterNode *ParseString(String &str)
{
   Token t = ReadToken(str);
   return Condition::Create(t,str);
}
