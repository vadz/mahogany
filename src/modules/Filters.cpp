/*-*- c++ -*-********************************************************
 * Filters - Filtering code for Mahogany                            *
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
#   include   "kbList.h"
#endif

#include "MModule.h"
#include "Mversion.h"
#include "MInterface.h"

#include <stdlib.h>
#include <ctype.h>

#include "modules/Filters.h"

/** Parsed representation of a filtering rule to be applied to a
    message.
*/
class FilterRuleImpl : public FilterRule
{
public:
   /** Apply the filter to a single message, returns 0 on success. */
   virtual int Apply(class Message *msg) const;
   /** Apply the filter to the messages in a folder.
       @param folder - the MailFolder object
       @param NewOnly - if true, apply it only to new messages
       @return 0 on success
   */
   virtual int Apply(class MailFolder *folder, bool NewOnly = true) const
      {
         ASSERT(0);
         return 0;
      }
   static FilterRule * Create(const String &filterrule,
                              MInterface *interface, MModule_Filters *mod)
      { return new FilterRuleImpl(filterrule, interface, mod); }
protected:
   FilterRuleImpl(const String &filterrule,
                  MInterface *interface,
                  MModule_Filters *fmodule);
   ~FilterRuleImpl();
private:
   class Parser     *m_Parser;
   class SyntaxNode *m_Program;
   MModule_Filters *m_FilterModule;
};


   
///------------------------------
/// Own functionality:
///------------------------------


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

/** This enum is used to distinguish variable or result types. */
enum Type
{
   /// Undefine value.
   Type_Error,
   /// Value is a long int number.
   Type_Number,
   /// Value is a string.
   Type_String
};

/** This is a value. */
class Value : public MObject
{
public:
   Value()
      {
         m_Type = Type_Error;
      }
   Value(const String &str)
      {
         m_Type = Type_String;
         m_String = str;
      }
   Value(long num)
      {
         m_Type = Type_Number;
         m_Num = num;
      }
   Type GetType(void) const
      { MOcheck(); return m_Type; }
   long GetNumber(void) const
      {
         MOcheck();
         ASSERT(m_Type == Type_Number);
         return m_Num;
      }
   String GetString(void) const
      {
         MOcheck();
         ASSERT(m_Type == Type_String);
         return m_String;
      }
   String ToString(void) const
      {
         if(m_Type == Type_String)
            return m_String;
         String str; str.Printf("%ld", m_Num);
         return str;
      }
private:
   Type m_Type;
   // Can't use union here because m_String needs constructor.
   long   m_Num;
   String m_String;
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
   /// Print out a message.
   virtual void Output(const String &msg) = 0;
   /// virtual destructor
   virtual ~Parser(void) {}
   /** Constructor function, returns a valid Parser object.
       @param input the input string holding the text to parse
   */
   static Parser * Create(const String &input, MInterface *interface);
};


// ----------------------------------------------------------------------------
// A smart reference to Parser - an easy way to avoid memory leaks
// ----------------------------------------------------------------------------

class Parser_obj
{
public:
   // ctor & dtor
   Parser_obj(const String& program, MInterface *interface)
      { m_Parser = Parser::Create(program, interface); }
   ~Parser_obj()
      { SafeDecRef(m_Parser); }
   // provide access to the real thing via operator->
   Parser *operator->() const { return m_Parser; }
   operator bool() const { return m_Parser != NULL; }
private:
   // no assignment operator/copy ctor
   Parser_obj(const Parser_obj&);
   Parser_obj& operator=(const Parser_obj&);
   Parser *m_Parser;
};

