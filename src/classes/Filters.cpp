/*-*- c++ -*-********************************************************
 * Filters - Filtering code for M                                   *
 *                                                                  *
 * (C) 1999 by Karsten Ballüder (ballueder@gmx.net)                 *
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

/* The syntax:

   CONDITION :=
     | CONDITION & EXPRESSION  
     | CONDITION | EXPRESSION
     | EXPRESSION

   EXPRESSION :=
     | EXPR + TERM
     | EXPR - TERM
     | TERM   

   TERM :=
     | TERM * CONDITION
     | TERM / CONDITION
     | CONDITION

     
   FACTOR :=
     | NUMBER
     | IDENTIFIER ( EXPRESSION )
     | ( EXPRESSION )
     | ! FACTOR

     With the following rewritten rules being used:

     CONDITION :=
       | EXPRESSION RESTCONDITION

     RESTCONDITION :=  
       | & EXPRESSION
       | | EXPRESSION
       | EMPTY
       
     EXPRESSION := TERM RESTEXPRESSION
     RESTEXPRESSION := 
       | + EXPRESSION
       | - EXPRESSION
       | EMPTY

     TERM := FACTOR RESTTERM
     RESTTERM := 
       | * TERM
       | / TERM
       | EMPTY

*/



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
   class SyntaxNode * ParseCondition(void);
   void  ParseRestExpression(class Expression *expression);
   void  ParseRestTerm(class Expression *expression);
   void  ParseRestCondition(class Expression *expression);
   class ArgList    * ParseArgList(void);
   class SyntaxNode *ParseFunctionCall(void);
   class SyntaxNode * ParseTerm(void);
   class SyntaxNode * ParseFactor(void);
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
   virtual long Evaluate(long left, long right) const = 0;
   /// for logical operators, shall we continue parsing the right?
   virtual bool TestLeft(long left) const = 0;
   /// Returns the priority  of the operator.
   virtual int  Priority(void) const = 0;
#ifdef DEBUG
   virtual String Debug(void) const = 0;
#endif
};

/// The numbers returned by Operator::Priority()
enum OperatorPriorities
{
   /// standard, lowest
   PRI_LOWEST,
   /// boolean operators:
   PRI_BOOLEAN,
   /// for addition and substraction
   PRI_PLUS_MINUS,
   /// for multiplication and division
   PRI_TIMES_DIV,
   /// the exponential operator
   PRI_EXPONENTIONAL
};

// ----------------------------------------------------------------------------
// ABC for syntax tree nodes.
// ----------------------------------------------------------------------------
/** This class is used to build a syntax tree.
 */
class SyntaxNode : public MObject
{
public:
   virtual long Evaluate() const = 0;
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
   virtual long Evaluate() const
      { MOcheck(); return m_value; }
#ifdef DEBUG
   virtual String Debug(void) const
      { MOcheck(); String s; s.Printf("%ld",m_value); return s; }
#endif    
private:
   long m_value;
};

class StringValue : public SyntaxNode
{
public:
   StringValue(String v) { m_Value = v; }
   virtual long Evaluate() const
      { MOcheck(); return m_Value.Length(); }
#ifdef DEBUG
   virtual String Debug(void) const
      {
         MOcheck();
         String s;
         s << '"' << m_Value << '"';
         return s;
      }
#endif    
private:
   String m_Value;
};

class Negation : public SyntaxNode
{
public:
   Negation(class SyntaxNode *sn) { m_Sn = sn; }
   ~Negation() { MOcheck(); delete m_Sn; }
   virtual long Evaluate() const
      { MOcheck(); return !m_Sn->Evaluate(); }
#ifdef DEBUG
   virtual String Debug(void) const
      {
         MOcheck();
         String s;
         s << "!(" << m_Sn->Debug() << ')';
         return s;
      }
#endif    
private:
   SyntaxNode *m_Sn;
};

/** Functioncall handling */

