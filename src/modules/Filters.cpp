/*-*- c++ -*-********************************************************
 * Filters - Filtering code for Mahogany                            *
 *                                                                  *
 * (C) 1999-2000 by Karsten Ballüder (ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "Filters.h"
#endif

#ifndef TEST
#   include   "Mpch.h"
#else
//  For regression testing, we pull the configuration options
//  and tweak them to suit our nefarious purposes...
#   include "Mconfig.h"
#   undef USE_PCH		// never use precompiled headers
#   undef USE_MODULES_STATIC	// don't pull in static linking crud
//  turn on some debugging options in case they weren't already
#   undef DEBUG		// in case it's already set
#   define DEBUG 1
#   undef DEBUG_filters	// in case it's already set
#   define DEBUG_filters 1
#endif

#ifndef USE_PCH
#   include   "Mcommon.h"
#   include   "Message.h"
#   include   "MailFolder.h"
#endif

#include "MModule.h"
#include "MInterface.h"
#include "MPython.h"

#include <stdlib.h>
#include <ctype.h>
#include <time.h>  // mktime()

#include "modules/Filters.h"

#include <wx/regex.h>   // wxRegEx::Flags

// for the func_print() function:
#include "gui/wxMessageView.h"

// forward declare all of our classes
class ArgList;
class Expression;
class Filter;
class FunctionCall;
class FunctionDefinition;
class IfElse;
class Negation;
class Negative;
class Number;
class Operator;
class Parser;
class Statement;
class StringConstant;
class SyntaxNode;
class Token;
class Value;

/// Type for functions to be called.
extern "C" {
   typedef Value (* FunctionPointer)(ArgList *args, Parser *p);
};

/** Parsed representation of a filtering rule to be applied to a
    message.
*/
class FilterRuleImpl : public FilterRule
{
public:
   virtual int Apply(MailFolder *mf, UIdType uid,
                     bool *changeflag) const;
   virtual int Apply(MailFolder *folder, bool NewOnly,
                     bool *changeflag) const;
   virtual int Apply(MailFolder *folder,
                     UIdArray msgs,
                     bool ignoreDeleted,
                     bool *changeflag) const;
   static FilterRule * Create(const String &filterrule,
                              MInterface *minterface, MModule_Filters *mod)
      { return new FilterRuleImpl(filterrule, minterface, mod); }
#ifdef DEBUG
   void Debug(void);
#endif

protected:
   FilterRuleImpl(const String &filterrule,
                  MInterface *minterface,
                  MModule_Filters *fmodule);
   ~FilterRuleImpl();
   /// common code for the two Apply() functions
   int ApplyCommonCode(MailFolder *folder,
                       UIdArray *msgs,
                       bool newOnly,
                       bool ignoreDeleted,
                       bool *changeflag) const;
private:
   Parser     *m_Parser;
   SyntaxNode *m_Program;
   MModule_Filters *m_FilterModule;
};



///------------------------------
/// Own functionality:
///------------------------------

class Token
{
public:
   /// Type of tokens
   enum TokenType
   {
      TT_Invalid = -1,
      TT_Char,
      TT_Operator,
      TT_String,
      TT_Number,
      TT_Identifier,
      TT_EOF
   };

   /// Tokens are either an ASCII character or a value greater > 255.
   /// the known operators
   enum OperatorType
   {
      // the values of the constants shouldn't be changed as parsing code in
      // GetToken() relies on them being in this order
      Operator_Or,
      Operator_Plus,
      Operator_Minus,
      Operator_Times,
      Operator_Divide,
      Operator_Mod,
      Operator_And,
      Operator_Less,
      Operator_Leq,
      Operator_Greater,
      Operator_Geq,
      Operator_Equal,
      Operator_Neq,
   };

   Token() { m_type = TT_Invalid; }
   inline TokenType GetType(void) const
      { return m_type; }
   inline void SetChar(char c)
      { m_type = TT_Char; m_number = (unsigned)c; }
   void SetOperator(OperatorType oper)
      { m_type = TT_Operator; m_number = oper; }
   inline void SetString(String const &s)
      { m_type = TT_String; m_string = s; }
   inline void SetNumber(long n)
      { m_type = TT_Number; m_number = n; }
   inline void SetIdentifier(String const &s)
      { m_type = TT_Identifier; m_string = s; }
   inline void SetEOF(void)
      { m_type = TT_EOF; }

   inline int IsChar(char c) const
      { return m_type == TT_Char && (unsigned char)m_number == (unsigned)c; }
   inline int IsOperator(OperatorType t) const
      { return (m_type == TT_Operator) && ((OperatorType)m_number) == t; }
   inline int IsString(void) const
      { return m_type == TT_String; }
   inline int IsNumber(void) const
      { return m_type == TT_Number; }
   inline int IsIdentifier(const char *s) const
      { return m_type == TT_Identifier && m_string == s; }
   inline int IsEOF(void)
      { return m_type == TT_EOF; }

   inline char GetChar(void) const
      { ASSERT(m_type == TT_Char); return m_number; }
   OperatorType GetOperator() const
      { ASSERT(m_type == TT_Operator); return (OperatorType)m_number; }
   inline const String & GetString(void) const
      { ASSERT(m_type == TT_String); return m_string; }
   inline long GetNumber(void) const
      { ASSERT(m_type == TT_Number); return m_number; }
   inline const String & GetIdentifier(void) const
      { ASSERT(m_type == TT_Identifier); return m_string; }
private:
   TokenType m_type;
   long      m_number;
   String    m_string;
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
         MOcheck();
         if(m_Type == Type_String)
            return m_String;
         String str; str.Printf("%ld", m_Num);
         return str;
      }
   long ToNumber(void) const
      {
         MOcheck();
         if(m_Type == Type_String)
            return m_String.Length();
         return GetNumber();
      }
private:
   Type m_Type;
   // Can't use union here because m_String needs constructor.
   long   m_Num;
   String m_String;
   MOBJECT_NAME(Value)
};

/** A Parser class.
 */
class Parser : public MObjectRC
{
public:
   virtual SyntaxNode * Parse(void) = 0;
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
   /// Print out a message in a message box.
   virtual void Output(const String &msg) = 0;
   /// Print out a message.
   virtual void Log(const String &imsg, int level = M_LOG_DEFAULT) = 0;
   /// check if a function is already defined
   virtual FunctionDefinition *FindFunction(const String &name) = 0;
   /// Defines a function and overrides a given one
   virtual FunctionDefinition *DefineFunction(const String &name, FunctionPointer fptr) = 0;


   /**@name for runtime information */
   //@{
   /// Set the message and folder to operate on:
   virtual void SetMessage(MailFolder *folder,
                           UIdType uid = UID_ILLEGAL) = 0;
   virtual bool GetChanged(void) const = 0;
   /// Obtain the mailfolder to operate on:
   virtual MailFolder * GetFolder(void) = 0;
   /// Obtain the message UId to operate on:
   virtual UIdType GetMessageUId(void) = 0;
   /// Obtain the message itself:
   virtual Message * GetMessage(void) = 0;
   /// tell parser that msg or folder got changed
   virtual void SetChanged(void) = 0;
   //@}

   /// virtual destructor
   virtual ~Parser(void) {}
   /** Constructor function, returns a valid Parser object.
       @param input the input string holding the text to parse
   */
   static Parser * Create(const String &input, MInterface *minterface);

   virtual MInterface * GetInterface(void) = 0;
   MOBJECT_NAME(Parser)
};


// ----------------------------------------------------------------------------
// A smart reference to Parser - an easy way to avoid memory leaks
// ----------------------------------------------------------------------------

