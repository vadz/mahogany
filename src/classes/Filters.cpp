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
   class ArgList    * ParseArgList(void);
   class SyntaxNode *ParseFunctionCall(void);
   class SyntaxNode * ParseFunctionOrConstant(void);
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
// ABC for logical operators.
// ----------------------------------------------------------------------------

/** This class represents logical operators. */
class Operator : public MObject
{
public:
   virtual long Evaluate(const class SyntaxNode *left,
                         const class SyntaxNode *right) const = 0;
#ifdef DEBUG
   virtual String Debug(const class SyntaxNode *left,
                        const class SyntaxNode *right) const = 0;
#endif
};

// ----------------------------------------------------------------------------
// ABC for syntax tree nodes.
// ----------------------------------------------------------------------------
/** This class is used to build a syntax tree.
 */
class SyntaxNode : public MObject
{
public:
   virtual long Evaluate(void) const = 0;
#ifdef DEBUG
   virtual String Debug(void) const = 0;
#endif
};



#define ARGLIST_DEFAULT_LENGTH 64

class ArgList : public MObject
{
public:
   typedef SyntaxNode *SyntaxNodePtr;
   ArgList()
      {
         m_MaxArgs = ARGLIST_DEFAULT_LENGTH;
         m_Args = new SyntaxNodePtr[m_MaxArgs];
         m_NArgs = 0;
      }
   ~ArgList()
      {
         for(size_t i = 0; i < m_NArgs; i++)
            delete m_Args[i];
         delete [] m_Args;
      }
   void Add(SyntaxNode *arg)
      {
         MOcheck();
         if(m_NArgs == m_MaxArgs)
            return; // FIXME realloc?
         m_Args[m_NArgs++] = arg;
      }
   size_t Count(void) const
      {
         MOcheck();
         return m_NArgs;
      }
   SyntaxNode * GetArg(size_t n) const
      {
         MOcheck();
         ASSERT(n < m_NArgs);
         return m_Args[n];
      }
private:
   SyntaxNodePtr *m_Args;
   size_t m_MaxArgs;
   size_t m_NArgs;
};
   


class Value : public SyntaxNode
{
public:
   Value(long v) { m_value = v; }
   virtual long Evaluate(void) const { MOcheck(); return m_value; }
#ifdef DEBUG
   virtual String Debug(void) const { MOcheck(); String s; s.Printf("Value(%ld)",m_value); return s; }
#endif    
private:
   long m_value;
};


/** Functioncall handling */

extern "C" {
   typedef long (*FunctionPointer)(ArgList *args);

   static long echo_func(ArgList *args)
      {  INFOMESSAGE(("echo_func")); return 1; }
};

struct FunctionDefinition
{
   const char *m_name;
   FunctionPointer m_FunctionPtr;
};

class FunctionCall : public SyntaxNode
{
public:
   FunctionCall(const String &name, ArgList *args);
   ~FunctionCall() { delete m_args; }
   virtual long Evaluate(void) const;
#ifdef DEBUG
   virtual String Debug(void) const
      { MOcheck(); return String("FunctionCall(") + m_name + String(")"); }
#endif    
private:
   FunctionPointer FindFunction(void);

   FunctionPointer m_function;
   ArgList *m_args;
   String      m_name;
   static FunctionDefinition ms_Functions[];
};

FunctionCall::FunctionCall(const String &name, ArgList *args)
{
   m_name = name;
   m_args = args;
   m_function = FindFunction();
}
   

/* static */
FunctionDefinition
FunctionCall::ms_Functions[] =
{
   { "print", echo_func },
   { NULL, NULL }
};

FunctionPointer
FunctionCall::FindFunction(void)
{
   MOcheck();
   FunctionDefinition *fd = ms_Functions;
   while(fd->m_name != NULL)
   {
      if(m_name == fd->m_name)
         return fd->m_FunctionPtr;
      else
         fd++;
   }
   return NULL;
}

long
FunctionCall::Evaluate(void) const
{
   MOcheck();
   ASSERT(m_function);
   return (*m_function)(m_args);
}