/* The syntax:

   STATEMENT :=
     | CONDITION ;
     | { STATEMENT ; RESTSTATEMENT}

   RESTSTATEMENT :=
     | STATEMENT ; RESTSTATEMENT  
     | EMPTY
     
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
     | FACTOR

     
   FACTOR :=
     | NUMBER
     | IDENTIFIER ( EXPRESSION )
     | ( EXPRESSION )
     | ! FACTOR

     With the following rewritten rules being used:

     CONDITION :=
       | EXPRESSION RESTCONDITION

     RESTCONDITION :=  
       | & CONDITION
       | | CONDITION
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
   ParserImpl(const String &input, MInterface *interface)
      {
         m_Input = input;
         Rewind();
         m_MInterface = interface;
      }
   class SyntaxNode * Parse(void);
   class SyntaxNode * ParseExpression(void);
   class Block      * ParseBlock(void);
   class SyntaxNode * ParseCondition(void);
   void  ParseRestBlock(class Block *statement);
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
   virtual void Output(const String &msg)
      {
         m_MInterface->Message(msg,NULL,_("Parser output"));
      }
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
   MInterface *m_MInterface;
};

// ----------------------------------------------------------------------------
// ABC for logical operators.
// ----------------------------------------------------------------------------

/** This class represents logical operators. */
class Operator : public MObject
{
public:
   virtual Value Evaluate(Value left, Value right) const = 0;
   /// for logical operators, shall we continue parsing the right?
   virtual bool TestLeft(Value left) const = 0;
#ifdef DEBUG
   virtual String Debug(void) const = 0;
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
   SyntaxNode(Parser *p) { m_Parser = p; }
   virtual Value Evaluate(void) const = 0;
   virtual String ToString(void) const
      { return Evaluate().ToString(); }
   Parser * GetParser(void) const { return m_Parser; }
#ifdef DEBUG
   virtual String Debug(void) const = 0;
#endif
private:
   Parser *m_Parser;
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
   

KBLIST_DEFINE(NodeList, SyntaxNode);

class Block : public SyntaxNode
{
public:
   Block(Parser *p) : SyntaxNode(p) {}
   virtual Value Evaluate() const
      {
         MOcheck();
         Value value;
         for(NodeList::iterator i = m_Nodes.begin();
             i != m_Nodes.end();
             i++)
            value = (**i).Evaluate();
         return value;
      }
   void AddNode(SyntaxNode *node)
      { m_Nodes.push_back(node); }
#ifdef DEBUG
   virtual String Debug(void) const
      {
         MOcheck();
         String s;
         s << '{';
         for(NodeList::iterator i = m_Nodes.begin();
             i != m_Nodes.end();
             i++)
            s << (**i).Debug();
         s << '}';
         return s;
      }
#endif    
private:
   NodeList  m_Nodes;
};



class Number : public SyntaxNode
{
public:
   Number(long v, Parser *p) : SyntaxNode(p) { m_value = v; }
   virtual Value Evaluate() const
      { MOcheck(); return m_value; }
#ifdef DEBUG
   virtual String Debug(void) const
      { MOcheck(); String s; s.Printf("%ld",m_value); return s; }
#endif    
private:
   long m_value;
};

class StringConstant : public SyntaxNode
{
public:
   StringConstant(String v, Parser *p) : SyntaxNode(p) { m_String = v; }
   virtual Value Evaluate() const
      { MOcheck(); return m_String; }

#ifdef DEBUG
   virtual String Debug(void) const
      {
         MOcheck();
         String s;
         s << '"' << m_String << '"';
         return s;
      }
#endif    
private:
   String m_String;
};

class Negation : public SyntaxNode
{
public:
   Negation(class SyntaxNode *sn, Parser *p) : SyntaxNode(p) { m_Sn = sn; }
   ~Negation() { MOcheck(); delete m_Sn; }
   virtual Value Evaluate() const
      {
         MOcheck();
         long num;
         Value v = m_Sn->Evaluate();
         num = v.GetType() == Type_Number ?
            v.GetNumber() : v.GetString().Length();
         return ! num;
      }
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
   typedef Value (*FunctionPointer)(ArgList *args, Parser *p);
};

struct FunctionDefinition
{
   String          m_Name;
   FunctionPointer m_FunctionPtr;
   size_t          m_UseCount;