class Parser_obj
{
public:
   // ctor & dtor
   Parser_obj(const String& program, MInterface *minterface)
      { m_Parser = Parser::Create(program, minterface); }
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

/* Here is the grammar of the filter language:

   PROGRAM :=
     | BLOCK [ BLOCK ]

   BLOCK :=
     | CONDITION ;
     | { BLOCK RESTBLOCK}

   RESTBLOCK :=
     | BLOCK ; RESTBLOCK
     | EMPTY

   CONDITION :=
     | EXPRESSION RELOP EXPRESSION
     | EXPRESSION

   EXPRESSION :=
     | EXPRESSION ADDOP TERM
     | TERM

   TERM :=
     | TERM MULTOP TERM
     | SIGNOP TERM
     | FACTOR

   FACTOR :=
     | NUMBER
     | IDENTIFIER ( EXPRESSION )
     | ( EXPRESSION )
     | NEGOP FACTOR

  RELOP := < | > | <= | >= | == | !=
  ADDOP := + | - | |
  MULTOP := * | / | &
  SIGNOP := + | -
  NEGOP := !

  With the following rewritten rules being used:

  CONDITION :=
    | EXPRESSION RESTCONDITION

  RESTCONDITION :=
    | RELOP EXPRESSION
    | EMPTY

  EXPRESSION :=
    | EXPRESSION RESTEXPRESSION

  RESTEXPRESSION :=
    | ADDOP TERM
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
   SyntaxNode * Parse(void);
   SyntaxNode * ParseProgram(void);
   SyntaxNode * ParseFilters(void);
   SyntaxNode * ParseIfElse(void);
   SyntaxNode * ParseBlock(void);
   SyntaxNode * ParseStmts(void);
   SyntaxNode * ParseExpression(void);
   SyntaxNode * ParseCondition(void);
   SyntaxNode * ParseQueryOp(void);
   SyntaxNode * ParseOrs(void);
   SyntaxNode * ParseIffs(void);
   SyntaxNode * ParseAnds(void);
   SyntaxNode * ParseBOrs(void);
   SyntaxNode * ParseXors(void);
   SyntaxNode * ParseBAnds(void);
   SyntaxNode * ParseRelational(void);
   SyntaxNode * ParseTerm(void);
   SyntaxNode * ParseFactor(void);
   SyntaxNode * ParseUnary(void);
   SyntaxNode * ParseFunctionCall(Token id);
   virtual size_t GetPos(void) const { return m_Position; }
   virtual Token GetToken(bool remove = true);
   virtual Token PeekToken(void) { return token; }
   virtual void Rewind(size_t pos = 0);
   inline void NextToken(void) { Rewind(m_Peek); }
   virtual void Error(const String &error);
   virtual void Output(const String &msg)
      {
         m_MInterface->MessageDialog(msg,NULL,_("Filters output"));
      }
   virtual void Log(const String &imsg, int level)
      {
         String msg = _("Filters: ");
         msg << imsg;
         m_MInterface->Log(level, msg);
      }
   /// check if a function is already defined
   virtual FunctionDefinition *FindFunction(const String &name);
   /// Defines a function and overrides a given one
   virtual FunctionDefinition *DefineFunction(const String &name, FunctionPointer fptr);

   /**@name for runtime information */
   //@{
   /// Set the message and folder to operate on:
   virtual void SetMessage(MailFolder *folder = NULL,
                           UIdType uid = UID_ILLEGAL)
      {
         SafeDecRef(m_MailFolder);
         m_MailFolder = folder;
         SafeIncRef(m_MailFolder);
         m_MessageUId = uid;
         m_MfWasChanged = FALSE;
      }
   /// Has filter rule caused a change to the folder/message?
   virtual bool GetChanged(void) const
      {
         ASSERT(m_MailFolder);
         return m_MfWasChanged;
      }
   /// Obtain the mailfolder to operate on:
   virtual MailFolder * GetFolder(void) { SafeIncRef(m_MailFolder); return m_MailFolder; }
   /// Obtain the message UId to operate on:
   virtual UIdType GetMessageUId(void) { return m_MessageUId; }
   /// Obtain the message itself:
   virtual Message * GetMessage(void)
   {
      return m_MailFolder ?
         m_MailFolder->GetMessage(m_MessageUId) :
         NULL ;
   }
   //@}
   virtual MInterface * GetInterface(void) { return m_MInterface; }
   /// tell parser that msg or folder got changed
   virtual void SetChanged(void) { m_MfWasChanged = TRUE; }
protected:
   ParserImpl(const String &input, MInterface *minterface);
   ~ParserImpl();
   inline void EatWhiteSpace(void)
      { while(isspace(m_Input[m_Position])) m_Position++; }
   inline const char Char(void) const
      { return m_Input[m_Position]; }
   inline const char CharInc(void)
      { return m_Input[m_Position++]; }
   inline String CharLeft(void)
      { return m_Input.Left(m_Position); }
   inline String CharMid(void)
      { return m_Input.Mid(m_Position); }

   void AddBuiltinFunctions(void);
   friend Parser;
private:
   String m_Input;
   Token token, peek;	// current and next tokens
   size_t m_Position;	// seek offset of current token
   size_t m_Peek;	// seek offset of next token
   MInterface *m_MInterface;
   FunctionDefinition *m_FunctionList;

   UIdType m_MessageUId;
   MailFolder *m_MailFolder;
   bool m_MfWasChanged;

   GCC_DTOR_WARN_OFF
};

// ----------------------------------------------------------------------------
// ABC for logical operators.
// ----------------------------------------------------------------------------

/** This class represents logical operators. */
class Operator : public MObject
{
public:
   virtual Value Evaluate(SyntaxNode *left, SyntaxNode * right) const = 0;
#ifdef DEBUG
   virtual String Debug(void) const = 0;
#endif
   MOBJECT_NAME(Operator)
};

// ----------------------------------------------------------------------------
// ABC for syntax tree nodes.
// ----------------------------------------------------------------------------
/** This class is used to build a syntax tree.
 */
class SyntaxNode : public MObject
{
public:
   SyntaxNode(void) {}
   virtual ~SyntaxNode(void) {}
   virtual Value Evaluate(void) const = 0;
   virtual String ToString(void) const
      { return Evaluate().ToString(); }
#ifdef DEBUG
   virtual String Debug(void) const = 0;
#endif
private:
   MOBJECT_NAME(SyntaxNode)
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
   MOBJECT_NAME(ArgList)
};

class Filter : public SyntaxNode
{
public:
   Filter(SyntaxNode *r, SyntaxNode *n)
      {
	 ASSERT(r != NULL); ASSERT(n != NULL);
         m_Rule = r; m_Next = n;
      }
   ~Filter(void) { delete m_Rule; delete m_Next; }
   virtual Value Evaluate() const
      {
         MOcheck();
         (void) m_Rule->Evaluate();;
	 // tail recursion, so no add'l stack frame
	 return m_Next->Evaluate();
      }
#ifdef DEBUG
   virtual String Debug(void) const
      {
         MOcheck();
         String s = m_Rule->Debug();
	 s << m_Next->Debug();
         return s;
      }
#endif
private:
   SyntaxNode * m_Rule, * m_Next;
   MOBJECT_NAME(Filter)
};

class Statement : public SyntaxNode
{
public:
   Statement(SyntaxNode *r, SyntaxNode *n)
      {
	 ASSERT(r != NULL); ASSERT(n != NULL);
         m_Rule = r; m_Next = n;
      }
   ~Statement(void) { delete m_Rule; delete m_Next; }
   virtual Value Evaluate() const
      {
         MOcheck();
         (void) m_Rule->Evaluate();;
	 // tail recursion, so no add'l stack frame
	 return m_Next->Evaluate();
      }
   void AddNext(SyntaxNode *node) { m_Next = node; }
#ifdef DEBUG
   virtual String Debug(void) const
      {
         MOcheck();
         String s = m_Rule->Debug();
	 s << ';' << m_Next->Debug();
         return s;
      }
#endif
private:
   SyntaxNode * m_Rule, * m_Next;
   MOBJECT_NAME(Statement)
};

class Number : public SyntaxNode
{
public:
   Number(long v) { m_value = v; }
   virtual Value Evaluate() const { MOcheck(); return m_value; }
#ifdef DEBUG
   virtual String Debug(void) const
      { MOcheck(); String s; s.Printf("%ld",m_value); return s; }
#endif
private:
   long m_value;
   MOBJECT_NAME(Number)
};

class StringConstant : public SyntaxNode
{
public:
   StringConstant(String v) { m_String = v; }
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
   MOBJECT_NAME(StringConstant)
};

class Negation : public SyntaxNode
{
public:
   Negation(SyntaxNode *sn) { m_Sn = sn; }
   ~Negation() { MOcheck(); delete m_Sn; }
   virtual Value Evaluate() const
      {
         MOcheck();
         Value v = m_Sn->Evaluate();
         return ! (v.GetType() == Type_Number ?
            v.GetNumber() : v.GetString().Length());
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
   MOBJECT_NAME(Negation)
};

class Negative : public SyntaxNode
{
public:
   Negative(SyntaxNode *sn) { m_Sn = sn; }
   ~Negative() { MOcheck(); delete m_Sn; }
   virtual Value Evaluate() const
      {
         MOcheck();
         Value v = m_Sn->Evaluate();
         return -(v.GetType() == Type_Number ?
            v.GetNumber() : v.GetString().Length());
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
   MOBJECT_NAME(Negative)
};

/** Functioncall handling */

/** This little class contains the definition of a function,
    i.e. links its name to a bit of C code and maintains a use
    count. */

class FunctionDefinition : public MObject
{
public:
   FunctionDefinition(const String &name, FunctionPointer fptr,
                      FunctionDefinition * next)
      {
         m_Name = name; m_FunctionPtr = fptr; m_Next = next; m_UseCount = 1;
         ASSERT(m_FunctionPtr);
      }
   ~FunctionDefinition()
      {
         MOcheck();
         ASSERT(m_UseCount == 0);
	 if (m_Next != NULL)
	    m_Next->DecRef();
      }

   FunctionDefinition * FindFunction(const String &name)
      {
         if(name == m_Name) {
	    IncRef();
	    return this;
	 }
         if(m_Next == NULL)
	    return NULL;
	 // tail recursion
         return m_Next->FindFunction(name);
      }

   bool DecRef(void)
      {
         MOcheck();
         if(--m_UseCount == 0)
         {
            delete this;
            return true;
         }
         else
            return false;
      }
   void IncRef(void)
      {
         MOcheck();
         m_UseCount++;
      }