/** An expression consisting of more than one token. */
class Expression : public SyntaxNode
{
public:
   Expression(const SyntaxNode *left, class Operator *op, const SyntaxNode *right)
      { m_left = left; m_right = right; m_operator = op; }
   long Evaluate(void) const
      { MOcheck(); return m_operator->Evaluate(m_left, m_right); }
   ~Expression()
      {
         MOcheck();
         if(m_left)  delete m_left;
         if(m_right) delete m_right;
         if(m_operator) delete m_operator;
      }
#ifdef DEBUG
   virtual String Debug(void) const
      {
         MOcheck();
         String s = "Expression(";
         if(m_operator) s << m_operator->Debug(m_left, m_right);
         s << ')';
         return s;
      }
#endif    
private:
   const SyntaxNode *m_left, *m_right;
   class Operator *m_operator;
};



/* This operator evaluates both operands and returns the value of the 
   right one, like ',' in C. */
class OperatorNone : public Operator
{
public:
   virtual long Evaluate(const class SyntaxNode *left,
                        const class SyntaxNode *right) const
      {
         MOcheck();
         (void) left->Evaluate();
         return right->Evaluate();
      }
#ifdef DEBUG
   String Debug(const class SyntaxNode *left, const class SyntaxNode
                *right) const
      {
         MOcheck();
         String s;
         if(left) s << left->Debug();
         s << ";";
         if(right) s << right->Debug();
         return s;
      }
#endif
};

#ifdef DEBUG
#define IMPLEMENT_OP(name, operator) \
class Operator##name : public Operator \
{ \
public: \
   virtual long Evaluate(const class SyntaxNode *left, \
                        const class SyntaxNode *right) const \
      { MOcheck(); return left->Evaluate() operator right->Evaluate(); } \
   virtual String Debug(const class SyntaxNode *left, \
                        const class SyntaxNode *right) const \
                           { MOcheck(); String s; if(left) s << left->Debug(); \
                           s << "operator"; \
                           if(right) s << right->Debug(); return s; } \
}
#else
#define IMPLEMENT_OP(name, operator) \
class Operator##name : public Operator \
{ \
public: \
   virtual long Evaluate(const class SyntaxNode *left, \
                        const class SyntaxNode *right) const \
      { return left->Evaluate() operator right->Evaluate(); } \
}
#endif


IMPLEMENT_OP(Or,||);
IMPLEMENT_OP(And,&&);
IMPLEMENT_OP(Plus,+);
IMPLEMENT_OP(Minus,-);
IMPLEMENT_OP(Times,*);
IMPLEMENT_OP(Divide,/);


/* static */
Parser *
Parser::Create(const String &input)
{
   return new ParserImpl(input);
}

void
ParserImpl::Error(const String &error)
{
   MOcheck();
   unsigned long pos = GetPos();
   String before, after, tmp;
   before = m_Input.Left(pos);
   after = m_Input.Mid(pos);
   tmp.Printf(_("Parse error at input position %lu:\n  %s\n%s<error> %s"),
              pos, error.c_str(), before.c_str(), after.c_str());

   ERRORMESSAGE((tmp));
}

/** Reads the next token from the string and removes it from it.
    @param str   string to parse
*/
Token
ParserImpl::GetToken(bool remove)
{
   MOcheck();
   Token token;
   String tokstr;

   size_t OldPos = GetPos();

   EatWhiteSpace();
   
   if(! Char())
   {
      token.SetEOF();
   }
   else if(isalpha(Char())) // this must be an identifier
   {
      while(isalpha(Char()) || isdigit(Char()) || Char() == '_')
         tokstr += CharInc();
      token.SetIdentifier(tokstr);
   }
   else if(isdigit(Char())) // it is a number
   {
      while(isdigit(Char()))
         tokstr += CharInc();
      token.SetNumber(atol(tokstr.c_str()));
   }
   else if(Char() == '"') // a quoted string
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
   else
      // All other cases: return the character:
      token.SetChar(CharInc());
                  
   if(! remove)
      Rewind(OldPos);
   return token;
}

SyntaxNode *
ParserImpl::Parse(void)
{
   MOcheck();
   return ParseExpression();
}