   FunctionDefinition(const String &name, FunctionPointer fptr)
      {
         m_Name = name; m_FunctionPtr = fptr; m_UseCount = 0;
         ASSERT(m_FunctionPtr);
      }
   ~FunctionDefinition()
      {
         ASSERT(m_UseCount == 0);
      }
};


KBLIST_DEFINE(FunctionList, FunctionDefinition);

class FunctionCall : public SyntaxNode
{
public:
   FunctionCall(const String &name, ArgList *args, Parser *p);
   ~FunctionCall()
      {
         FunctionDefinition *fd = FindFunction(m_name);
         ASSERT(fd && fd->m_UseCount > 0);
         fd->m_UseCount--;// decref the function
         delete m_args;
      }
   virtual Value Evaluate() const;
#ifdef DEBUG
   virtual String Debug(void) const
      { MOcheck(); return String("FunctionCall(") + m_name + String(")"); }
#endif    

   static bool AddFunction(const String &name, FunctionPointer fptr);

private:
   static FunctionDefinition *FindFunction(const String &name);

   static FunctionList *GetFunctionList(void)
      {
         if(ms_Functions == NULL)
            ms_Functions = new FunctionList;
         return ms_Functions;
      }
   
   FunctionPointer m_function;
   ArgList *m_args;
   String  m_name;