   inline const String &GetName(void) const { return m_Name; }
   inline FunctionPointer GetFPtr(void) const { return m_FunctionPtr; }
private:
   int             m_UseCount;
   String          m_Name;
   FunctionPointer m_FunctionPtr;
   FunctionDefinition * m_Next;
   MOBJECT_NAME(FunctionDefinition)
};

/** This class represents a function call. */
class FunctionCall : public SyntaxNode
{
public:
   // FIXME: name not needed, available from fd
   FunctionCall(const String &name, ArgList *args,
                FunctionDefinition *fd, Parser *p);
   ~FunctionCall()
      {
         MOcheck();
         if(m_fd)
            m_fd->DecRef();
         ASSERT(m_args);
         delete m_args;
      }
   virtual Value Evaluate() const;
#ifdef DEBUG
   virtual String Debug(void) const
      { MOcheck(); return String("FunctionCall(") + m_name + String(")"); }
#endif

private:
   FunctionPointer m_function;
   FunctionDefinition *m_fd;
   ArgList *m_args;
   String  m_name;
   Parser *m_Parser;
   MOBJECT_NAME(FunctionCall)
};

FunctionCall::FunctionCall(const String &name, ArgList *args,
                           FunctionDefinition *fd,
                           Parser *p)
{
   m_name = name;
   m_args = args;
   m_fd = fd;
   m_fd->IncRef();
   m_function = m_fd->GetFPtr();
   m_Parser = p;
}


Value
FunctionCall::Evaluate() const
{
   MOcheck();
   ASSERT(m_function);
   return (*m_function)(m_args, m_Parser);
}

class QueryOp : public SyntaxNode
{
public:
   QueryOp(SyntaxNode *cond, SyntaxNode *left, SyntaxNode *right)
      {
	 ASSERT(cond != NULL); ASSERT(left != NULL); ASSERT(right != NULL);
         m_Cond = cond; m_Left = left; m_Right = right;
      }
   ~QueryOp(void)
      {
         MOcheck();
         delete m_Cond;
         delete m_Left;
         delete m_Right;
      }
   virtual Value Evaluate(void) const
      {
         MOcheck();
	 return m_Cond->Evaluate().ToNumber()
	      ? m_Left->Evaluate()
	      : m_Right->Evaluate();
      }
#ifdef DEBUG
   virtual String Debug(void) const
      {
         MOcheck();
         String s = "(";
         s << m_Left->Debug()
	   << '?'
	   << m_Left->Debug()
	   << ':'
	   << m_Right->Debug()
           << ')';
         return s;
      }
#endif

private:
   SyntaxNode *m_Cond, *m_Left, *m_Right;
   MOBJECT_NAME(QueryOp)
};

/** An expression consisting of more than one token. */
class Expression : public SyntaxNode
{
public:
   Expression(SyntaxNode *left, Operator *op, SyntaxNode *right)
      {
         ASSERT(left != NULL); ASSERT(op != NULL); ASSERT(right != NULL);
	 m_Left = left; m_Op = op; m_Right = right;
      }
   ~Expression()
      {
         MOcheck();
         delete m_Left;
         delete m_Right;
      }
   virtual Value Evaluate(void) const
      {
         MOcheck();
         return m_Op->Evaluate(m_Left, m_Right);
      }
#ifdef DEBUG
   virtual String Debug(void) const
      {
         MOcheck();
         String s = "(";
         s << m_Left->Debug() << m_Op->Debug() << m_Right->Debug() << ')';
         return s;
      }
#endif
private:
   SyntaxNode *m_Left, *m_Right;
   Operator *m_Op;
   MOBJECT_NAME(Expression)
};

/* The following macros use Value() as an error type to indicate that
   one was trying to combine two non-matching types. */

#define IMPLEMENT_VALUE_OP(oper, string) \
Value operator oper(const Value &left, const Value &right) \
{ \
   if(left.GetType() == Type_Error || right.GetType() == Type_Error || \
      left.GetType() != right.GetType()) \
   return Value(); \
   else if(left.GetType() == Type_Number) \
      return Value(left.GetNumber() oper right.GetNumber()); \
   else if(left.GetType() == Type_String) \
      return Value(left.GetString()string oper right.GetString()string); \
   else \
   { \
      ASSERT(0); \
      return Value(); \
   } \
}

#ifdef DEBUG
#define IMPLEMENT_OP(name, oper, string) \
IMPLEMENT_VALUE_OP(oper, string); \
static class Operator##name : public Operator \
{ \
public: \
   virtual Value Evaluate(SyntaxNode *left, SyntaxNode *right) const \
      { \
         MOcheck(); ASSERT(left); ASSERT(right); \
         return left->Evaluate() oper right->Evaluate(); \
      } \
   virtual String Debug(void) const \
      { MOcheck(); return #oper; } \
} Oper_##name

#else	// not DEBUGing

#define IMPLEMENT_OP(name, oper, string) \
IMPLEMENT_VALUE_OP(oper, string); \
static class Operator##name : public Operator \
{ \
public: \
   virtual Value Evaluate(SyntaxNode *left, SyntaxNode *right) const \
      { MOcheck(); return left->Evaluate() oper right->Evaluate(); } \
} Oper_##name
#endif

IMPLEMENT_OP(Plus,+,);
IMPLEMENT_OP(Minus,-,.Length());
IMPLEMENT_OP(Times,*,.Length());
IMPLEMENT_OP(Divide,/,.Length());
IMPLEMENT_OP(Mod,%,.Length());

IMPLEMENT_OP(Less, <,);
IMPLEMENT_OP(Leq, <=,);
IMPLEMENT_OP(Greater, >,);
IMPLEMENT_OP(Geq, >=,);
IMPLEMENT_OP(Equal, ==,);
IMPLEMENT_OP(Neq, !=,);

IMPLEMENT_VALUE_OP(&&,.Length());
static class OperatorAnd : public Operator
{
public:
   virtual Value Evaluate(SyntaxNode *left, SyntaxNode *right) const
      {
         MOcheck(); ASSERT(left); ASSERT(right);
         Value lv = left->Evaluate();
         if(lv.ToNumber())
            return lv && right->Evaluate();
         else
            return lv;
      }
#ifdef DEBUG
   virtual String Debug(void) const
      { MOcheck(); return "And"; }
#endif
} Oper_And;

IMPLEMENT_VALUE_OP(||,.Length());
static class OperatorOr : public Operator
{
public:
   virtual Value Evaluate(SyntaxNode *left, SyntaxNode *right) const
      {
         MOcheck(); ASSERT(left); ASSERT(right);
         Value lv = left->Evaluate();
         if(! lv.ToNumber())
            return lv || right->Evaluate();
         else
            return lv;
      }
#ifdef DEBUG
   virtual String Debug(void) const
      { MOcheck(); return "Or"; }
#endif
} Oper_Or;

/** An if-else statement. */
class IfElse : public SyntaxNode
{
public:
   IfElse(SyntaxNode *condition,
          SyntaxNode *ifBlock,
          SyntaxNode *elseBlock)
      {
         m_Condition = condition;
         m_IfBlock = ifBlock;
         m_ElseBlock = elseBlock;
      }
   ~IfElse(void)
      {
         MOcheck();
         delete m_Condition;
         delete m_IfBlock;
         delete m_ElseBlock;
      }
   virtual Value Evaluate(void) const
      {
         MOcheck();
         ASSERT(m_Condition != NULL);
         ASSERT(m_IfBlock != NULL);
         Value rc = m_Condition->Evaluate();
         if(rc.ToNumber())
            return m_IfBlock->Evaluate();
         else if(m_ElseBlock)
            return m_ElseBlock->Evaluate();
         else
            return rc;
      }
#ifdef DEBUG
   virtual String Debug(void) const
      {
         MOcheck();
         String s = "if(";
         s << m_Condition->Debug() << "){";
         if(m_IfBlock)
            s << m_IfBlock->Debug();
         s << '}';
         if(m_ElseBlock)
         {
            s << "else{"
              << m_ElseBlock->Debug()
              << '}';
         }
         return s;
      }
#endif
private:
   SyntaxNode *m_Condition, *m_IfBlock, *m_ElseBlock;
   MOBJECT_NAME(IfElse)
};


static void PreProcessInput(String *input)
{
   bool modified = false;
   String output;
   while(input->Length() && input->c_str()[0] == '@')
   {
      const char *cptr = input->c_str()+1;
      String filename;
      while(*cptr && *cptr != '\n' && *cptr != '\r')
         filename += *cptr++;
      while(*cptr && (*cptr == '\r' || *cptr == '\n'))
         cptr++;
      *input = cptr;
      FILE *fp = fopen(filename,"rt");
      if(fp)
      {
         fseek(fp,0, SEEK_END);
         long len = ftell(fp);
         fseek(fp,0, SEEK_SET);
         if(len > 0)
         {
            char *cp = new char [ len + 1];
            if(fread(cp, 1, len, fp) > 0)
            {
               cp[len] = '\0';
               output << cp;
               modified = true;
            }
            delete [] cp;
         }
         fclose(fp);
      }
   }
   if(modified)
   {
      *input = output;
      if(input->Length())
         PreProcessInput(input);
   }
}

/* static */
Parser *
Parser::Create(const String &input, MInterface *i)
{
   /* Here we handle the one special occasion of input being
      @filename
      in which case we replace input with the contents of that file.
   */
   String program = input;
   PreProcessInput(&program);
   return new ParserImpl(program, i);
}


ParserImpl::ParserImpl(const String &input, MInterface *minterface)
{
   m_Input = input;
   Rewind();
   m_MInterface = minterface;
   m_FunctionList = NULL;
   m_MailFolder = NULL;
   m_MessageUId = UID_ILLEGAL;

   AddBuiltinFunctions();
}

ParserImpl::~ParserImpl()
{
   /* Make sure all function definitions are gone. They are
      refcounted, so all we need to do is decrement the
      count for the pointer we are destroying */
   if (m_FunctionList != NULL)
      m_FunctionList->DecRef();

   SafeDecRef(m_MailFolder);
}


FunctionDefinition *
ParserImpl::DefineFunction(const String &name, FunctionPointer fptr)
{
   FunctionDefinition *fd = FindFunction(name);
   if(fd != NULL)
   {
      fd->DecRef();
      return NULL; // already exists, not overridden
   }
   return m_FunctionList = new FunctionDefinition(name, fptr, m_FunctionList);
}

FunctionDefinition *
ParserImpl::FindFunction(const String &name)
{
   if (m_FunctionList == NULL)
      return NULL;
   return m_FunctionList->FindFunction(name);
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

   // FIXME: this should be wxLogError() call as otherwise we get several
   //        message boxes for each error instead of only one combining all
   //        messages!
   m_MInterface->MessageDialog(tmp,NULL,_("Parse error!"));
}

/** Reads the next token from the string and removes it from it.
    @param str   string to parse
*/
Token
ParserImpl::GetToken(bool remove)
{
   MOcheck();
   if (remove) {
   	Token mytok = token;
   	NextToken();
	return mytok;
   }
   return token;
}

/** Reads the next token from the string starting at the given location.
    @param loc   location within string
*/
void
ParserImpl::Rewind(size_t pos)
{
   MOcheck();

   m_Position = pos;
   EatWhiteSpace();
   pos = m_Position;

   String tokstr;
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
   else   /// should be a special character
   {
      int c = CharInc();
      switch( c )
      {
      case '+':
         token.SetOperator(Token::Operator_Plus);
	 break;
      case '-':
         token.SetOperator(Token::Operator_Minus);
	 break;
      case '*':
         token.SetOperator(Token::Operator_Times);
	 break;
      case '/':
         token.SetOperator(Token::Operator_Divide);
	 break;
      case '%':
         token.SetOperator(Token::Operator_Mod);
	 break;
      case '&':
	 if ( Char() == c )
	    (void)CharInc();
         token.SetOperator(Token::Operator_And);
	 break;
      case '|':
	 if ( Char() == c )
	    (void)CharInc();
         token.SetOperator(Token::Operator_Or);
	 break;
      case '<':
	 if ( Char() == '=' )
	 {
	    (void)CharInc();
            token.SetOperator(Token::Operator_Leq);
	 }
	 else if ( Char() == '>' )
	 {
	    (void)CharInc();
            token.SetOperator(Token::Operator_Neq);
	 }
	 else
            token.SetOperator(Token::Operator_Less);
	 break;
      case '>':
	 if ( Char() == '=' )
	 {
	    (void)CharInc();
            token.SetOperator(Token::Operator_Geq);
	 }
	 else
            token.SetOperator(Token::Operator_Greater);
	 break;
      case '=':
	 if ( Char() == c )
	    (void)CharInc();
         token.SetOperator(Token::Operator_Equal);
	 break;
      case '!':
	 if ( Char() == '=' )
	 {
	    (void)CharInc();
            token.SetOperator(Token::Operator_Neq);
	    break;
	 }
	 // fall through to...
      default:
         // All other cases: return the character:
	 token.SetChar(c);
      }
   }

   m_Peek = m_Position;
   m_Position = pos;
}

SyntaxNode *
ParserImpl::Parse(void)
{
   MOcheck();
   return ParseProgram();
}

SyntaxNode *
ParserImpl::ParseProgram(void)
{
   MOcheck();
   if(token.IsEOF())
   {
      Error(_("No filter program found"));
      return NULL;
   }
   SyntaxNode * pgm = ParseFilters();
   if(pgm == NULL)
      Error(_("Parse error, cannot find valid program."));
   return pgm;
}

SyntaxNode *
ParserImpl::ParseFilters(void)
{
   MOcheck();
   SyntaxNode * filter = NULL;
   if(token.IsIdentifier("if"))
   {
      filter = ParseIfElse();
   }
   else if(token.IsChar('{'))
   {
      filter = ParseBlock();
   }
   if (filter == NULL)
      return NULL;
   if(token.IsEOF())
      return filter;
   SyntaxNode * next = ParseFilters();
   if (next == NULL) {
      delete filter;
      return NULL;
   }
   return new Filter(filter, next);
}

SyntaxNode *
ParserImpl::ParseIfElse(void)
{
   MOcheck();

   ASSERT(token.IsIdentifier("if"));
   NextToken(); // swallow "if"
   if(!token.IsChar('('))
   {
      Error(_("expected '(' after 'if'."));
      return NULL;
   }
   NextToken(); // swallow '('

   SyntaxNode *condition = ParseCondition();
   if(! condition)
      return NULL;

   if(!token.IsChar(')'))
   {
      Error(_("expected ')' after condition in if statement."));
      delete condition;
      return NULL;
   }
   NextToken(); // swallow ')'

   SyntaxNode *ifBlock = ParseBlock();
   if(! ifBlock) {
      delete condition;
      return NULL;
   }

   SyntaxNode *elseBlock = NULL;
   if(token.IsIdentifier("else"))
   {
      // we must parse the else branch, too:
      NextToken(); // swallow the "else"
      if(token.IsIdentifier("if"))
         elseBlock = ParseIfElse();
      else
         elseBlock = ParseBlock();
      if(! elseBlock)
      {
         delete condition;
         delete ifBlock;
         return NULL;
      }
   }
   // if we reach here, everything was parsed OK:
   return new IfElse(condition, ifBlock, elseBlock);
}

SyntaxNode *
ParserImpl::ParseBlock(void)
{
   MOcheck();
   // Normal block:
   if(!token.IsChar('{'))
   {
      Error(_("Expected '{' at start of block."));
      return NULL;
   }
   NextToken(); // swallow '{'
   SyntaxNode * stmt;
   if(token.IsChar('{'))
      stmt = ParseBlock(); // it can generate {{{ ... }}}
   else
      stmt = ParseStmts();
   if(stmt == NULL)
   {
      Error(_("Expected statement block after '{'"));
      return NULL;
   }
   if(!token.IsChar('}'))
   {
      Error(_("Expected '}' after block."));
      delete stmt;
      return NULL;
   }
   NextToken(); // swallow '}'
   return stmt;
}

SyntaxNode *
ParserImpl::ParseStmts(void)
{
   MOcheck();
   SyntaxNode * stmt;
   if(token.IsIdentifier("if"))
   {
      stmt = ParseIfElse();
      if(stmt == NULL)
         return NULL;
   }
   else if(token.GetType() == Token::TT_Identifier)
   {
      Token id = GetToken();
      stmt = ParseFunctionCall(id);
      if(stmt == NULL)
         return NULL;
      if(!token.IsChar(';'))
      {
         Error(_("Expected ';' at end of statement."));
         delete stmt;
         return NULL;
      }
      NextToken();
   }
   else
   {
      Error(_("Expected a statement."));
      return NULL;
   }
   if(token.IsChar('}'))
      return stmt;
   SyntaxNode * next = ParseStmts();
   if(next == NULL) {
      delete stmt;
      return NULL;
   }
   return new Statement(stmt, next);
}

SyntaxNode *
ParserImpl::ParseExpression(void)
{
   MOcheck();
   return ParseQueryOp();
}

SyntaxNode *
ParserImpl::ParseCondition(void)
{
   MOcheck();
   SyntaxNode *sn = ParseQueryOp();
   if (sn != NULL)
      return sn;
   Error(_("Invalid conditional expression"));
   return NULL;
}

SyntaxNode *
ParserImpl::ParseQueryOp(void)
{
   MOcheck();
   SyntaxNode *sn = ParseOrs();
   if(sn == NULL)
      return NULL;
   if(!token.IsChar('?'))
   	return sn;
   NextToken();
   SyntaxNode *left = ParseExpression();
   if(left == NULL) {
      Error(_("Expected expression after '?'"));
      delete sn;
      return NULL;
   }
   if(!token.IsChar(':'))
   {
      Error(_("Expected ':' after '?' expression."));
      delete left; delete sn;
      return NULL;
   }
   NextToken();
   SyntaxNode *right = ParseExpression();
   if (right == NULL) {
      Error(_("Expected expression after ':'"));
      delete left; delete sn;
      return NULL;
   }
   return new QueryOp(sn, left, right);
}

// These little ditties will be needed a number of times, so they
// are macros to save some typing (and typos).
#define OPERATOR_VALUE(oper) \
   case Token::Operator_##oper: return &Oper_##oper

// This ditty implements the dreaded left-recursive grammar
//	name	:= part
//		| name op part
// That is, a string of left-associative operators at the same
// precedence level.  The operands are all of type `part'
#define LeftAssoc(name,opers,part,msg) \
SyntaxNode * \
ParserImpl::Parse##name(void) \
{ \
   MOcheck(); \
   SyntaxNode *expr = Parse##part(); \
   if (expr == NULL) \
      return NULL; \
   for (;;) \
   { \
      Operator *op = opers(token); \
      if (op == NULL) \
         break; \
      NextToken(); \
      SyntaxNode *exp = Parse##part(); \
      if (exp == NULL) { \
         delete expr; \
	 Error(msg); \
	 return NULL; \
      } \
      expr = new Expression(expr, op, exp); \
   } \
   return expr; \
}

static Operator *
OrOp(Token t)
{
   if (t.GetType() == Token::TT_Operator)
      switch (t.GetOperator())
      {
         OPERATOR_VALUE(Or);
         default: return NULL;
      }
   else if (t.IsIdentifier("or"))
      return &Oper_Or;
   return NULL;
}
LeftAssoc(Ors,OrOp,Iffs,_("Expected expression after OR operator"))

static Operator *
IffOp(Token t)
{
   if (t.GetType() == Token::TT_Operator)
      switch (t.GetOperator())
      {
         //OPERATOR_VALUE(Iff);
         default: return NULL;
      }
   //else if (t.IsIdentifier("iff"))
   //   return &Oper_Iff;
   return NULL;
}
LeftAssoc(Iffs,IffOp,Ands,_("Expected expression after IFF operator"))

static Operator *
AndOp(Token t)
{
   if (t.GetType() == Token::TT_Operator)
      switch (t.GetOperator())
      {
         OPERATOR_VALUE(And);
         default: return NULL;
      }
   else if (t.IsIdentifier("and"))
      return &Oper_And;
   return NULL;
}
LeftAssoc(Ands,AndOp,BOrs,_("Expected expression after AND operator"))

static Operator *
BOrOp(Token t)
{
   if (t.GetType() != Token::TT_Operator)
      return NULL;
   switch (t.GetOperator())
   {
      //OPERATOR_VALUE(BOr);
      default: return NULL;
   }
}
LeftAssoc(BOrs,BOrOp,Xors,_("Expected expression after bit OR operator"))

static Operator *
XorOp(Token t)
{
   if (t.GetType() == Token::TT_Operator)
      switch (t.GetOperator())
      {
         //OPERATOR_VALUE(Xor);
         default: return NULL;
      }
   //else if (t.IsIdentifier("xor"))
   //   return &Oper_Xor;
   return NULL;
}
LeftAssoc(Xors,XorOp,BAnds,_("Expected expression after XOR operator"))

static Operator *
BAndOp(Token t)
{
   if (t.GetType() != Token::TT_Operator)
      return NULL;
   switch (t.GetOperator())
   {
      //OPERATOR_VALUE(BAnd);
      default: return NULL;
   }
}
LeftAssoc(BAnds,BAndOp,Relational,_("Expected expression after bit AND operator"))

static Operator *
RelOp(Token t)
{
   if (t.GetType() != Token::TT_Operator)
      return NULL;
   switch (t.GetOperator())
   {
      OPERATOR_VALUE(Less);
      OPERATOR_VALUE(Leq);
      OPERATOR_VALUE(Greater);
      OPERATOR_VALUE(Geq);
      OPERATOR_VALUE(Equal);
      OPERATOR_VALUE(Neq);
      default: return NULL;
   }
}
// Relationals are a special case; they don't associate.
SyntaxNode *
ParserImpl::ParseRelational(void)
{
   MOcheck();
   SyntaxNode *expr = ParseTerm();
   if (expr == NULL)
      return NULL;
   Operator *op = RelOp(token);
   if (op == NULL)
      return expr;
   NextToken();
   SyntaxNode *exp = ParseTerm();
   if (exp == NULL) {
      delete expr;
      Error(_("Expected expression after relational operator"));
      return NULL;
   }
   return new Expression(expr, op, exp);
}

static Operator *
AddOp(Token t)
{
   if (t.GetType() != Token::TT_Operator)
      return NULL;
   switch (t.GetOperator())
   {
      OPERATOR_VALUE(Plus);
      OPERATOR_VALUE(Minus);
      default: return NULL;
   }
}
LeftAssoc(Term,AddOp,Factor,_("Expected term after plus/minus operator"))

static Operator *
MulOp(Token t)
{
   if (t.GetType() != Token::TT_Operator)
      return NULL;
   switch (t.GetOperator())
   {
      OPERATOR_VALUE(Times);
      OPERATOR_VALUE(Divide);
      OPERATOR_VALUE(Mod);
      default: return NULL;
   }
}
LeftAssoc(Factor,MulOp,Unary,_("Expected factor after multiply/divide/modulus operator"))

SyntaxNode *
ParserImpl::ParseUnary(void)
{
   MOcheck();
   SyntaxNode *sn = NULL;
   if(token.GetType() == Token::TT_Char)
   {
      /* expression in parenthesis */
      if( token.GetChar() == '(')
      {
         NextToken();
         sn = ParseExpression();
         if(!token.IsChar(')'))
         {
            delete sn;
            Error(_("Expected ')' after expression."));
            return NULL;
         }
         NextToken();
      }
      else if(token.GetChar() == '!')
      {
         NextToken();
         sn = ParseUnary();
         if(sn == NULL)
         {
            Error(_("Expected unary after negation operator."));
            return NULL;
         }
         sn = new Negation(sn);
      }
   }
   else if( token.GetType() == Token::TT_Operator )
   {
      if(token.GetOperator() == Token::Operator_Plus)
      {
	 NextToken();
         return ParseUnary();
      }
      else if(token.GetOperator() == Token::Operator_Minus)
      { // might be a number
         NextToken();
         if(token.IsNumber())
         {
            sn = new Number(-token.GetNumber());
            NextToken();
         }
         else
	 {
	    sn = ParseUnary();
	    if (sn == NULL)
	       return NULL;
	    sn = new Negative(sn);
	 }
      }
   }
   else if( token.IsString() )
   {
      sn = new StringConstant(token.GetString());
      NextToken();
   }
   else if(token.GetType() == Token::TT_Identifier)
   {
      Token id = GetToken();
      if(token.GetType() == Token::TT_Char) {
	 // IDENTIFIER ( ARGLIST )  ==> function call
         if(token.GetChar() == '(')
            sn = ParseFunctionCall(id);
         // IDENTIFIER [ EXPRESSION ] ==> subscripted reference
         // IDENTIFIER ==> variable reference
      }
   }
   else if(token.IsNumber())
   {
      sn = new Number(token.GetNumber());
      NextToken();
   }
   // sn == NULL, illegal unary value
   if(sn == NULL)
      Error(_("Expected a number or a function call."));
   return sn;
}

SyntaxNode *
ParserImpl::ParseFunctionCall(Token id)
{
   MOcheck();
   ASSERT(id.GetType() == Token::TT_Identifier);
   
   // marshall arg list
   if(!token.IsChar('('))
   {
      String err;
      err.Printf(_("Functioncall expected '(' after '%s'."),
                 id.GetIdentifier().c_str());
      Error(err);
      return NULL;
   }
   NextToken(); // swallow '('

   ArgList *args = new ArgList;
   // non-empty arglist?
   if(!token.IsChar(')'))
   {
      for(;;)
      {
         SyntaxNode *expr = ParseExpression();
         if(expr)
            args->Add(expr);
         else
	 {
            Error(_("Expected an expression in argument list."));
	    delete args;
	    return NULL;
	 }
         if(token.GetType() != Token::TT_Char)
         {
            Error(_("Expected ',' or ')' after argument."));
            delete args;
            return NULL;
         }
         else if(token.GetChar() == ')')
            break;
         else if(token.GetChar() == ',')
            NextToken(); // swallow the comma
      }
   }
   NextToken(); // swallow ')'

   FunctionDefinition *fd = FindFunction(id.GetIdentifier());
   if(fd == NULL)
   {
      String err;
      err.Printf(_("Attempt to call undefined function '%s'."),
                 id.GetIdentifier().c_str());
      Error(err);
      delete args;
      return NULL;
   }
   fd->DecRef(); // won't go away because it's still in list
   return new FunctionCall(id.GetIdentifier(), args, fd, this);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Built-in functions
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


/**
   First, a function to check for SPAM:
**/

#ifdef USE_RBL


#define __STRICT_ANSI__
#include <netinet/in.h>

// FreeBSD uses a variable name "class"
#define class xxclass
#undef MIN  // defined by glib.h
#undef MAX  // defined by glib.h
#include <arpa/nameser.h>
#undef class

#include <resolv.h>
#include <netdb.h>

#ifdef OS_SOLARIS
   extern "C" {
   // Solaris 2.5.1 has no prototypes for it:
   extern int res_init(void);
   extern int res_query(const char *, int , int ,
                        unsigned char *, int);
   };
#endif

#define BUFLEN 512

/* rblcheck()
 * Checks the specified dotted-quad address against the provided RBL
 * domain. If "txt" is non-zero, we perform a TXT record lookup. We
 * return the text returned from a TXT match, or an empty string, on
 * a successful match, or NULL on an unsuccessful match. */
static
bool CheckRBL( int a, int b, int c, int d, const String & rblDomain)
{


   char * answerBuffer = new char[ BUFLEN ];
   int len;

   String domain;
   domain.Printf("%d.%d.%d.%d.%s", d, c, b, a, rblDomain.c_str() );

   res_init();
   len = res_query( domain.c_str(), C_IN, T_A,
                    (unsigned char *)answerBuffer, PACKETSZ );

   if( len != -1 )
   {
      if( len > PACKETSZ ) // buffer was too short, re-alloc:
      {
         delete [] answerBuffer;
         answerBuffer = new char [ len ];
         // and again:
         len = res_query( domain.c_str(), C_IN, T_A,
                          (unsigned char *) answerBuffer, len );
      }
   }
   delete [] answerBuffer;
   return len != -1; // found, so it´s known spam
}

static const char * gs_RblSites[] =
{ "rbl.maps.vix.com", "relays.orbs.org", "rbl.dorkslayers.com", NULL };

static bool findIP(String &header,
                   char openChar, char closeChar,
                   int *a, int *b, int *c, int *d)
{
   String toExamine;
   String ip;
   int pos = 0, closePos;

   toExamine = header;
   while(toExamine.Length() > 0)
   {
      if((pos = toExamine.Find(openChar)) == wxNOT_FOUND)
         break;
      ip = toExamine.Mid(pos+1);
      closePos = ip.Find(closeChar);
      if(closePos == wxNOT_FOUND)
         // no second bracket found
         break;
      if(sscanf(ip.c_str(), "%d.%d.%d.%d", a,b,c,d) != 4)
      {
         // no valid IP number behind open bracket, continue
         // search:
         ip = ip.Mid(closePos+1);
            toExamine = ip;
      }
      else // read the IP number
      {
         header = ip.Mid(closePos+1);
            return true;
      }
   }
   header = "";
   return false;
}

#endif

// VZ: VC++ doesn't allow to have extern "C" functions returning a class
#ifndef _MSC_VER
extern "C"
{
#endif // VC++
   static Value func_checkSpam(ArgList *args, Parser *p)
   {
      // standard check:
      if(args->Count() != 0) return Value("");

      bool rc = false;
#ifdef USE_RBL
      String received;

      Message * msg = p->GetMessage();
      if(! msg) return Value(0);
      msg->GetHeaderLine("Received", received);
      msg->DecRef();

      int a,b,c,d;
      String testHeader = received;

      while( (!rc) && (testHeader.Length() > 0) )
      {
         if(findIP(testHeader, '(', ')', &a, &b, &c, &d))
         {
            for(int i = 0; gs_RblSites[i] && ! rc ; i++)
               rc |= CheckRBL(a,b,c,d,gs_RblSites[i]);
         }
      }
      testHeader = received;
      while( (!rc) && (testHeader.Length() > 0) )
      {
         if(findIP(testHeader, '[', ']', &a, &b, &c, &d))
         {
            for(int i = 0; gs_RblSites[i] && ! rc ; i++)
               rc |= CheckRBL(a,b,c,d,gs_RblSites[i]);
         }
      }

      /*FIXME: if it is a hostname, maybe do a DNS lookup first? */
#endif
      return rc;
   }

   static Value func_msgbox(ArgList *args, Parser *p)
   {
      SyntaxNode *sn;
      Value v;
      ASSERT(args);
      String msg;
      for(size_t i = 0; i < args->Count(); i++)
      {
         sn = args->GetArg(i);
         v= sn->Evaluate();
         msg << v.ToString();
      }
      p->Output(msg);
      return 1;
   }

   static Value func_log(ArgList *args, Parser *p)
   {
      SyntaxNode *sn;
      Value v;
      ASSERT(args);
      String msg;
      for(size_t i = 0; i < args->Count(); i++)
      {
         sn = args->GetArg(i);
         v= sn->Evaluate();
         msg << v.ToString();
      }
      p->Log(msg);
      return 1;
   }
/* * * * * * * * * * * * * * *
 *
 * Tests for message contents
 *
 * * * * * * * * * * * * * */
   static Value func_containsi(ArgList *args, Parser *p)
   {
      ASSERT(args);
      if(args->Count() != 2)
         return 0;
      Value v1 = args->GetArg(0)->Evaluate();
      Value v2 = args->GetArg(1)->Evaluate();
      String haystack = v1.ToString();
      String needle = v2.ToString();
      p->GetInterface()->strutil_tolower(haystack);
      p->GetInterface()->strutil_tolower(needle);
      return haystack.Find(needle) != -1;
   }

   static Value func_contains(ArgList *args, Parser *p)
   {
      ASSERT(args);
      if(args->Count() != 2)
         return 0;
      Value v1 = args->GetArg(0)->Evaluate();
      Value v2 = args->GetArg(1)->Evaluate();
      String haystack = v1.ToString();
      String needle = v2.ToString();
      return haystack.Find(needle) != -1;
   }

   static Value func_match(ArgList *args, Parser *p)
   {
      ASSERT(args);
      if(args->Count() != 2)
         return 0;
      Value v1 = args->GetArg(0)->Evaluate();
      Value v2 = args->GetArg(1)->Evaluate();
      String haystack = v1.ToString();
      String needle = v2.ToString();
      return haystack == needle;
   }

   static Value func_matchi(ArgList *args, Parser *p)
   {
      ASSERT(args);
      if(args->Count() != 2)
         return 0;
      Value v1 = args->GetArg(0)->Evaluate();
      Value v2 = args->GetArg(1)->Evaluate();
      String haystack = v1.ToString();
      String needle = v2.ToString();
      p->GetInterface()->strutil_tolower(haystack);
      p->GetInterface()->strutil_tolower(needle);
      return haystack == needle;
   }

   static Value DoMatchRegEx(ArgList *args, Parser *p, int flags = 0)
   {
      ASSERT(args);
      if(args->Count() != 2)
         return 0;
      Value v1 = args->GetArg(0)->Evaluate();
      Value v2 = args->GetArg(1)->Evaluate();
      String haystack = v1.ToString();
      String needle = v2.ToString();
      strutil_RegEx * re = p->GetInterface()->strutil_compileRegEx(needle);
      if(! re) return FALSE;
      bool rc = p->GetInterface()->strutil_matchRegEx(re, haystack, flags);
      p->GetInterface()->strutil_freeRegEx(re);
      return rc;
   }

   static Value func_matchregex(ArgList *args, Parser *p)
   {
      return DoMatchRegEx(args, p);
   }

   static Value func_matchregexi(ArgList *args, Parser *p)
   {
      return DoMatchRegEx(args, p, wxRegExBase::RE_ICASE);
   }

#ifdef USE_PYTHON
   static Value func_python(ArgList *args, Parser *p)
   {
#ifndef TEST	// uses functions not in libraries
      ASSERT(args);
      if(args->Count() != 1)
         return 0;
      Message * msg = p->GetMessage();
      if(! msg) return Value("");
      String funcName = args->GetArg(0)->Evaluate().ToString();

      int result = 0;
      bool rc = PyH_CallFunction(funcName,
                                 "func_python",
                                 p, "Message",
                                 "%d", &result,
                                 NULL);
      msg->DecRef();
      p->SetChanged(); // worst case guess
      return rc ? (result != 0) : false;
#else
      return 0;
#endif
   }
#else

   static Value func_python(ArgList *args, Parser *p)
   {
      p->Error(_("Python support for filters is not available."));
      return false;
   }
#endif

   static Value func_print(ArgList *args, Parser *p)
   {
#ifndef TEST	// uses functions not in libraries
      if(args->Count() != 0)
         return Value(0);

      Message * msg = p->GetMessage();
      if(! msg)
         return Value("");

      wxMessageViewFrame *mvf = new wxMessageViewFrame(NULL,
                                                       UID_ILLEGAL,
                                                       NULL);
      mvf->Show(FALSE);
      mvf->ShowMessage(msg);
      msg->DecRef();
      bool rc = mvf->GetMessageView()->Print(FALSE);
      delete mvf;

      return Value(rc);
#else
      return 0;
#endif
   }
/* * * * * * * * * * * * * * *
 *
 * Access to message contents
 *
 * * * * * * * * * * * * * */
   static Value func_subject(ArgList *args, Parser *p)
   {
      if(args->Count() != 0)
         return Value("");
      Message * msg = p->GetMessage();
      if(! msg)
         return Value("");
      String subj = msg->Subject();
      msg->DecRef();
      return Value(subj);
   }

   static Value func_from(ArgList *args, Parser *p)
   {
      if(args->Count() != 0)
         return Value("");
      Message * msg = p->GetMessage();
      if(! msg)
         return Value("");
      String subj = msg->From();
      msg->DecRef();
      return Value(subj);
   }

   static Value func_to(ArgList *args, Parser *p)
   {
      if(args->Count() != 0)
         return Value("");
      Message * msg = p->GetMessage();
      if(! msg)
         return Value("");
      String tostr;
      msg->GetHeaderLine("To", tostr);
      msg->DecRef();
      return Value(tostr);
   }

   static Value func_recipients(ArgList *args, Parser *p)
   {
      if(args->Count() != 0)
         return Value("");
      Message * msg = p->GetMessage();
      if(! msg)
         return Value("");
      String result;
      static const char *headers[] =
      {
         "To", "CC", "Bcc",
         "Resent-To", "Resent-Cc", "Resent-Bcc",
         NULL
      };
      String tmp;
      for(int i = 0; headers[i]; i++)
      {
         msg->GetHeaderLine(headers[i], tmp);
         if(tmp[0] && result[0])
            result << ',';
         result << tmp;
      }
      msg->DecRef();
      return Value(result);
   }

   static Value func_header(ArgList *args, Parser *p)
   {
      if(args->Count() != 0)
         return Value("");
      Message * msg = p->GetMessage();
      if(! msg)
         return Value("");
      String subj = msg->GetHeader();
      msg->DecRef();
      return Value(subj);
   }

   static Value func_headerline(ArgList *args, Parser *p)
   {
      if(args->Count() != 1)
         return Value("");
      Value v1 = args->GetArg(0)->Evaluate();
      String field = v1.ToString();
      Message * msg = p->GetMessage();
      if(! msg)
         return Value("");
      String result;
      msg->GetHeaderLine(field, result);
      msg->DecRef();
      return Value(result);
   }

   static Value func_body(ArgList *args, Parser *p)
   {
      if(args->Count() != 0)
         return Value("");
      Message * msg = p->GetMessage();
      if(! msg)
         return Value("");
      String str;
      msg->WriteToString(str, false);
      msg->DecRef();
      return Value(str);
   }

   static Value func_text(ArgList *args, Parser *p)
   {
      if(args->Count() != 0)
         return Value("");
      Message * msg = p->GetMessage();
      if(! msg)
         return Value("");
      String str;
      msg->WriteToString(str, true);
      msg->DecRef();
      return Value(str);
   }

   static Value func_delete(ArgList *args, Parser *p)
   {
      if(args->Count() != 0)
         return 0;
      MailFolder *mf = p->GetFolder();
      if(! mf) return Value(0);
      UIdType uid = p->GetMessageUId();
      int rc = mf->DeleteMessage(uid);
      mf->DecRef();
      p->SetChanged();
      return Value(rc);
   }

   static Value func_uniq(ArgList *args, Parser *p)
   {
      if(args->Count() != 0)
         return 0;
      MailFolder *mf = p->GetFolder();
      if(! mf) return Value(0);
      UIdType rc = mf->DeleteDuplicates();
      mf->DecRef();
      if(rc != UID_ILLEGAL) p->SetChanged();
      return Value( (int)(rc != UID_ILLEGAL) ); // success if 0 or
      // more deleted
   }

   static Value func_copytofolder(ArgList *args, Parser *p)
   {
#ifndef TEST		// UIdArray not instantiated
      if(args->Count() != 1)
         return 0;
      Value fn = args->GetArg(0)->Evaluate();
      MailFolder *mf = p->GetFolder();
      UIdType uid = p->GetMessageUId();
      UIdArray ia;
      ia.Add(uid);
      int rc = mf->SaveMessages(&ia, fn.ToString(), true, false);
      mf->DecRef();
      return Value(rc);
#else
      return 0;
#endif
   }

   static Value func_movetofolder(ArgList *args, Parser *p)
   {
      Value rc = func_copytofolder(args, p);
      if(rc.ToNumber()) // successful
      {
         ArgList emptyArgs;
         return func_delete(&emptyArgs, p);
      }
      else
         return 0;
   }

   static Value func_getstatus(ArgList *args, Parser *p)
   {
      if(args->Count() != 0)
         return Value(-1);
      Message *msg = p->GetMessage();
      int rc = msg->GetStatus();
      msg->DecRef();
      return Value(rc);
   }

   static Value func_date(ArgList *args, Parser *p)
   {
      if(args->Count() != 0)
         return Value(-1);
      Message *msg = p->GetMessage();
      struct tm tms;
      memset(&tms, 0, sizeof(tms));
      (void) msg->GetStatus(NULL,
                            (unsigned int *)&tms.tm_mday,
                            (unsigned int *)&tms.tm_mon,
                            (unsigned int *)&tms.tm_year);
      msg->DecRef();
      // cclient returns the real year whil mktime() uses 1900 as the origin
      tms.tm_year -= 1900;
      time_t today = mktime(&tms);
      today /= 60 * 60 * 24; // we count in days, not seconds
      return Value(today);
   }

   static Value func_now(ArgList *args, Parser *p)
   {
      if(args->Count() != 0)
         return Value(-1);
      time_t today = time(NULL) / 60 / 60 / 24;
      return Value(today);
   }

   static Value func_size(ArgList *args, Parser *p)
   {
      if(args->Count() != 0)
         return Value(-1);
      Message *msg = p->GetMessage();
      unsigned long size;
      (void) msg->GetStatus(&size);
      msg->DecRef();
      return Value(size / 1024); // return KiloBytes
   }

   static Value func_score(ArgList *args, Parser *p)
   {
      if(args->Count() != 0)
         return Value(0);
      int score = 0;
      Message *msg = p->GetMessage();
      HeaderInfoList *hil = p->GetFolder()->GetHeaders();
      if(hil)
      {
         score = hil->GetEntryUId( msg->GetUId() )->GetScore();
         hil->DecRef();
      }
      msg->DecRef();
      return Value(score);
   }

   static Value func_addscore(ArgList *args, Parser *p)
   {
      if(args->Count() != 1)
         return Value(-1);
      Value d = args->GetArg(0)->Evaluate();
      int delta = d.ToNumber();
      Message *msg = p->GetMessage();
      HeaderInfoList *hil = p->GetFolder()->GetHeaders();
      if(hil)
      {
         hil->GetEntryUId( msg->GetUId() )->AddScore(delta);
         hil->DecRef();
      }
      msg->DecRef();
      return Value(0);
   }
   static Value func_setcolour(ArgList *args, Parser *p)
   {
      if(args->Count() != 1)
         return Value(-1);
      Value c = args->GetArg(0)->Evaluate();
      String col = c.ToString();
      Message *msg = p->GetMessage();
      HeaderInfoList *hil = p->GetFolder()->GetHeaders();
      if(hil)
      {
         hil->GetEntryUId( msg->GetUId() )->SetColour(col);
         hil->DecRef();
      }
      msg->DecRef();
      return Value(0);
   }


/* * * * * * * * * * * * * * *
 *
 * Folder functionality
 *
 * * * * * * * * * * * * * */
   static Value func_expunge(ArgList *args, Parser *p)
   {
      if(args->Count() != 0)
         return Value(0);
      MailFolder *mf = p->GetFolder();
      mf->ExpungeMessages();
      mf->DecRef();
      p->SetChanged();
      return 1;
   }

#ifdef TEST

/* * * * * * * * * * * * * * *
 *
 * Testing hacks
 *
 * * * * * * * * * * * * * */
   static Value func_nargs(ArgList *args, Parser *p)
   {
      return args->Count();
   }
   static Value func_arg(ArgList *args, Parser *p)
   {
      if (args->Count() != 1)
         return 0;
      return args->GetArg(0)->Evaluate();
   }

#endif

#ifndef _MSC_VER
};
#endif // VC++



void
ParserImpl::AddBuiltinFunctions(void)
{
   DefineFunction("message", func_msgbox);
   DefineFunction("log", func_log);
   DefineFunction("match", func_match);
   DefineFunction("contains", func_contains);
   DefineFunction("matchi", func_matchi);
   DefineFunction("containsi", func_containsi);
   DefineFunction("matchregex", func_matchregex);
   DefineFunction("subject", func_subject);
   DefineFunction("to", func_to);
   DefineFunction("recipients", func_recipients);
   DefineFunction("headerline", func_headerline);
   DefineFunction("from", func_from);
   DefineFunction("header", func_header);
   DefineFunction("body", func_body);
   DefineFunction("text", func_text);
   DefineFunction("delete", func_delete);
   DefineFunction("uniq", func_uniq);
   DefineFunction("copy", func_copytofolder);
   DefineFunction("move", func_movetofolder);
   DefineFunction("print", func_print);
   DefineFunction("date", func_date);
   DefineFunction("size", func_size);
   DefineFunction("now", func_now);
   DefineFunction("isspam", func_checkSpam);
   DefineFunction("expunge", func_expunge);
#ifdef USE_PYTHON
   DefineFunction("python", func_python);
#endif
   DefineFunction("matchregexi", func_matchregexi);
   DefineFunction("setcolour", func_setcolour);
   DefineFunction("score", func_score);
   DefineFunction("addscore", func_addscore);
#ifndef TEST
#else
   DefineFunction("nargs", func_nargs);
   DefineFunction("arg", func_arg);
#endif
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * FilterRuleImpl - the class representing a program
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int FilterRuleImpl::Apply(MailFolder *mf, UIdType uid,
                          bool *changeflag) const
{
   if(! m_Program || ! m_Parser)
      return 0;

   ASSERT(mf);
   ASSERT(uid != UID_ILLEGAL);
//FIXME   ASSERT(mf->IsLocked()); // must be called on a locked mailfolder
   mf->IncRef();
   m_Parser->SetMessage(mf, uid);
   Value rc = m_Program->Evaluate();
   if(changeflag) *changeflag = m_Parser->GetChanged();
   m_Parser->SetMessage(NULL);
   mf->DecRef();
   return (int) rc.GetNumber();
}

int
FilterRuleImpl::Apply(MailFolder *mf, bool NewOnly,
                      bool *changeflag) const
{
   return ApplyCommonCode(mf, NULL, NewOnly, NewOnly, changeflag);
}

int
FilterRuleImpl::Apply(MailFolder *folder,
                      UIdArray msgs,
                      bool ignoreDeleted,
                      bool *changeflag) const
{
#ifndef TEST		// UIdArray not instantiated
   return ApplyCommonCode(folder, &msgs, FALSE, ignoreDeleted, changeflag);
#else
   return 0;
#endif
}

/* Common logic to test if we want to filter a message of this status
   or not: */
static inline bool
CheckStatusMatch(int status, bool newOnly, bool ignoreDeleted)
{
   return
      /* Do we want all messages or new ones only? If so, check
         that the message is RECENT and not SEEN: */
      ( ! newOnly ||
        ( (status & MailFolder::MSG_STAT_RECENT)
          && ! (status & MailFolder::MSG_STAT_SEEN) ) )
      /* Check if we need to ignore this message: */
      && (ignoreDeleted
          || (status & MailFolder::MSG_STAT_DELETED) == 0 )
      ;
}

int
FilterRuleImpl::ApplyCommonCode(MailFolder *mf,
                                UIdArray *msgs,
                                bool newOnly,
                                bool ignoreDeleted,
                                bool *changeflag) const
{
#ifndef TEST		// UIdArray not instantiated
   if(! m_Program || ! m_Parser)
      return 0;
   int rc = 1; // no error yet
   CHECK(mf, 0, "no folder for filtering");
   mf->IncRef();

   bool changed = FALSE;
   bool changedtemp;
   HeaderInfoList *hil = mf->GetHeaders();
   CHECK(hil,0,"Cannot get header info list");

   if(msgs) // apply to all messages in list
   {
      for(size_t idx = 0; idx < msgs->Count(); idx++)
      {
         const HeaderInfo * hi = hil->GetEntryUId((*msgs)[idx]);
         ASSERT(hi);
         if(hi && CheckStatusMatch(hi->GetStatus(),
                                   newOnly, ignoreDeleted))
            rc &= Apply(mf, (*msgs)[idx], &changedtemp);
         changed |= changedtemp;
      }
   }
   else // apply to all or all recent messages
   {
      for(size_t i = 0; i < hil->Count(); i++)
      {
         const HeaderInfo * hi = (*hil)[i];
         ASSERT(hi);
         if(hi && CheckStatusMatch(hi->GetStatus(),
                                   newOnly, ignoreDeleted))
         {
            rc &= Apply(mf, hi->GetUId(), &changedtemp);
            changed |= changedtemp;
         }
      }
   }
   hil->DecRef();
   mf->DecRef();
   if(changeflag) *changeflag = changed;
   return rc;
#else
   return 0;
#endif
}

FilterRuleImpl::FilterRuleImpl(const String &filterrule,
                               MInterface *minterface,
                               MModule_Filters *mod
   )
{
   m_FilterModule = mod;
   m_Parser = Parser::Create(filterrule, minterface);
   m_Program = m_Parser->Parse();
   ASSERT(mod);
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
void FilterRuleImpl::Debug(void)
{
   m_Parser->Output(m_Program->Debug());
}
#endif

/** Filtering Module class. */
class MModule_FiltersImpl : public MModule_Filters
{
   /// standard MModule functions
   MMODULE_DEFINE();

   /** Takes a string representation of a filterrule and compiles it
       into a class FilterRule object.
   */
   virtual FilterRule * GetFilter(const String &filterrule) const;
   DEFAULT_ENTRY_FUNC
protected:
   MModule_FiltersImpl()
      : MModule_Filters()
      {
      }
};

MMODULE_BEGIN_IMPLEMENT(MModule_FiltersImpl,
                        "Filters",
                        MMODULE_INTERFACE_FILTERS,
                        _("Filtering capabilities plugin"),
                        "0.00")
   MMODULE_PROP("description",
                _("This plug-in provides a filtering language for Mahogany.\n"
                  "\n"
                  "It is an interpreter for a simplified algebraic language "
                  "which allows one to apply different tests and operations "
                  "to messages, like sorting, replying or moving them "
                  "automatically."))
   MMODULE_PROP("author", "Karsten Ballüder <karsten@phy.hw.ac.uk>")
MMODULE_END_IMPLEMENT(MModule_FiltersImpl)

FilterRule *
MModule_FiltersImpl::GetFilter(const String &filterrule) const
{
   return FilterRuleImpl::Create(filterrule, m_MInterface,
                                 (MModule_FiltersImpl *) this);
}

/* static */
MModule *
MModule_FiltersImpl::Init(int vmajor, int vminor, int vrelease,
                      MInterface *minterface, int *errorCode)
{
   if(! MMODULE_SAME_VERSION(vmajor, vminor, vrelease))
   {
      if(errorCode) *errorCode = MMODULE_ERR_INCOMPATIBLE_VERSIONS;
      return NULL;
   }
   return new MModule_FiltersImpl();
}

#ifdef TEST	// test suite
#include "../classes/MObject.cpp"	// Gross hack...
class MyParser : public ParserImpl
{
public:
	MyParser(String s) : ParserImpl(s, 0) {}
	virtual void Error(const String &error);
	String CharLeft() { return ParserImpl::CharLeft(); }
	String CharMid() { return ParserImpl::CharMid(); }
};
void
MyParser::Error(const String &error)
{
   printf(">>>%s\n", error.c_str());
}

void
Rejected(MyParser &p)
{
	printf("Rejected `%s<error>%s'\n",
		p.CharLeft().c_str(),
		p.CharMid().c_str());
}

int	// test whether the string is rejected by the parser
TestExprFail(const char *s)
{
	MyParser p(s);
	SyntaxNode *exp = p.ParseExpression();
	if (exp == NULL) {
		Rejected(p);
		return 0;
	}
	Value v = exp->Evaluate();
	delete exp;
	printf("`%s' was accepted and evaluated as %ld\n", s, v.ToNumber());
	return 1;
}

int	// test whether the string is accepted by the parser
TestExpr(int arg, const char *s)
{
	MyParser p(s);
	SyntaxNode *exp = p.ParseExpression();
	if (exp == NULL) {
		Rejected(p);
		return 1;
	}
	Value v = exp->Evaluate();
	delete exp;
	if (v.ToNumber() != arg) {
		printf("`%s' was %ld instead of %d\n", s, v.ToNumber(), arg);
		return 1;
	}
	return 0;
}

int	// test whether the pgm is rejected by the parser
TestReject(const char *s)
{
	MyParser p(s);
	SyntaxNode *pgm = p.ParseProgram();
	if (pgm == NULL) {
		Rejected(p);
		return 0;
	}
	Value v = pgm->Evaluate();
	delete pgm;
	printf("`%s' was accepted and evaluated as %ld\n", s, v.ToNumber());
	return 1;
}

int	// test whether the pgm is accepted by the parser
TestAccept(const char *s)
{
	MyParser p(s);
	SyntaxNode *pgm = p.ParseProgram();
	if (pgm == NULL) {
		Rejected(p);
		return 1;
	}
	delete pgm;
	return 0;
}

int	// test whether the pgm is accepted by the parser
TestPgm(int arg, const char *s)
{
	MyParser p(s);
	SyntaxNode *pgm = p.ParseProgram();
	if (pgm == NULL) {
		Rejected(p);
		return 1;
	}
	Value v = pgm->Evaluate();
	delete pgm;
	if (v.ToNumber() != arg) {
		printf("`%s' was %ld instead of %d\n", s, v.ToNumber(), arg);
		return 1;
	}
	return 0;
}

int
main(void)
{
	int errs = 0;
	for (;;) {
		int c = getchar();
		if (c == EOF)
			break;
		if (c == '\n')
			continue;
		if (c == '#') {
			// swallow comment
			while (getchar() != '\n') {}
			continue;
		}
		String cmd = (char)c, opt, exp;
		while ((c = getchar()) != '\t')
			cmd += (char)c;
		while ((c = getchar()) != '\t')
			opt += (char)c;
		while ((c = getchar()) != '\n')
			exp += (char)c;
		if (cmd == "expr") {
			long val;
			if (opt == "reject")
				errs += TestExprFail(exp);
			else if (opt.ToLong(&val))
				errs += TestExpr(val, exp);
			else
				printf("Unknown option `%s' to expr command\n",
					opt.c_str());
		} else
		if (cmd == "pgm") {
			long val;
			if (opt == "accept")
				errs += TestAccept(exp);
			else if (opt == "reject")
				errs += TestReject(exp);
			else if (opt.ToLong(&val))
				errs += TestPgm(val, exp);
			else
				printf("Unknown option `%s' to pgm command\n",
					opt.c_str());
		} else
			printf("Unknown command `%s'\n", cmd.c_str());
	}

	if (errs > 0)
		printf("%d errors found\n", errs);

	return errs != 0;
}
#endif