extern "C" {
   typedef long (*FunctionPointer)(ArgList *args);

   static long echo_func(ArgList *args)
      {  INFOMESSAGE(("echo_func")); return 1; }

   static long func_rFive(ArgList *args)
      {  return 5; }
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
   virtual long Evaluate() const;
#ifdef DEBUG
   virtual String Debug(void) const
      { MOcheck(); return String("FunctionCall(") + m_name + String(")"); }
#endif    
private:
   FunctionPointer FindFunction(void);

   FunctionPointer m_function;
   ArgList *m_args;
   String  m_name;
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
   { "returnFive", func_rFive },
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
FunctionCall::Evaluate() const
{
   MOcheck();
   ASSERT(m_function);
   return (*m_function)(m_args);
}


/** An expression consisting of more than one token. */
class Expression : public SyntaxNode
{
public:
   Expression(SyntaxNode *left = NULL,
              Operator   *op = NULL,
              SyntaxNode *right = NULL)
      { m_Left = left; m_Op = op; m_Right = right; }
   virtual long Evaluate(void) const
      {
         MOcheck();
         if(m_Op)
            return m_Op->Evaluate(m_Left->Evaluate(),
                                   m_Right->Evaluate());
         else
         {
            ASSERT(m_Right == NULL);
            return m_Left->Evaluate();
         }
      }
   void SetLeft(SyntaxNode *left)
      { MOcheck(); ASSERT(m_Left == NULL); m_Left = left; }
   void SetRight(SyntaxNode *right)
      { MOcheck(); ASSERT(m_Right == NULL); m_Right = right; }
   void SetOperator(Operator *op)
      { MOcheck(); ASSERT(m_Op == NULL); m_Op = op; }
   ~Expression()
      {
         MOcheck();
         if(m_Left) delete m_Left;
         if(m_Op) delete m_Op;
         if(m_Right)  delete m_Right;
      }
#ifdef DEBUG
   virtual String Debug(void) const
      {
         MOcheck();
         String s = "(";
         s << m_Left->Debug() ;
         if(m_Op)
            s << m_Op->Debug() << m_Right->Debug();
         else
         {
            ASSERT(m_Right == NULL);
         }
         s << ')';
         return s;
      }
#endif    
private:
   SyntaxNode *m_Left, *m_Right;
   class Operator *m_Op;
};

#ifdef DEBUG
#define IMPLEMENT_OP(name, oper, priority) \
class Operator##name : public Operator \
{ \
public: \
   virtual long Evaluate(long left, long right) const \
      { MOcheck(); return left oper right; } \
   virtual bool TestLeft(long left) const \
      { MOcheck(); return (bool) (left oper 1) != 0; } \
   virtual int Priority(void) const \
      { return priority; } \
   virtual String Debug(void) const \
      { MOcheck(); return #oper; } \
}
#else
#define IMPLEMENT_OP(name, oper) \
class Operator##name : public Operator \
{ \
public: \
   virtual long Evaluate(long left, long right) const \
      { return left oper right; } \
   virtual bool TestLeft(long left) const \
      { MOcheck(); return (bool) (left oper 1) != 0; } \
}
#endif


IMPLEMENT_OP(Or,||, PRI_BOOLEAN);
IMPLEMENT_OP(And,&&, PRI_BOOLEAN);
IMPLEMENT_OP(Plus,+, PRI_PLUS_MINUS);
IMPLEMENT_OP(Minus,-, PRI_PLUS_MINUS);
IMPLEMENT_OP(Times,*, PRI_TIMES_DIV);
IMPLEMENT_OP(Divide,/, PRI_TIMES_DIV);


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

   String functionName = t.GetIdentifier();
   
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
ParserImpl::ParseFactor(void)
{
   MOcheck();
   /* FACTOR :=
        | ! FACTOR
        | ( EXRESSION ) 
        | IDENTIFIER( ARGLIST )
        | NUMBER
        | "STRING"
   */
   SyntaxNode *sn = NULL;
   Token t = PeekToken();

   /* First case, expression in brackets: */
   if(t.GetType() == TT_Char)
   {
      if( t.GetChar() == '(')
      {
         t = GetToken();
         sn = ParseExpression();
         t = PeekToken();
         if(t.GetType() == TT_Char && t.GetChar() == ')')
            GetToken();
         else
         {
            if(sn) delete sn;
            Error("Expected ')' after expression.");
            return NULL;
         }
      }
      else if(t.GetChar() == '!')
      {
         GetToken();
         sn = ParseFactor();
         if(!sn)
         {
            Error(_("Expected Factor after negation operator."));
            return NULL;
         }
         else
            return new Negation(sn);
      }
   }
   else if( t.GetType() == TT_String )
   {
      GetToken();
      return new StringValue(t.GetString());
   }
   else if(t.GetType() == TT_Identifier)
   {
      // IDENTIFIER ( ARGLIST )
      sn = ParseFunctionCall();
   }
   else
   {
      if(t.GetType() != TT_Number)
         Error(_("Expected a number or a function call."));
      else
      {
         sn = new Value(t.GetNumber());
         GetToken();
      }
   }
   return sn;
}


void
ParserImpl::ParseRestCondition(Expression *condition)
{
   MOcheck();
   ASSERT(condition);
   
   /* RESTCONDITION :=
      | & CONDITION
      | | CONDITION
      | EMPTY
   */
   Token t = PeekToken();

   if(t.GetType() == TT_EOF
      || ( t.GetType() == TT_Char
           && t.GetChar() != '&'
           && t.GetChar() != '|')
      )
      return; 

   GetToken();
   SyntaxNode * expr = ParseCondition();
   if(! expr)
   {
      Error(_("Expected Condition after conditional operator."));
      return;
   }
   condition->SetRight(expr);
   switch(t.GetChar())
   {
   case '&':
      condition->SetOperator(new OperatorAnd);break;
   case '|':
      condition->SetOperator(new OperatorOr);break;
   default:
      ASSERT(0);
   }
}

void
ParserImpl::ParseRestExpression(Expression *expression)
{
   MOcheck();
   ASSERT(expression);
   
   /* RESTEXPRESSION :=
      | + TERM
      | - TERM
      | EMPTY

      The term passed as argument is the left side of the operator in
      the original expression. We return not the rest expression but
      the full expression here, so we need this.
   */
   Token t = PeekToken();

   if(t.GetType() == TT_EOF
      || (t.GetType() == TT_Char
          && t.GetChar() != '+'
          && t.GetChar() != '-')
      )
      return; 

   GetToken();
   SyntaxNode * exp = ParseExpression();
   if(! exp)
   {
      Error(_("Expected expression after operator."));
      return;
   }
   expression->SetRight(exp);
   switch(t.GetChar())
   {
   case '+':
      expression->SetOperator(new OperatorPlus);break;
   case '-':
      expression->SetOperator(new OperatorMinus);break;
   default:
      ASSERT(0);
   }
}


void
ParserImpl::ParseRestTerm(Expression *expression)
{
   MOcheck();
   /* RESTTERM :=
      | * FACTOR
      | / FACTOR
      | EMPTY

      The term passed as argument is the left side of the operator in
      the original expression. We return not the rest expression but
      the full expression here, so we need this.
   */
   Token t = PeekToken();
   if(t.GetType() == TT_EOF
      ||
      ( t.GetType() == TT_Char
        && t.GetChar() != '/'
        && t.GetChar() != '*'))
      return;
   
   t = GetToken(); // eat operator
   char token = t.GetChar();
   SyntaxNode *term = ParseTerm();
   if(! term)
   {
      Error(_("Expected term after operator."));
      return;
   }
   expression->SetRight(term);
   switch(token)
   {
   case '*':
      expression->SetOperator(new OperatorTimes);break;
   case '/':
      expression->SetOperator(new OperatorDivide);break;
   default:
      ASSERT(0);
   }
}


SyntaxNode *
ParserImpl::ParseCondition(void)
{
   MOcheck();
   /* CONDITION :=
      | EXPRESSION & EXPRESSION
      | EXPRESSION | EXPRESSION
      | EXPRESSION   

      Which is identical to the following expression, resolving the
      left recursion problem:
      
      CONDITION := EXPRESSION RESTCONDITION    
      
   */
   SyntaxNode *expr = ParseExpression();
   if(expr)
   {
      Expression *cond = new Expression(expr);
      ParseRestCondition(cond);
      return cond;
   }
   return expr;
}


SyntaxNode *
ParserImpl::ParseExpression(void)
{
   MOcheck();
   /* EXPRESSION :=
      | EXPRESSION + TERM
      | EXPRESSION - TERM
      | TERM   

      Which is identical to the following expression, resolving the
      left recursion problem:
      
      EXPRESSION :=
      | TERM RESTEXPRESSION   
      
   */
   SyntaxNode *sn = ParseTerm();
   if(sn)
   {
      Expression *exp = new Expression(sn);
      ParseRestExpression(exp);
      return exp;
   }
   return NULL;
}

SyntaxNode *
ParserImpl::ParseTerm(void)
{
   MOcheck();
   /* TERM :=
      | TERM * FACTOR
      | TERM / FACTOR
      | FACTOR   

      Which is identical to the following expression, resolving the
      left recursion problem:
      
      EXPRESSION :=
      | FACTOR RESTTERM   
      
   */
   SyntaxNode *factor = ParseFactor();
   if(factor)
   {
      Expression *exp = new Expression(factor);
      ParseRestTerm(exp);
      return exp;
   }
   return factor;
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


/***************************************************************************/
                                                                            

/** FOR TESTING ONLY, called from MAppBase::OnStartup(): **/

#if 0
static const char *testprogram = " 5 + 4 * ( 3 * 4 ) + 5 * print() ";
static const char *testprogram = " 5 + 4 * 4 -1";
#endif


void FilterTest(const String &program)
{
   Parser *p = Parser::Create(program);
   SyntaxNode *sn = p->Parse();
   if(sn)
   {
      String str = sn->Debug();
      INFOMESSAGE((str.c_str()));
      str.Printf("Evaluated '%s' = %ld",
                 program.c_str(), sn->Evaluate());
      INFOMESSAGE((str.c_str()));
      p->DecRef();
      delete sn;
   }
   else
   {
      ERRORMESSAGE(("Parsing '%s' failed.", program.c_str()));
   }
}