   static FunctionList *ms_Functions;
};

FunctionCall::FunctionCall(const String &name, ArgList *args,
                           Parser *p)
   : SyntaxNode(p)
{
   m_name = name;
   m_args = args;
   FunctionDefinition *fd = FindFunction(name);
   m_function = fd->m_FunctionPtr;
   fd->m_UseCount++;
}
   

/* static */
FunctionList * FunctionCall::ms_Functions = NULL;

/* static */
FunctionDefinition *
FunctionCall::FindFunction(const String &name)
{
   FunctionList *fl = GetFunctionList();
   
   for(FunctionList::iterator i = fl->begin();
       i != fl->end();
       i++)
      if(name == (**i).m_Name)
         return *i;
   return NULL;
}

/* static */
bool
FunctionCall::AddFunction(const String &name, FunctionPointer fptr)
{
   FunctionList *fl = GetFunctionList();
   FunctionDefinition *fd = FindFunction(name);
   if(fd)
      return false; // already exists
   fd = new FunctionDefinition(name, fptr);
   fl->push_back(fd);
   return true;
}

Value
FunctionCall::Evaluate() const
{
   MOcheck();
   ASSERT(m_function);
   return (*m_function)(m_args, GetParser());
}


/** An expression consisting of more than one token. */
class Expression : public SyntaxNode
{
public:
   Expression(SyntaxNode *left,
              Operator   *op,
              SyntaxNode *right,
              Parser *p)
      : SyntaxNode(p)
      { m_Left = left; m_Op = op; m_Right = right; }
   virtual Value Evaluate(void) const
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

/* The following macros use Value() as an error type to indicate that
   one was trying to combine two non-matching types. */

#define IMPLEMENT_VALUE_OP(oper, string1, string2) \
Value operator oper(const Value &left, const Value &right) \
{ \
   if(left.GetType() == Type_Error || right.GetType() == Type_Error || \
      left.GetType() != right.GetType()) \
   return Value(); \
   else if(left.GetType() == Type_Number) \
      return Value(left.GetNumber() oper right.GetNumber()); \
   else if(left.GetType() == Type_String) \
      return Value(string1 oper string2); \
   else \
   { \
      ASSERT(0); \
      return Value(); \
   } \
} 

#ifdef DEBUG
#define IMPLEMENT_OP(name, oper) \
class Operator##name : public Operator \
{ \
public: \
   virtual Value Evaluate(Value left, Value right) const \
      { MOcheck(); return left oper right; } \
   virtual bool TestLeft(Value left) const \
      { \
         MOcheck(); \
         return (bool) ((left.GetType() == Type_Number) ? \
                       left.GetNumber() : left.GetString().Length())!= 0; } \
   virtual String Debug(void) const \
      { MOcheck(); return #oper; } \
}

#else
#define IMPLEMENT_OP(name, oper) \
class Operator##name : public Operator \
{ \
public: \
   virtual Value Evaluate(Value left, Value right) const \
      { MOcheck(); return left oper right; } \
   virtual bool TestLeft(Value left) const \
      { \
         MOcheck(); \
         return (bool) ((left.GetType() == Type_Number) ? \
                       left.GetNumber() : left.GetString().Length())!= 0; } \
} 
#endif

IMPLEMENT_VALUE_OP(||,left.GetString().Length(),right.GetString().Length());
IMPLEMENT_VALUE_OP(&&,left.GetString().Length(),right.GetString().Length());
IMPLEMENT_VALUE_OP(+, left.GetString(),right.GetString());
IMPLEMENT_VALUE_OP(-, left.GetString().Length(),right.GetString().Length());
IMPLEMENT_VALUE_OP(*, left.GetString().Length(),right.GetString().Length());
IMPLEMENT_VALUE_OP(/, left.GetString().Length(),right.GetString().Length());

IMPLEMENT_OP(Or,||);
IMPLEMENT_OP(And,&&);
IMPLEMENT_OP(Plus,+);
IMPLEMENT_OP(Minus,-);
IMPLEMENT_OP(Times,*);
IMPLEMENT_OP(Divide,/);


/* static */
Parser *
Parser::Create(const String &input, MInterface *i)
{
   return new ParserImpl(input, i);
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

   m_MInterface->Message(tmp,NULL,_("Parse error!"));
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
      if(Char() == '"')
         m_Position++;
      else
         Error(_("Unterminated string."));
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
   return ParseBlock();
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
   
   return new FunctionCall(functionName, args, this);
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
            return new Negation(sn, this);
      }
   }
   else if( t.GetType() == TT_String )
   {
      GetToken();
      return new StringConstant(t.GetString(), this);
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
         sn = new Number(t.GetNumber(), this);
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
   SyntaxNode * cond = ParseCondition();
   if(! cond)
   {
      Error(_("Expected Condition after conditional operator."));
      return;
   }
   condition->SetRight(cond);
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
      | + EXPRESSION
      | - EXPRESSION
      | EMPTY
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
      | * TERM
      | / TERM
      | EMPTY
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
      Expression *cond = new Expression(expr, NULL, NULL,this);
      ParseRestCondition(cond);
      return cond;
   }
   return expr;
}

Block *
ParserImpl::ParseBlock(void)
{
   MOcheck();
   /* STATEMENT :=
      | { STATEMENT RESTSTATEMENT }
      | CONDITION;
   */

   Token t = PeekToken();
   if(t.GetType() == TT_Char && t.GetChar() == '{')
   {
      GetToken();
      Block * stmt = ParseBlock();
      if(! stmt)
      {
         Error(_("Expected Block after '{'"));
         return NULL;
      }
      ParseRestBlock(stmt);
      t = GetToken();
      if(t.GetType() != TT_Char || t.GetChar() != '}')
      {
         Error(_("Expected Condition after conditional operator."));
         delete stmt;
         return NULL;
      }
   }
   // condition
   SyntaxNode *cond = ParseCondition();
   if(! cond)
      return NULL;
   t = PeekToken();
   if(t.GetType() != TT_Char || t.GetChar() != ';')
   {
      Error(_("Expected ';' at end of condition."));
      delete cond;
      return NULL;
   }
   GetToken();
   Block *block = new Block(this);
   block->AddNode(cond);
   return block;
}
      
void
ParserImpl::ParseRestBlock(Block *parent)
{
   MOcheck();
   /* RESTSTATEMENT :=
      | STATEMENT ; RESTSTATEMENT
      | EMPTY
   */

   Block *stmt = ParseBlock();
   if(! stmt)
      return; // empty statement
   // else:
   do
   {
      parent->AddNode(stmt);
      stmt = ParseBlock();
   }while(stmt);
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
      Expression *exp = new Expression(sn, NULL, NULL, this);
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
      Expression *exp = new Expression(factor, NULL, NULL, this);
      ParseRestTerm(exp);
      return exp;
   }
   return factor;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Built-in functions
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

extern "C"
{
   static Value echo_func(ArgList *args, Parser *p)
   {
      SyntaxNode *sn;
      Value v;
      ASSERT(args);
      String msg = "Echo_Function: ";
      for(size_t i = 0; i < args->Count(); i++)
      {
         sn = args->GetArg(i);
         v= sn->Evaluate();
         msg << v.ToString();
         if(i < args->Count()-1)
            msg << ',';
      }
      p->Output(msg);
      return 1;
   }
   
   static Value func_rFive(ArgList *args, Parser *p)
   {  return 5; }
};


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * FilterRuleImpl - the class representing a program
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int FilterRuleImpl::Apply(class Message *msg) const
{
   ASSERT(msg);
//   m_Parser->SetMessage(msg);
   Value rc = m_Program->Evaluate();
//   m_Parser->SetMessage(NULL);
   return 1; //FIXME
}

FilterRuleImpl::FilterRuleImpl(const String &filterrule,
                               MInterface *interface,
                               MModule_Filters *mod) 
{
   m_Parser = new ParserImpl(filterrule, interface);
   m_Program = m_Parser->Parse();
   ASSERT(mod);
   m_FilterModule = mod;
   // we cannot allow the module to disappear while we exist
   m_FilterModule->IncRef();
}

FilterRuleImpl::~FilterRuleImpl()
{
   if(m_Program) delete m_Program;
   m_Parser->DecRef();
   m_FilterModule->DecRef();
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * MModule_FiltersImpl - the module
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef DEBUG

/** A small test for the filter module. */
static
int FilterTest(MInterface *interface)
{

   String program = interface->GetMApplication()->
      GetProfile()->readEntry("FilterTest"," 5 + 4 * ( 3 * 4 ) + 5 * print(); ");
   
   FunctionCall::AddFunction("print", echo_func);
   FunctionCall::AddFunction("Five", func_rFive);

   Parser_obj p(program, interface);
   String msg;
   if(p)
   {
      SyntaxNode *sn = p->Parse();
      if(sn)
      {
         String str = sn->Debug();
         msg << str << '\n';
         Value v = sn->Evaluate();
         msg << '\n' << program + String(" = ")+v.ToString();
         delete sn;
      }
      else
      {
         msg << '\n' << "Parsing " << program.c_str() << " failed.";
      }
      interface->Message(msg);
   }
   return MMODULE_ERR_NONE;
}
#endif



/** Filtering Module class. */
class MModule_FiltersImpl : public MModule_Filters
{
   /// standard MModule functions
   MMODULE_DEFINE(MModule_FiltersImpl)

   /** Takes a string representation of a filterrule and compiles it
       into a class FilterRule object.
   */
   virtual class FilterRule * GetFilter(const String &filterrule) const;
protected:
   MModule_FiltersImpl(MInterface *interface)
      {
         m_Interface = interface;
#ifdef DEBUG
         FilterTest(m_Interface);
#endif
      }

   /// the MInterface
   MInterface *m_Interface;
};

MMODULE_IMPLEMENT(MModule_FiltersImpl,
                  "Filters",
                  MMODULE_INTERFACE_FILTERS,
                  _("This plug-in provides a filtering language for Mahogany."),
                  "0.00")

FilterRule *
MModule_FiltersImpl::GetFilter(const String &filterrule) const
{
   return FilterRuleImpl::Create(filterrule, m_Interface,
                                 (MModule_FiltersImpl * /* non-const
                                                         */)this); 
}

/* static */
MModule *
MModule_FiltersImpl::Init(int vmajor, int vminor, int vrelease,
                      MInterface *interface, int *errorCode)
{
   if(! MMODULE_SAME_VERSION(vmajor, vminor, vrelease))
   {
      if(errorCode) *errorCode = MMODULE_ERR_INCOMPATIBLE_VERSIONS;
      return NULL;
   }
   return new MModule_FiltersImpl(interface);
}