ArgList *
ParserImpl::ParseArgList(void)
{
   MOcheck();
   ArgList *arglist = new ArgList;
   
   Token t = PeekToken();
   for(;;)
   {
      // end of arglist?
      if(t.GetType() == TT_Char && t.GetChar() == ')')
         break;
      SyntaxNode *expr = ParseExpression();
      if(expr)
         arglist->Add(expr);
      t = PeekToken();
      if(t.GetType() != TT_Char)
      {
         Error("Expected ',' or ')' behind argument.");
         delete arglist;
         arglist = NULL;
      }
      else
      {
         if(t.GetChar() == ',')
            GetToken(); // swallow the argument
      }
   }
   return arglist;
}

SyntaxNode *
ParserImpl::ParseFunctionCall(void)
{
   MOcheck();
   Token t = GetToken();
   ASSERT(t.GetType() == TT_Identifier);

   const String & functionName = t.GetIdentifier();
   
   // Need to swallow argument list?
   t = PeekToken();
   if(t.GetType() != TT_Char || t.GetChar() != '(')
   {
      String err;
      err.Printf(_("Functioncall expected '(' after '%s'."),
                 functionName.c_str());
      Error(err);
      return NULL;
   }
   (void) GetToken(); // swallow '('

   ArgList *args = ParseArgList();

   EatWhiteSpace();
   if(Char() != ')')
      Error("Expected ')' at end of argument list.");
   else
      GetToken(); // swallow it
   
   return new FunctionCall(functionName, args);
}

SyntaxNode *
ParserImpl::ParseFunctionOrConstant(void)
{
   MOcheck();
   /* FuncionOrConstant :=
        | IDENTIFIER( ARGLIST )
        | NUMBER
   */
   SyntaxNode *sn = NULL;

   Token t = PeekToken();
   if(t.GetType() == TT_Identifier)
   {
      // IDENTIFIER ( ARGLIST )
      sn = ParseFunctionCall();
   }
   else
   {
      if(t.GetType() != TT_Number)
         Error(_("Expecting a number or a function call."));
      else
      {
         sn = new Value(t.GetNumber());
         GetToken();
      }
   }
   return sn;
}


SyntaxNode *
ParserImpl::ParseExpression(void)
{
   MOcheck();
   /* Expression :=
        | FunctonOrConstant
        | ( Expression )
        | Expression LogicalOperator Expression
   */
   SyntaxNode *sn = NULL;
   bool needParan = false;
   
   Token t = PeekToken();
   if(t.GetType() == TT_Char)
   {
      if(t.GetChar() == '(')
      {
         needParan = true;
         t = GetToken();
      }
   }

   sn = ParseFunctionOrConstant();
   
   t = PeekToken();
   if(t.GetType() == TT_Char)
   {
      char token = t.GetChar();
      switch(token)
      {
      case '&':
         GetToken(); sn = new Expression(sn, new OperatorAnd, ParseExpression()); break;
      case '|':
         GetToken(); sn = new Expression(sn, new OperatorOr, ParseExpression()); break;
      case '+':
         GetToken(); sn = new Expression(sn, new OperatorPlus, ParseExpression()); break;
      case '-':
         GetToken(); sn = new Expression(sn, new OperatorMinus, ParseExpression()); break;
      case '*':
         GetToken(); sn = new Expression(sn, new OperatorTimes, ParseExpression()); break;
      case '/':
         GetToken(); sn = new Expression(sn, new OperatorDivide, ParseExpression()); break;
      case ';':
         GetToken(); sn = new Expression(sn, new OperatorNone, ParseExpression());break;
      case ')':
         if(needParan)
         {
            Token t = PeekToken();
            if(t.GetType() == TT_Char && t.GetChar() == ')')
               GetToken();
            else
            {
               Error("   ')' needed after expression.");
               if(sn) delete sn;
               return NULL;
            }
         }
         break;
      default:
         Error("Unexpected character.");
         if(sn) delete sn;
         return NULL;
      }
   }
   return sn;
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




/** FOR TESTING ONLY, called from MAppBase::OnStartup(): **/

static const char *testprogram = " 5 + ( 3 * 4 )";

void FilterTest(void)
{
   Parser *p = Parser::Create(testprogram);
   SyntaxNode *sn = p->Parse();
   String str = sn->Debug();
   INFOMESSAGE((str.c_str()));
   str.Printf("Evaluated '%s' = %ld",
              testprogram, sn->Evaluate());
   INFOMESSAGE((str.c_str()));
   p->DecRef();
   delete sn;
}
