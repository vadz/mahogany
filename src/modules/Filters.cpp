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

#include   "Mpch.h"

// VZ: Greg, testing is nice, but this breaks Windows compilation!
#if 0
#ifndef TEST
#   include   "Mpch.h"
#else
//  For regression testing, we can't use the pre-compiled headers.
//  Instead, we pull the configuration options, tweak them to suit
//  our nefarious purposes, and include the needed headers below.
#   include "Mconfig.h"
#   undef USE_PCH              // pull in headers below
#   undef USE_MODULES_STATIC   // don't pull in static linking crud
//  turn on some debugging options in case they weren't already
#   undef DEBUG                // in case it's already set
#   define DEBUG 1
#   undef DEBUG_filters        // in case it's already set
#   define DEBUG_filters 1
#endif
#endif // 0

#ifndef USE_PCH
#   include   "Mcommon.h"
#   include   "Message.h"
#   include   "MailFolder.h"
#endif

#include "MModule.h"
#include "MInterface.h"
#include "MPython.h"
#include "HeaderInfo.h"

#include <stdlib.h>
#include <ctype.h>
#include <time.h>  // mktime()

#include "modules/Filters.h"
#include "MFilterLang.h"

#include "UIdArray.h"

#include <wx/regex.h>   // wxRegEx::Flags

// ----------------------------------------------------------------------------
// global variables
// ----------------------------------------------------------------------------

// this is exquisitely ugly but I don't see any other way to return any
// information from the spam test functions so we just use a global flag :-(
static String gs_spamTest;                // MT-FIXME

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// all recipient headers, more can be added (but always NULL terminate!)
static const char *headersRecipients[] =
{
   "To",
   "CC",
   "Bcc",
   "Resent-To",
   "Resent-Cc",
   "Resent-Bcc",
   NULL
};

// forward declare all of our classes
class ArgList;
class Expression;
class Filter;
class FilterRuleImpl;
class FunctionCall;
class FunctionDefinition;
class IfElse;
class Negation;
class Negative;
class Number;
class Statement;
class StringConstant;
class SyntaxNode;
class Token;
class Value;

/// Type for functions to be called.
typedef Value (* FunctionPointer)(ArgList *args, FilterRuleImpl *p);

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
   TokenType GetType(void) const
      { return m_type; }
   void SetChar(char c)
      { m_type = TT_Char; m_number = (unsigned)c; }
   void SetOperator(OperatorType oper)
      { m_type = TT_Operator; m_number = oper; }
   void SetString(String const &s)
      { m_type = TT_String; m_string = s; }
   void SetNumber(long n)
      { m_type = TT_Number; m_number = n; }
   void SetIdentifier(String const &s)
      { m_type = TT_Identifier; m_string = s; }
   void SetEOF(void)
      { m_type = TT_EOF; }
   void SetInvalid(void)
      { m_type = TT_Invalid; }

   int IsChar(char c) const
      { return m_type == TT_Char && (unsigned char)m_number == (unsigned)c; }
   int IsOperator(void) const
      { return m_type == TT_Operator; }
   int IsOperator(OperatorType t) const
      { return (m_type == TT_Operator) && ((OperatorType)m_number) == t; }
   int IsString(void) const
      { return m_type == TT_String; }
   int IsNumber(void) const
      { return m_type == TT_Number; }
   int IsIdentifier(const char *s) const
      { return m_type == TT_Identifier && m_string == s; }
   int IsEOF(void) const
      { return m_type == TT_EOF; }

   char GetChar(void) const
      { ASSERT(m_type == TT_Char); return m_number; }
   OperatorType GetOperator() const
      { ASSERT(m_type == TT_Operator); return (OperatorType)m_number; }
   const String & GetString(void) const
      { ASSERT(m_type == TT_String); return m_string; }
   long GetNumber(void) const
      { ASSERT(m_type == TT_Number); return m_number; }
   const String & GetIdentifier(void) const
      { ASSERT(m_type == TT_Identifier); return m_string; }

private:
   TokenType m_type;
   long      m_number;
   String    m_string;
};

/** Parsed representation of a filtering rule to be applied to a
    message.
*/
class FilterRuleImpl : public FilterRule
{
public:
   // the operations a filter rule can perform on a message
   enum
   {
      None     = 0,
      Copy     = 1,
      Delete   = 2,
      Expunge  = 4
   };

   // implement the base class pure virtual
   virtual int Apply(MailFolder *folder, UIdArray& msgs);

   static FilterRule * Create(const String &filterrule,
                              MInterface *minterface,
                              MModule_Filters *mod)
      { return new FilterRuleImpl(filterrule, minterface, mod); }
#ifdef DEBUG
   void Debug(void);
#endif

protected:
   FilterRuleImpl(const String &filterrule,
                  MInterface *minterface,
                  MModule_Filters *fmodule);
   ~FilterRuleImpl();

public:
   const SyntaxNode * Parse(const String &);
   const SyntaxNode * ParseProgram(void);
   const SyntaxNode * ParseFilters(void);
   const SyntaxNode * ParseIfElse(void);
   const SyntaxNode * ParseBlock(void);
   const SyntaxNode * ParseStmts(void);
   const SyntaxNode * ParseExpression(void);
   const SyntaxNode * ParseCondition(void);
   const SyntaxNode * ParseQueryOp(void);
   const SyntaxNode * ParseOrs(void);
   const SyntaxNode * ParseIffs(void);
   const SyntaxNode * ParseAnds(void);
   const SyntaxNode * ParseBOrs(void);
   const SyntaxNode * ParseXors(void);
   const SyntaxNode * ParseBAnds(void);
   const SyntaxNode * ParseRelational(void);
   const SyntaxNode * ParseTerm(void);
   const SyntaxNode * ParseFactor(void);
   const SyntaxNode * ParseUnary(void);
   const SyntaxNode * ParseFunctionCall(Token id);
   size_t GetPos(void) const { return m_Position; }
   Token GetToken(bool remove = true);
   Token PeekToken(void) { return token; }
   void Rewind(size_t pos = 0);
   void NextToken(void) { Rewind(m_Peek); }

#ifdef TEST
   virtual // So we can override the Error function
#endif
   void Error(const String &error);
   void Output(const String &msg)
      { m_MInterface->MessageDialog(msg,NULL,_("Filters output")); }
   void Log(const String &imsg, int level = M_LOG_DEFAULT)
      {
         String msg = _("Filters: ");
         msg << imsg;
         m_MInterface->Log(level, msg);
      }

   /// check if a function is already defined
   const FunctionDefinition *FindFunction(const String &name);

   /**@name for runtime information */
   //@{

   /// Obtain the mailfolder to operate on:
   MailFolder * GetFolder(void) const
      { SafeIncRef(m_MailFolder); return m_MailFolder; }

   /// Obtain the message UId to operate on:
   UIdType GetMessageUId(void) const { return m_MessageUId; }

   /// Obtain the message itself:
   Message * GetMessage(void) const
      { SafeIncRef(m_MailMessage); return m_MailMessage; }

   //@}

   /// obtain the interface to the main program
   MInterface *GetInterface(void) { return m_MInterface; }

   /// called if messages should be expunged from folder
   void SetExpunged() { m_operation |= Expunge; }

   /// called by func_delete() to tell us that a message was deleted
   void SetDeleted() { m_operation |= Delete; }

   /// called by func_copytofolder()
   void CopyTo(const String& copiedTo)
   {
      m_copiedTo = copiedTo;
      m_operation = Copy;
   }

protected:
   void EatWhiteSpace(void)
      { while(isspace(m_Input[m_Position])) m_Position++; }
   const char Char(void) const
      { return m_Input[m_Position]; }
   const char CharInc(void)
      { return m_Input[m_Position++]; }
   String CharLeft(void)
      { return m_Input.Left(m_Position); }
   String CharMid(void)
      { return m_Input.Mid(m_Position); }

private:
   MModule_Filters *m_FilterModule;
   MInterface *m_MInterface;

   String m_Input;
   Token token;              // current token
   size_t m_Position;        // seek offset of current token
   size_t m_Peek;            // seek offset of next token
   const SyntaxNode *m_Program; // compiled filter program

   // the UID of the message we're currently filtering
   UIdType m_MessageUId;

   // the message itself
   Message *m_MailMessage;

   // the folder we're working in
   MailFolder *m_MailFolder;

   // the operation we should perform with the current message
   int m_operation;

   // the folder the message was copied or moved to or empty
   String m_copiedTo;

   // the optimization hints - set in FindFunction(), used in Apply()
   bool m_hasToFunc,
        m_hasRcptFunc,
        m_hasHdrLineFunc,
        m_hasHeaderFunc;

   GCC_DTOR_WARN_OFF
};

///------------------------------
/// Own functionality:
///------------------------------

/** This is a value. */
class Value : public MObject
{
   /** This enum is used to distinguish variable or result types. */
   enum Type
   {
      Type_Error,  /// Undefined value (used for expression error)
      Type_Number, /// Value is a long int number
      Type_String, /// Value is a string
      Type_Max
   };

public:
   // constructors
   Value() : m_Type(Type_Error) { Init(); }
   Value(long num) : m_Type(Type_Number), m_Num(num) { Init(); }
   Value(const String &str) : m_Type(Type_String), m_String(str) { Init(); }

   // suppress Purify warnings about UMRs
#ifdef DEBUG
   Value(const Value& val) : m_Type(val.m_Type), m_String(val.m_String)
   {
      m_abort = val.m_abort;

      if ( m_Type == Type_Number )
         m_Num = val.m_Num;
   }
#endif // DEBUG

   bool IsValid(void) const
      { MOcheck(); return m_Type != Type_Error; }
   bool IsNumber(void) const
      { MOcheck(); return m_Type == Type_Number; }
   bool IsString(void) const
      { MOcheck(); return m_Type == Type_String; }
   bool IsSame(const Value &v) const
      { MOcheck(); v.MOcheck(); return v.m_Type == m_Type; }
   long GetNumber(void) const
      {
         MOcheck();
         ASSERT(m_Type == Type_Number);
         return m_Num;
      }
   const String GetString(void) const
      {
         MOcheck();
         ASSERT(m_Type == Type_String);
         return m_String;
      }
   bool MakeNumber(void) const
      {
         MOcheck();
         if (m_Type == Type_String)
         {
            Value *that = (Value*)this;
            if ( !m_String.ToLong(&that->m_Num) )
               return false;

            that->m_Type = Type_Number;
         }
         ASSERT(m_Type == Type_Number);
         return true;
      }
   long ToNumber(void) const
      {
         MOcheck();
         if(!MakeNumber())
            return m_String.Length();
         return GetNumber();
      }
   const String ToString(void) const
      {
         MOcheck();
         if(m_Type == Type_String)
            return m_String;

         return String::Format("%ld", m_Num);
      }

   void Abort() { m_abort = true; }
   bool ShouldAbort() const { return m_abort; }

private:
   void Init() { m_abort = false; }

   Type m_Type;

   // Can't use union here because m_String needs constructor.
   long   m_Num;
   const String m_String;

   // if this flag is set, it means that the filter evaluation must be aborted
   // (e.g. because the message was deleted)
   bool m_abort;

   MOBJECT_NAME(Value)
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
   virtual const Value Evaluate(void) const = 0;
   virtual String ToString(void) const
      { return Evaluate().ToString(); }
#ifdef DEBUG
   virtual String Debug(void) const = 0;
#endif
private:
   MOBJECT_NAME(SyntaxNode)
};

class SequentialEval : public SyntaxNode
{
public:
   SequentialEval(const SyntaxNode *r, const SyntaxNode *n)
      : m_Rule(r), m_Next(n)
      {
         ASSERT(m_Rule); ASSERT(m_Next);
      }
   ~SequentialEval(void) { delete m_Rule; delete m_Next; }
   virtual const Value Evaluate() const
      {
         MOcheck();

         Value v = m_Rule->Evaluate();
         if ( v.ShouldAbort() )
         {
            // don't evaluate the rest
            return v;
         }

         // tail recursion, so no add'l stack frame
         return m_Next->Evaluate();
      }

protected:
   const SyntaxNode *m_Rule,
                    *m_Next;
};

class Filter : public SequentialEval
{
public:
   Filter(const SyntaxNode *r, const SyntaxNode *n)
      : SequentialEval(r, n)
   {
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

protected:
   FilterRuleImpl *m_Parser;

   MOBJECT_NAME(Filter)
};

class Statement : public SequentialEval
{
public:
   Statement(const SyntaxNode *r, const SyntaxNode *n)
      : SequentialEval(r, n) {}
#ifdef DEBUG
   virtual String Debug(void) const
      {
         MOcheck();
         String s = m_Rule->Debug();
         s << ';' << m_Next->Debug();
         return s;
      }
#endif
   MOBJECT_NAME(Statement)
};

class Number : public SyntaxNode
{
public:
   Number(long v) { m_value = v; }
   virtual const Value Evaluate() const { MOcheck(); return m_value; }
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
   StringConstant(String v) : m_String(v) {}
   virtual const Value Evaluate() const
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
   const String m_String;
   MOBJECT_NAME(StringConstant)
};

class Negation : public SyntaxNode
{
public:
   Negation(const SyntaxNode *sn) { m_Sn = sn; }
   ~Negation() { MOcheck(); delete m_Sn; }
   virtual const Value Evaluate() const
      {
         MOcheck();
         Value v = m_Sn->Evaluate();
         return ! (v.MakeNumber() ?
            v.GetNumber() : (long)v.GetString().Length());
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
   const SyntaxNode *m_Sn;
   MOBJECT_NAME(Negation)
};

class Negative : public SyntaxNode
{
public:
   Negative(const SyntaxNode *sn) { m_Sn = sn; }
   ~Negative() { MOcheck(); delete m_Sn; }
   virtual const Value Evaluate() const
      {
         MOcheck();
         Value v = m_Sn->Evaluate();
         return -(v.MakeNumber() ?
            v.GetNumber() : (long)v.GetString().Length());
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
   const SyntaxNode *m_Sn;
   MOBJECT_NAME(Negative)
};

/** Functioncall handling */

/** This little class contains the definition of a function,
    i.e. links its name to a bit of C code. */

class FunctionDefinition
{
public:
   FunctionDefinition(const char *name, FunctionPointer fptr)
      : m_Name(name), m_FunctionPtr(fptr)
      { ASSERT(m_Name); ASSERT(m_FunctionPtr); }
   const char *GetName(void) const { return m_Name; }
   FunctionPointer GetFPtr(void) const { return m_FunctionPtr; }

private:
   String          m_Name;
   FunctionPointer m_FunctionPtr;
};
M_LIST(FunctionList, FunctionDefinition);

/** These classes represents a function call. */

#define ARGLIST_DEFAULT_LENGTH 8

class ArgList : public MObject
{
   typedef const SyntaxNode *SyntaxNodePtr;
public:
   ArgList()
      {
         m_MaxArgs = ARGLIST_DEFAULT_LENGTH;
         m_Args = new SyntaxNodePtr[m_MaxArgs];
         m_NArgs = 0;
      }
   ~ArgList()
      {
         MOcheck();
         for(size_t i = 0; i < m_NArgs; ++i)
            delete m_Args[i];
         delete [] m_Args;
      }
   void Add(const SyntaxNode *arg)
      {
         MOcheck();
         if(m_NArgs == m_MaxArgs)
         {
            size_t t = m_MaxArgs + m_MaxArgs;
            SyntaxNodePtr *args = new SyntaxNodePtr[t];
            for (size_t i = 0; i < m_NArgs; ++i)
               args[i] = m_Args[i];
            delete [] m_Args;
            m_MaxArgs = t, m_Args = args;
         }
         m_Args[m_NArgs++] = arg;
      }
   size_t Count(void) const
      {
         MOcheck();
         return m_NArgs;
      }
   const SyntaxNode * GetArg(size_t n) const
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

class FunctionCall : public SyntaxNode
{
public:
   // takes ownership of the FunctionDefinition argument
   FunctionCall(const FunctionDefinition *fd, ArgList *args, FilterRuleImpl *p)
      : m_fd(fd), m_args(args), m_Parser(p)
      {
         ASSERT(m_fd); ASSERT(m_args); ASSERT(m_Parser);
      }
   ~FunctionCall()
      {
         MOcheck();
         delete m_args;
      }
   virtual const Value Evaluate() const
      {
         MOcheck();
         return (*m_fd->GetFPtr())(m_args, m_Parser);
      }
#ifdef DEBUG
   virtual String Debug(void) const
      {
         MOcheck();
         return String("FunctionCall(") + m_fd->GetName() + String(")");
      }
#endif
private:
   const FunctionDefinition *m_fd;
   ArgList *m_args;
   FilterRuleImpl *m_Parser;
   MOBJECT_NAME(FunctionCall)
};

class QueryOp : public SyntaxNode
{
public:
   QueryOp(const SyntaxNode *cond, const SyntaxNode *left,
           const SyntaxNode *right)
      : m_Cond(cond), m_Left(left), m_Right(right)
      {
         ASSERT(m_Cond); ASSERT(m_Left); ASSERT(m_Right);
      }
   ~QueryOp(void)
      {
         MOcheck();
         delete m_Cond;
         delete m_Left;
         delete m_Right;
      }
   virtual const Value Evaluate(void) const
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
         s << m_Cond->Debug()
           << '?'
           << m_Left->Debug()
           << ':'
           << m_Right->Debug()
           << ')';
         return s;
      }
#endif

private:
   const SyntaxNode *m_Cond, *m_Left, *m_Right;
   MOBJECT_NAME(QueryOp)
};

/** A binary expression.  ABC for operators. */
class Expression : public SyntaxNode
{
public:
   Expression(const SyntaxNode *left, const SyntaxNode *right)
      : m_Left(left), m_Right(right)
      {
         ASSERT(m_Left); ASSERT(m_Right);
      }
   ~Expression()
      {
         MOcheck();
         delete m_Left;
         delete m_Right;
      }
#ifdef DEBUG
   virtual const char *OperName(void) const = 0;
   String Debug(void) const
      {
         MOcheck();
         String s = "(";
         s << m_Left->Debug() << OperName() << m_Right->Debug() << ')';
         return s;
      }
#endif
protected:
   const SyntaxNode *m_Left, *m_Right;
   MOBJECT_NAME(Expression)
};

/* The following macros use Value() as an error type to indicate that
   one was trying to combine two non-matching types. */

#define IMPLEMENT_VALUE_OP(oper, string) \
static inline Value operator oper(const Value &left, const Value &right) \
{ \
   if(left.IsValid() && right.IsValid()) \
   { \
      if(!left.IsSame(right)) \
      { \
         if(!left.MakeNumber() || !right.MakeNumber()) \
            return Value(); \
      } \
      if(left.IsNumber()) \
         return Value(left.GetNumber() oper right.GetNumber()); \
      if(left.IsString()) \
         return Value(left.string oper right.string); \
      ASSERT(0); \
   } \
   return Value(); \
}

#ifdef DEBUG
#define IMPLEMENT_OP(name, oper, string) \
IMPLEMENT_VALUE_OP(oper, string); \
class Operator##name : Expression \
{ \
public: \
   Operator##name(const SyntaxNode *l, const SyntaxNode *r) \
      : Expression(l, r) {} \
   static const SyntaxNode *Create(const SyntaxNode *l, const SyntaxNode *r) \
      { return new Operator##name(l, r); } \
   virtual const Value Evaluate(void) const \
      { return m_Left->Evaluate() oper m_Right->Evaluate(); } \
   virtual const char *OperName(void) const { return #oper; } \
}

#else        // not DEBUGing

#define IMPLEMENT_OP(name, oper, string) \
IMPLEMENT_VALUE_OP(oper, string) \
class Operator##name : Expression \
{ \
public: \
   Operator##name(const SyntaxNode *l, const SyntaxNode *r) \
      : Expression(l, r) {} \
   static const SyntaxNode *Create(const SyntaxNode *l, const SyntaxNode *r) \
      { return new Operator##name(l, r); } \
   virtual const Value Evaluate(void) const \
      { return m_Left->Evaluate() oper m_Right->Evaluate(); } \
}
#endif

IMPLEMENT_OP(Plus,+,GetString());
IMPLEMENT_OP(Minus,-,GetString().Length());
IMPLEMENT_OP(Times,*,GetString().Length());
IMPLEMENT_OP(Divide,/,GetString().Length());
IMPLEMENT_OP(Mod,%,GetString().Length());

IMPLEMENT_OP(Less, <, GetString());
IMPLEMENT_OP(Leq, <=, GetString());
IMPLEMENT_OP(Greater, >, GetString());
IMPLEMENT_OP(Geq, >=, GetString());
IMPLEMENT_OP(Equal, ==, GetString());
IMPLEMENT_OP(Neq, !=, GetString());

// special logic for AND
IMPLEMENT_VALUE_OP(&&, GetString().Length());
class OperatorAnd : Expression
{
public:
   OperatorAnd(const SyntaxNode *l, const SyntaxNode *r) : Expression(l, r) {}
   static const SyntaxNode *Create(const SyntaxNode *l, const SyntaxNode *r)
      { return new OperatorAnd(l, r); }
   virtual const Value Evaluate(void) const
      {
         Value lv = m_Left->Evaluate();
         if(lv.ToNumber())
            return lv && m_Right->Evaluate();

         return lv;
      }
#ifdef DEBUG
   virtual const char *OperName(void) const { return "&&"; }
#endif
};

// special logic for OR
IMPLEMENT_VALUE_OP(||, GetString().Length());
class OperatorOr : Expression
{
public:
   OperatorOr(const SyntaxNode *l, const SyntaxNode *r) : Expression(l, r) {}
   static const SyntaxNode *Create(const SyntaxNode *l, const SyntaxNode *r)
      { return new OperatorOr(l, r); }
   virtual const Value Evaluate(void) const
      {
         Value lv = m_Left->Evaluate();
         if(! lv.ToNumber())
            return lv || m_Right->Evaluate();

         return lv;
      }
#ifdef DEBUG
   virtual const char *OperName(void) const { return "||"; }
#endif
};

/** An if-else statement. */
class IfElse : public SyntaxNode
{
public:
   IfElse(const SyntaxNode *condition,
          const SyntaxNode *ifBlock,
          const SyntaxNode *elseBlock)
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
   virtual const Value Evaluate(void) const
      {
         MOcheck();
         ASSERT(m_Condition != NULL);
         ASSERT(m_IfBlock != NULL);
         const Value rc = m_Condition->Evaluate();
         if(rc.ToNumber())
            return m_IfBlock->Evaluate();
         if(m_ElseBlock)
            return m_ElseBlock->Evaluate();

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
   const SyntaxNode *m_Condition, *m_IfBlock, *m_ElseBlock;
   MOBJECT_NAME(IfElse)
};

static const FunctionList *BuiltinFunctions(void);

const FunctionDefinition *
FilterRuleImpl::FindFunction(const String &name)
{
   // SOMEDAY: when user-defined functions, search that list, too
   const FunctionList *list = BuiltinFunctions();
   for (FunctionList::iterator i = list->begin(); i != list->end(); ++i)
   {
      if ( name == i->GetName() )
      {
         // remember if we have some particular functions - we use it to
         // optimize filter execution in Apply()
         if ( name == "to" )
            m_hasToFunc = true;
         else if ( name == "recipients" )
            m_hasRcptFunc = true;
         else if ( name == "headerline" )
            m_hasHdrLineFunc = true;
         else if ( name == "header" )
            m_hasHeaderFunc = true;

         return i.operator->();
      }
   }

   return NULL;
}

void
FilterRuleImpl::Error(const String &error)
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
FilterRuleImpl::GetToken(bool remove)
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
FilterRuleImpl::Rewind(size_t pos)
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
      long v = CharInc() - '0';
      while(isdigit(Char()))
         v = v*10 + (CharInc() - '0');
      token.SetNumber(v);
   }
   else if(Char() == '"') // a quoted string
   {
      m_Position++;
      bool escaped = false;
      for (;;)
      {
         if(Char() == '\0')
         {
            Error(_("Unterminated string."));
            token.SetInvalid();
            break;
         }
         if(escaped)
         {
            escaped = false;
         }
         else if(Char() == '\\')
         {
            escaped = true;
            ++m_Position;
            continue; // skip it
         }
         else if(Char() == '"')
         {
            token.SetString(tokstr);
            ++m_Position;
            break;
         }
         tokstr += CharInc();
      }
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
         fseek(fp, 0, SEEK_END);
         long len = ftell(fp);
         fseek(fp, 0, SEEK_SET);
         if(len > 0)
         {
            char *cp = new char[len + 1];
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

const SyntaxNode *
FilterRuleImpl::Parse(const String &input)
{
   MOcheck();
   /* Here we handle the one special occasion of input being @filename
      in which case we replace input with the contents of that file.
   */
   m_Input = input;
   PreProcessInput(&m_Input);
   Rewind();
#ifndef TEST
   return ParseProgram();
#else
   return 0;
#endif
}

const SyntaxNode *
FilterRuleImpl::ParseProgram(void)
{
   MOcheck();
   if(token.IsEOF())
   {
      Error(_("No filter program found"));
      return NULL;
   }
   const SyntaxNode * pgm = ParseFilters();
   if(pgm == NULL)
      Error(_("Parse error, cannot find valid program."));
   return pgm;
}

const SyntaxNode *
FilterRuleImpl::ParseFilters(void)
{
   MOcheck();
   const SyntaxNode * filter = NULL;
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
   const SyntaxNode * next = ParseFilters();
   if (next == NULL) {
      delete filter;
      return NULL;
   }
   return new Filter(filter, next);
}

const SyntaxNode *
FilterRuleImpl::ParseIfElse(void)
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

   const SyntaxNode *condition = ParseCondition();
   if(! condition)
      return NULL;

   if(!token.IsChar(')'))
   {
      Error(_("expected ')' after condition in if statement."));
      delete condition;
      return NULL;
   }
   NextToken(); // swallow ')'

   const SyntaxNode *ifBlock = ParseBlock();
   if(! ifBlock) {
      delete condition;
      return NULL;
   }

   const SyntaxNode *elseBlock = NULL;
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

const SyntaxNode *
FilterRuleImpl::ParseBlock(void)
{
   MOcheck();
   // Normal block:
   if(!token.IsChar('{'))
   {
      Error(_("Expected '{' at start of block."));
      return NULL;
   }
   NextToken(); // swallow '{'
   const SyntaxNode * stmt;
   if(token.IsChar('{'))
      stmt = ParseBlock(); // it can generate {{{ ... }}}
   else
      stmt = ParseStmts();
   if(stmt == NULL)
   {
      Error(_("Expected statements after '{'"));
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

const SyntaxNode *
FilterRuleImpl::ParseStmts(void)
{
   MOcheck();
   const SyntaxNode * stmt;
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
   const SyntaxNode * next = ParseStmts();
   if(next == NULL) {
      delete stmt;
      return NULL;
   }
   return new Statement(stmt, next);
}

const SyntaxNode *
FilterRuleImpl::ParseExpression(void)
{
   MOcheck();
   return ParseQueryOp();
}

const SyntaxNode *
FilterRuleImpl::ParseCondition(void)
{
   MOcheck();
   const SyntaxNode *sn = ParseQueryOp();
   if (sn != NULL)
      return sn;
   Error(_("Invalid conditional expression"));
   return NULL;
}

const SyntaxNode *
FilterRuleImpl::ParseQueryOp(void)
{
   MOcheck();
   const SyntaxNode *sn = ParseOrs();
   if(sn == NULL)
      return NULL;
   if(!token.IsChar('?'))
           return sn;
   NextToken();
   const SyntaxNode *left = ParseExpression();
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
   const SyntaxNode *right = ParseExpression();
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
   case Token::Operator_##oper: return Operator##oper::Create

// This ditty implements the dreaded left-recursive grammar
//      name    := part
//              | name op part
// That is, a string of left-associative operators at the same
// precedence level.  The operands are all of type `part'
typedef const SyntaxNode *(*OpCreate)(const SyntaxNode *l, const SyntaxNode *r);
#define LeftAssoc(name,opers,part,msg) \
const SyntaxNode * \
FilterRuleImpl::Parse##name(void) \
{ \
   MOcheck(); \
   const SyntaxNode *expr = Parse##part(); \
   if (expr == NULL) \
      return NULL; \
   for (;;) \
   { \
      OpCreate op = opers(token); \
      if (op == NULL) \
         break; \
      NextToken(); \
      const SyntaxNode *exp = Parse##part(); \
      if (exp == NULL) { \
         delete expr; \
         Error(msg); \
         return NULL; \
      } \
      expr = (*op)(expr, exp); \
   } \
   return expr; \
}

static inline OpCreate
OrOp(Token t)
{
   if (t.IsOperator())
      switch (t.GetOperator())
      {
         OPERATOR_VALUE(Or);
         default: return NULL;
      }
   else if (t.IsIdentifier("or"))
      return OperatorOr::Create;
   return NULL;
}
LeftAssoc(Ors,OrOp,Iffs,_("Expected expression after OR operator"))

static inline OpCreate
IffOp(Token t)
{
#if 0     // unimplemented
   if (t.IsOperator())
      switch (t.GetOperator())
      {
         OPERATOR_VALUE(Iff);
         default: return NULL;
      }
   else if (t.IsIdentifier("iff"))
   return OperatorIff::Create;
#endif
   return NULL;
}
LeftAssoc(Iffs,IffOp,Ands,_("Expected expression after IFF operator"))

static inline OpCreate
AndOp(Token t)
{
   if (t.IsOperator())
      switch (t.GetOperator())
      {
         OPERATOR_VALUE(And);
         default: return NULL;
      }
   else if (t.IsIdentifier("and"))
      return OperatorAnd::Create;
   return NULL;
}
LeftAssoc(Ands,AndOp,BOrs,_("Expected expression after AND operator"))

static inline OpCreate
BOrOp(Token t)
{
#if 0     // unimplemented
   if (t.IsOperator())
      switch (t.GetOperator())
      {
         OPERATOR_VALUE(BOr);
         default: return NULL;
      }
#endif
   return NULL;
}
LeftAssoc(BOrs,BOrOp,Xors,_("Expected expression after bit OR operator"))

static inline OpCreate
XorOp(Token t)
{
#if 0     // unimplemented
   if (t.IsOperator())
      switch (t.GetOperator())
      {
         OPERATOR_VALUE(Xor);
         default: return NULL;
      }
   else if (t.IsIdentifier("xor"))
      return OperatorXor::Create;
#endif
   return NULL;
}
LeftAssoc(Xors,XorOp,BAnds,_("Expected expression after XOR operator"))

static inline OpCreate
BAndOp(Token t)
{
#if 0     // unimplemented
   if (t.IsOperator())
      switch (t.GetOperator())
      {
         OPERATOR_VALUE(BAnd);
         default: return NULL;
      }
#endif
   return NULL;
}
LeftAssoc(BAnds,BAndOp,Relational,
          _("Expected expression after bit AND operator"))

static inline OpCreate
RelOp(Token t)
{
   if (!t.IsOperator())
      return NULL;
   switch (t.GetOperator())
   {
      OPERATOR_VALUE(Less);
      OPERATOR_VALUE(Leq);
      OPERATOR_VALUE(Greater);
      OPERATOR_VALUE(Geq);
      OPERATOR_VALUE(Equal);
      OPERATOR_VALUE(Neq);

      // needed to shut up gcc warnings
      default:
         ;
   }
   return NULL;
}
// Relationals are a special case; they don't associate.
const SyntaxNode *
FilterRuleImpl::ParseRelational(void)
{
   MOcheck();
   const SyntaxNode *expr = ParseTerm();
   if (expr == NULL)
      return NULL;
   OpCreate op = RelOp(token);
   if (op == NULL)
      return expr;
   NextToken();
   const SyntaxNode *exp = ParseTerm();
   if (exp == NULL) {
      delete expr;
      Error(_("Expected expression after relational operator"));
      return NULL;
   }
   return (*op)(expr, exp);
}

static inline OpCreate
AddOp(Token t)
{
   if (!t.IsOperator())
      return NULL;
   switch (t.GetOperator())
   {
      OPERATOR_VALUE(Plus);
      OPERATOR_VALUE(Minus);

      // needed to shut up gcc warnings
      default:
         ;
   }

   return NULL;
}
LeftAssoc(Term,AddOp,Factor,_("Expected term after plus/minus operator"))

static inline OpCreate
MulOp(Token t)
{
   if (!t.IsOperator())
      return NULL;
   switch (t.GetOperator())
   {
      OPERATOR_VALUE(Times);
      OPERATOR_VALUE(Divide);
      OPERATOR_VALUE(Mod);

      // needed to shut up gcc warnings
      default:
         ;
   }
   return NULL;
}
LeftAssoc(Factor,MulOp,Unary,
          _("Expected factor after multiply/divide/modulus operator"))

const SyntaxNode *
FilterRuleImpl::ParseUnary(void)
{
   MOcheck();
   const SyntaxNode *sn = NULL;
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
   else if( token.IsOperator() )
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

const SyntaxNode *
FilterRuleImpl::ParseFunctionCall(Token id)
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
         const SyntaxNode *expr = ParseExpression();
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

   const FunctionDefinition *fd = FindFunction(id.GetIdentifier());
   if(fd == NULL)
   {
      String err;
      err.Printf(_("Attempt to call undefined function '%s'."),
                 id.GetIdentifier().c_str());
      Error(err);
      delete args;
      return NULL;
   }
   return new FunctionCall(fd, args, this);
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

#endif // USE_RBL

// ----------------------------------------------------------------------------
// SPAM tests: different functions below are all used by func_isspam()
// ----------------------------------------------------------------------------

// check whether the subject contains too many raw 8 bit characters
static bool CheckSubjectFor8Bit(const String& subject)
{
   // consider that the message is a spam if its subject contains more
   // than half of non alpha numeric chars but isn't properly encoded
   if ( subject != MailFolder::DecodeHeader(subject) )
   {
      // an encoded subject can have 8 bit chars
      return false;
   }

   size_t num8bit = 0,
          max8bit = subject.length() / 2;
   for ( const unsigned char *pc = (const unsigned char *)subject.c_str();
         *pc;
         pc++ )
   {
      if ( *pc > 127 || *pc == '?' || *pc == '!' )
      {
         if ( num8bit++ == max8bit )
         {
            return true;
         }
      }
   }

   return false;
}

// check if the subject is in capitals
static bool CheckSubjectForCapitals(const String& subject)
{
   size_t len = 0;
   for ( const char *pc = subject; *pc; pc++, len++ )
   {
      if ( islower(*pc) )
      {
         // not only caps
         return false;
      }
   }

   // it is possible to have a short subjectentirely capitalized in a
   // legitime message if it short enough (e.g. "USA" or "HI"...) but if it's a
   // long sentence containing only caps, it's probabyl spam
   return len > 8;
}

// check if the subject is of the form "...      foo-12-xyz": spammers seem to
// often tackle a unique alphanumeric tail to the subjects of their messages
static bool CheckSubjectForJunkAtEnd(const String& subject)
{
   // we look for at least that many consecutive spaces before starting looking
   // for a junk tail
   static const size_t NUM_SPACES = 6;

   // and the tail should have at least that many symbols
   static const size_t LEN_TAIL = 4;

   // and the sumb of both should be at least equal to this (very short tails
   // without many preceding spaces could occur in a legal message)
   static const size_t LEN_SPACES_AND_TAIL = 15;

   const wxChar *p = wxStrstr(subject, String(_T(' '), NUM_SPACES));
   if ( !p )
      return false;

   // skip all whitespace
   const wxChar * const startSpaces = p;
   p += NUM_SPACES;
   while ( isspace(*p) )
      p++;

   // start of the tail
   const wxChar * const startTail = p;
   while ( *p && (wxIsalnum(*p) || wxStrchr(_T("-_{}+"), *p)) )
      p++;

   // only junk (and enough of it) until the end?
   return !*p &&
            (size_t)(p - startTail) >= LEN_TAIL &&
               (size_t)(p - startSpaces) >= LEN_SPACES_AND_TAIL;
}

// check the given MIME part and all of its children for Korean charset, return
// true if any of them has it
static bool CheckMimePartForKoreanCSet(const MimePart *part)
{
   while ( part )
   {
      if ( CheckMimePartForKoreanCSet(part->GetNested()) )
         return true;

      String cset = part->GetParam("charset").Lower();
      if ( cset == "ks_c_5601-1987" || cset == "euc-kr" )
      {
         return true;
      }

      part = part->GetNext();
   }

   return false;
}

// check the value of X-Spam-Status header and return true if we believe it
// indicates that this is a spam
static bool CheckXSpamStatus(const String& value)
{
   // SpamAssassin adds header "X-Spam-Status: Yes" for the messages it
   // believes to be spams, so simply check if the header value looks like this
   return value.Lower().StartsWith("yes");
}

// check the value of X-Authentication-Warning header and return true if we
// believe it indicates that this is a spam
static bool CheckXAuthWarning(const String& value)
{
   // check for "^.*Host.+claimed to be.+$" regex manually
   static const char *HOST_STRING = "Host ";
   static const char *CLAIMED_STRING = "claimed to be ";

   const char *pc = strstr(value, HOST_STRING);
   if ( !pc )
      return false;

   const char *pc2 = strstr(pc + 1, CLAIMED_STRING);
   if ( !pc2 )
      return false;

   // there seems to be a few common misconfiguration problems which
   // result in generation of X-Authentication-Warning headers like this
   //
   //    Host host.domain.com [ip] claimed to be host
   //
   // or like that:
   //
   //    Host alias.domain.com [ip] claimed to be mail.domain.com
   //
   // which are, of course, harmless, i.e. don't mean that this is spam, so we
   // try to filter them out

   // skip to the hostnames
   pc += strlen(HOST_STRING);
   pc2 += strlen(CLAIMED_STRING);

   // check if the host names are equal (case 1 above)
   const char *host1 = pc,
              *host2 = pc2;

   bool hostsEqual = true;
   while ( *host1 != '.' && *host2 != '\r' )
   {
      if ( *host1++ != *host2++ )
      {
         // hosts don't match
         hostsEqual = false;

         break;
      }
   }

   // did the host names match?
   if ( hostsEqual && *host1 == '.' && *host2 == '\r' )
   {
      // they did, ignore this warning
      return false;
   }

   // check if the domains match
   const char *domain1 = strchr(pc, '.'),
              *domain2 = strchr(pc2, '.');

   if ( !domain1 || !domain2 )
   {
      // no domain at all -- this looks suspicious
      return true;
   }

   while ( *domain1 != ' ' && *domain2 != '\r' )
   {
      if ( *domain1++ != *domain2++ )
      {
         // domain differ as well, seems like a valid warning
         return true;
      }
   }

   // did the domains match fully?
   return *domain1 == ' ' && *domain2 == '\r';
}

// check the value of Received headers and return true if we believe they
// indicate that this is a spam
static bool CheckReceivedHeaders(const String& value)
{
   static const char *FROM_UNKNOWN_STR = "from unknown";
   static const size_t FROM_UNKNOWN_LEN = strlen(FROM_UNKNOWN_STR);

   // "Received: from unknown" is very suspicious, especially if it appears in
   // the last "Received:" header, i.e. the first one in chronological order
   const char *pc = strstr(value, FROM_UNKNOWN_STR);
   if ( !pc )
      return false;

   // it should be in the beginning of the header line
   if ( pc != value.c_str() && *(pc - 1) != '\n' )
      return false;

   // and it should be the last Received: header -- unfortunately there are
   // sometimes "from unknown" warnings for legitimate mail so we try to reduce
   // the number of false positives by checking the last header only as this is
   // normally the only one which the spammer directly controls
   pc = strstr(pc + FROM_UNKNOWN_LEN, "\r\n");
   if ( pc )
   {
      // skip the line end
      pc += 2;
   }

   // and there shouldn't be any more lines after this one
   return pc && *pc == '\0';
}

// check if we have an HTML-only message
static bool CheckForHTMLOnly(const Message *msg)
{
   const MimePart *part = msg->GetTopMimePart();
   if ( !part )
   {
      // no MIME info at all?
      return false;
   }

   // we accept the multipart/alternative messages with a text/plain (but see
   // below) and a text/html part but not the top level text/html messages
   //
   // the things are further complicated by the fact that some spammers send
   // HTML messages without MIME-Version header which results in them
   // [correctly] not being recognizad as MIME by c-client at all and so they
   // have the default type of text/plain and we have to check for this
   // (common) case specially
   const MimeType type = part->GetType();
   switch ( type.GetPrimary() )
   {
      case MimeType::TEXT:
         {
            String subtype = type.GetSubType();

            if ( subtype == "PLAIN" )
            {
               // check if it was really in the message or returned by c-client in
               // absence of MIME-Version
               String value;
               if ( !msg->GetHeaderLine("MIME-Version", value) )
               {
                  if ( msg->GetHeaderLine("Content-Type", value) )
                  {
                     if ( strstr(value.MakeLower(), "text/html") )
                        return true;
                  }
               }
            }
            else if ( subtype == "HTML" )
            {
               return true;
            }
         }
         break;

      case MimeType::MULTIPART:
         // if we have only one subpart and it's HTML, flag it as spam
         {
            const MimePart *subpart1 = part->GetNested();
            if ( subpart1 &&
                  !subpart1->GetNext() &&
                     subpart1->GetType() == _T("TEXT/HTML") )
            {
               return true;
            }

            if ( type.GetSubType() == _T("ALTERNATIVE") )
            {
               // although multipart/alternative messages with a text/plain and a
               // text/html type are legal, spammers sometimes send them with an
               // empty text part -- which is not
               //
               // note that a text line with 2 blank lines is going to have
               // size 4 and as no valid message is probably going to have just
               // 2 letters, let's consider messages with small size empty too
               if ( subpart1->GetType() == _T("TEXT/PLAIN") &&
                           subpart1->GetSize() < 5 )
               {
                  const MimePart *subpart2 = subpart1->GetNext();
                  if ( subpart2 &&
                        !subpart2->GetNext() &&
                           subpart2->GetType() == _T("TEXT/HTML") )
                  {
                     // multipart/alternative message with text/plain and html
                     // subparts [only] and the text part is empty -- junk
                     return true;
                  }
               }
            }
         }
         break;

      default:
         // it's a MIME message but of non TEXT type
         ;
   }

   return false;
}

// check if we have a message with "suspicious" MIME structure
static bool CheckForSuspiciousMIME(const Message *msg)
{
   // we consider multipart messages with only one part suspicious: although
   // formally valid, there is really no legitimate reason to send them
   //
   // the sole exception is for multipart/mixed messages with a single part:
   // although they are weird too, there *is* some reason to send them, namely
   // to protect the Base 64 encoded parts from being corrupted by the mailing
   // list trailers and although no client known to me does this currently it
   // may happen in the future
   const MimePart *part = msg->GetTopMimePart();
   if ( !part ||
            part->GetType().GetPrimary() != MimeType::MULTIPART ||
               part->GetType().GetSubType() == _T("MIXED") )
   {
      // either not multipart at all or multipart/mixed which we don't check
      return false;
   }

   // only return true if we have exactly one subpart
   const MimePart *subpart = part->GetNested();

   return subpart && !subpart->GetNext();
}

static Value func_isspam(ArgList *args, FilterRuleImpl *p)
{
   String arg;
   switch ( args->Count() )
   {
      case 0:
         // for compatibility: before, this function didn't take any arguments
         // and only checked the RBL
         arg = "rbl";
         break;

      case 1:
         arg = args->GetArg(0)->Evaluate().GetString();
         break;

      default:
         return Value();
   }

   wxArrayString tests = strutil_restore_array(arg);
   size_t count = tests.GetCount();

   if ( !count )
   {
      wxLogWarning(
         _("No SPAM tests configured, please edit this filter rule."));

      return Value();
   }

   Message_obj msg = p->GetMessage();
   if ( !msg )
      return false;

   String value;

   gs_spamTest.clear();
   for ( size_t n = 0; n < count && gs_spamTest.empty(); n++ )
   {
      const wxString& test = tests[n];
      if ( test == SPAM_TEST_SUBJ8BIT )
      {
         if ( CheckSubjectFor8Bit(msg->Subject()) )
            gs_spamTest = _("8 bit subject");
      }
      else if ( test == SPAM_TEST_SUBJCAPS )
      {
         if ( CheckSubjectForCapitals(msg->Subject()) )
            gs_spamTest = _("only caps in subject");
      }
      else if ( test == SPAM_TEST_SUBJENDJUNK )
      {
         if ( CheckSubjectForJunkAtEnd(msg->Subject()) )
            gs_spamTest = _("junk at end of subject");
      }
      else if ( test == SPAM_TEST_KOREAN )
      {
         // detect all Korean charsets -- and do it for all MIME parts, not
         // just the top level one
         if ( CheckMimePartForKoreanCSet(msg->GetTopMimePart()) )
            gs_spamTest = _("message in Korean");
      }
      else if ( test == SPAM_TEST_SPAMASSASSIN )
      {
         // SpamAssassin adds header "X-Spam-Status: Yes" to all (probably)
         // detected spams
         if ( msg->GetHeaderLine("X-Spam-Status", value) &&
                  CheckXSpamStatus(value) )
         {
            gs_spamTest = _("tagged by SpamAssassin");
         }
      }
      else if ( test == SPAM_TEST_XAUTHWARN )
      {
         // unfortunately not only spams have this header but we consider that
         // only spammers change their address in such way
         if ( msg->GetHeaderLine("X-Authentication-Warning", value) &&
                  CheckXAuthWarning(value) )
         {
            gs_spamTest = _("contains X-Authentication-Warning");
         }
      }
      else if ( test == SPAM_TEST_RECEIVED )
      {
         if ( msg->GetHeaderLine("Received", value) &&
                  CheckReceivedHeaders(value) )
         {
            gs_spamTest = _("suspicious \"Received:\"");
         }
      }
      else if ( test == SPAM_TEST_MIME )
      {
         if ( CheckForSuspiciousMIME(msg.Get()) )
            gs_spamTest = _("suspicious MIME structure");
      }
      else if ( test == SPAM_TEST_HTML )
      {
         if ( CheckForHTMLOnly(msg.Get()) )
            gs_spamTest = _("pure HTML content");
      }
#ifdef USE_RBL
      else if ( test == SPAM_TEST_RBL )
      {
         msg->GetHeaderLine("Received", value);

         int a,b,c,d;
         String testHeader = value;

         bool rc = false;
         while ( !rc && !testHeader.empty() )
         {
            if(findIP(testHeader, '(', ')', &a, &b, &c, &d))
            {
               for ( int i = 0; gs_RblSites[i]; ++i )
               {
                  if ( CheckRBL(a,b,c,d,gs_RblSites[i]) )
                  {
                     rc = true;
                     break;
                  }
               }
            }
         }

         if ( !rc )
         {
            testHeader = value;
            while ( !rc && !testHeader.empty() )
            {
               if(findIP(testHeader, '[', ']', &a, &b, &c, &d))
               {
                  for ( int i = 0; gs_RblSites[i] ; ++i )
                  {
                     if ( CheckRBL(a,b,c,d,gs_RblSites[i]) )
                     {
                        rc = true;
                        break;
                     }
                  }
               }
            }
         }

         /*FIXME: if it is a hostname, maybe do a DNS lookup first? */

         if ( rc )
            gs_spamTest = _("blacklisted by RBL");
      }
#endif // USE_RBL
      //else: simply ignore unknown tests, don't complain as it would be
      //      too annoying probably
   }

   return !gs_spamTest.empty();
}

static Value func_msgbox(ArgList *args, FilterRuleImpl *p)
{
   ASSERT(args);
   String msg;
   for(size_t i = 0; i < args->Count(); ++i)
      msg << args->GetArg(i)->Evaluate().ToString();
   p->Output(msg);
   return 1;
}

static Value func_log(ArgList *args, FilterRuleImpl *p)
{
   ASSERT(args);
   String msg;
   for(size_t i = 0; i < args->Count(); ++i)
      msg << args->GetArg(i)->Evaluate().ToString();
   p->Log(msg);
   return 1;
}
/* * * * * * * * * * * * * * *
*
* Tests for message contents
*
* * * * * * * * * * * * * */
static Value func_containsi(ArgList *args, FilterRuleImpl *p)
{
   ASSERT(args);
   if(args->Count() != 2)
      return 0;
   const Value v1 = args->GetArg(0)->Evaluate();
   const Value v2 = args->GetArg(1)->Evaluate();
   String haystack = v1.ToString();
   String needle = v2.ToString();
   p->GetInterface()->strutil_tolower(haystack);
   p->GetInterface()->strutil_tolower(needle);
   return haystack.Find(needle) != -1;
}

static Value func_contains(ArgList *args, FilterRuleImpl *p)
{
   ASSERT(args);
   if(args->Count() != 2)
      return 0;
   const Value v1 = args->GetArg(0)->Evaluate();
   const Value v2 = args->GetArg(1)->Evaluate();
   String haystack = v1.ToString();
   String needle = v2.ToString();
   return haystack.Find(needle) != -1;
}

static Value func_match(ArgList *args, FilterRuleImpl *p)
{
   ASSERT(args);
   if(args->Count() != 2)
      return 0;
   const Value v1 = args->GetArg(0)->Evaluate();
   const Value v2 = args->GetArg(1)->Evaluate();
   String haystack = v1.ToString();
   String needle = v2.ToString();
   return haystack == needle;
}

static Value func_matchi(ArgList *args, FilterRuleImpl *p)
{
   ASSERT(args);
   if(args->Count() != 2)
      return 0;
   const Value v1 = args->GetArg(0)->Evaluate();
   const Value v2 = args->GetArg(1)->Evaluate();
   String haystack = v1.ToString();
   String needle = v2.ToString();
   p->GetInterface()->strutil_tolower(haystack);
   p->GetInterface()->strutil_tolower(needle);
   return haystack == needle;
}

static Value DoMatchRegEx(ArgList *args, FilterRuleImpl *p, int flags = 0)
{
   ASSERT(args);
   if(args->Count() != 2)
      return 0;
   const Value v1 = args->GetArg(0)->Evaluate();
   const Value v2 = args->GetArg(1)->Evaluate();
   String haystack = v1.ToString();
   String needle = v2.ToString();
   strutil_RegEx * re = p->GetInterface()->
                           strutil_compileRegEx(needle, flags);
   if(! re) return FALSE;

   // yes, 0, don't use flags here
   bool rc = p->GetInterface()->strutil_matchRegEx(re, haystack, 0);
   p->GetInterface()->strutil_freeRegEx(re);
   return rc;
}

static Value func_matchregex(ArgList *args, FilterRuleImpl *p)
{
   return DoMatchRegEx(args, p);
}

static Value func_matchregexi(ArgList *args, FilterRuleImpl *p)
{
   return DoMatchRegEx(args, p, wxRE_ICASE);
}

#ifdef USE_PYTHON
static Value func_python(ArgList *args, FilterRuleImpl *p)
{
#ifndef TEST        // uses functions not in libraries
   ASSERT(args);
   if(args->Count() != 1)
      return 0;
   Message * msg = p->GetMessage();
   if(! msg)
      return Value("");
   String funcName = args->GetArg(0)->Evaluate().ToString();

   int result = 0;
   bool rc = PyH_CallFunction(funcName,
                              "func_python",
                              p, "Message",
                              "%d", &result,
                              NULL);
   msg->DecRef();
   return rc ? (result != 0) : 0;
#else
   return 0;
#endif
}
#else // !USE_PYTHON
static Value func_python(ArgList *args, FilterRuleImpl *p)
{
   p->Error(_("Python support for filters is not available."));
   return false;
}
#endif // USE_PYTHON/!USE_PYTHON

static Value func_print(ArgList *args, FilterRuleImpl *p)
{
#ifndef TEST        // uses functions not in libraries
   if(args->Count() != 0)
      return Value(0);

   Message * msg = p->GetMessage();
   if(! msg)
      return Value("");

   // FIXME: this can't work like this!!
#if 0
   wxMessageViewFrame *mvf = new wxMessageViewFrame(NULL);
   mvf->Show(FALSE);
   mvf->ShowMessage(msg);
   msg->DecRef();
   bool rc = mvf->GetMessageView()->Print();
   delete mvf;

   return Value(rc);
#else
   return 0;
#endif
#else // TEST
   return 0;
#endif
}
/* * * * * * * * * * * * * * *
*
* Access to message contents
*
* * * * * * * * * * * * * */
static Value func_subject(ArgList *args, FilterRuleImpl *p)
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

static Value func_from(ArgList *args, FilterRuleImpl *p)
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

static Value func_to(ArgList *args, FilterRuleImpl *p)
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

static Value func_recipients(ArgList *args, FilterRuleImpl *p)
{
   String result;

   if ( args->Count() == 0 )
   {
      Message *msg = p->GetMessage();
      if ( msg )
      {
         wxArrayString values = msg->GetHeaderLines(headersRecipients);
         result = strutil_flatten_array(values, ',');

         msg->DecRef();
      }
   }
   //else: error in arg count!

   return Value(result);
}

static Value func_istome(ArgList *args, FilterRuleImpl *p)
{
   Value value = func_recipients(args, p);

   MailFolder_obj mf = p->GetFolder();
   if ( !mf )
      return Value(false);

   if ( p->GetInterface()->contains_own_address(value.GetString(),
                                                mf->GetProfile()) )
   {
      return Value(true);
   }

   // check for List-Id header: this would mean that this message has been sent
   // to a mailing list and so we should let it pass (of course, a smart
   // spammer could always add a bogus List-Id but for now I didn't see this
   // happen)
   Message_obj msg = p->GetMessage();

   if ( msg )
   {
      String value;
      if ( msg->GetHeaderLine("List-Post", value) )
      {
         return Value(true);
      }
   }

   // this message doesn't seem to be addresses to us
   return Value(false);
}

static Value func_hasflag(ArgList *args, FilterRuleImpl *p)
{
   if(args->Count() != 1)
      return Value(false);
   const Value v1 = args->GetArg(0)->Evaluate();
   String flag_str = v1.ToString();
   if(flag_str.Length() != 1)
      return Value(false);
   Message_obj msg = p->GetMessage();
   if(! msg)
      return Value(false);
   int status = msg->GetStatus();

   if      ( flag_str == "U" )   // Unread (result is inverted)
      return Value(status & MailFolder::MSG_STAT_SEEN     ? false : true);
   else if ( flag_str == "D" )   // Deleted
      return Value(status & MailFolder::MSG_STAT_DELETED  ? true  : false);
   else if ( flag_str == "A" )   // Answered
      return Value(status & MailFolder::MSG_STAT_ANSWERED ? true  : false);
   else if ( flag_str == "R" )   // Recent
      return Value(status & MailFolder::MSG_STAT_RECENT   ? true  : false);
// else if ( flag_str == "S" )   // Search match
//    return Value(status & MailFolder::MSG_STAT_SEARCHED ? true  : false);
   else if ( flag_str == "*" )   // Flagged/Important
      return Value(status & MailFolder::MSG_STAT_FLAGGED  ? true  : false);

   return Value(false);
}

static Value func_header(ArgList *args, FilterRuleImpl *p)
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

static Value func_headerline(ArgList *args, FilterRuleImpl *p)
{
   if(args->Count() != 1)
      return Value("");
   const Value v1 = args->GetArg(0)->Evaluate();
   String field = v1.ToString();
   Message * msg = p->GetMessage();
   if(! msg)
      return Value("");
   String result;
   msg->GetHeaderLine(field, result);
   msg->DecRef();
   return Value(result);
}

static Value func_body(ArgList *args, FilterRuleImpl *p)
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

static Value func_text(ArgList *args, FilterRuleImpl *p)
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

static Value func_do_delete(ArgList *args, FilterRuleImpl *p, bool expunge)
{
   if(args->Count() != 0)
      return 0;

   p->SetDeleted();
   if ( expunge )
      p->SetExpunged();

   // we shouldn't evaluate any subsequent filters after deleting the
   // message
   Value v = 1;
   v.Abort();

   return v;
}

static Value func_delete(ArgList *args, FilterRuleImpl *p)
{
   return func_do_delete(args, p, false);
}

static Value func_zap(ArgList *args, FilterRuleImpl *p)
{
   return func_do_delete(args, p, true);
}

static Value func_copytofolder(ArgList *args, FilterRuleImpl *p)
{
   if(args->Count() != 1)
      return 0;

   const Value fn = args->GetArg(0)->Evaluate();
   p->CopyTo(fn.ToString());

   return 1;
}

static Value func_movetofolder(ArgList *args, FilterRuleImpl *p)
{
   Value rc = func_copytofolder(args, p);
   if(rc.ToNumber()) // successful
   {
      ArgList emptyArgs;
      return func_delete(&emptyArgs, p);
   }

   return 0;
}

static Value func_date(ArgList *args, FilterRuleImpl *p)
{
   if(args->Count() != 0)
      return Value(-1);

   Message *msg = p->GetMessage();
   time_t today = msg->GetDate();
   today /= 60 * 60 * 24; // we count in days, not seconds
   msg->DecRef();

   return Value(today);
}

static Value func_now(ArgList *args, FilterRuleImpl *p)
{
   if(args->Count() != 0)
      return Value(-1);
   time_t today = time(NULL) / 60 / 60 / 24;
   return Value(today);
}

static Value func_size(ArgList *args, FilterRuleImpl *p)
{
   if(args->Count() != 0)
      return Value(-1);
   Message *msg = p->GetMessage();
   unsigned long size = msg->GetSize();
   msg->DecRef();
   return Value(size / 1024); // return KiloBytes
}

static Value func_score(ArgList *args, FilterRuleImpl *p)
{
   if(args->Count() != 0)
      return Value(0);

   int score = 0;
#ifdef USE_HEADER_SCORE
   MailFolder_obj mf = p->GetFolder();
   if ( mf )
   {
      HeaderInfoList_obj hil = mf->GetHeaders();
      if(hil)
      {
         Message_obj msg = p->GetMessage();
         if ( msg )
         {
            score = hil->GetEntryUId( msg->GetUId() )->GetScore();
         }
      }
   }
#endif // USE_HEADER_SCORE

   return Value(score);
}

static Value func_setscore(ArgList *args, FilterRuleImpl *p)
{
   if(args->Count() != 1)
      return Value(-1);

   const Value d = args->GetArg(0)->Evaluate();

#ifdef USE_HEADER_SCORE
   MailFolder_obj mf = p->GetFolder();
   if ( mf )
   {
      HeaderInfoList_obj hil = mf->GetHeaders();
      if(hil)
      {
         Message_obj msg = p->GetMessage();
         if ( msg )
         {
            // MAC: I'd rather use ->SetScore() or something similar,
            // MAC: but I couldn't find any of the scoring methods.
            int score = hil->GetEntryUId( msg->GetUId() )->GetScore();
            hil->GetEntryUId( msg->GetUId() )->AddScore(d.ToNumber() - score);
         }
      }
   }
#endif // USE_HEADER_SCORE

   return Value(0);
}

static Value func_addscore(ArgList *args, FilterRuleImpl *p)
{
   if(args->Count() != 1)
      return Value(-1);

   const Value d = args->GetArg(0)->Evaluate();

#ifdef USE_HEADER_SCORE
   MailFolder_obj mf = p->GetFolder();
   if ( mf )
   {
      HeaderInfoList_obj hil = mf->GetHeaders();
      if(hil)
      {
         Message_obj msg = p->GetMessage();
         if ( msg )
         {
            hil->GetEntryUId( msg->GetUId() )->AddScore(d.ToNumber());
         }
      }
   }
#endif // USE_HEADER_SCORE

   return Value(0);
}

static Value func_setcolour(ArgList *args, FilterRuleImpl *p)
{
   if(args->Count() != 1)
      return Value(-1);

#ifdef USE_HEADER_COLOUR
   const Value c = args->GetArg(0)->Evaluate();
   String col = c.ToString();
   MailFolder_obj mf = p->GetFolder();
   if ( mf )
   {
      HeaderInfoList_obj hil = mf->GetHeaders();
      if(hil)
      {
         Message_obj msg = p->GetMessage();
         if ( msg )
         {
            hil->GetEntryUId( msg->GetUId() )->SetColour(col);
         }
      }
   }
#endif // USE_HEADER_COLOUR

   return Value(0);
}

static Value func_do_setflag(ArgList *args, FilterRuleImpl *p, bool set)
{
   if(args->Count() != 1)
      return Value(false);
   const Value v1 = args->GetArg(0)->Evaluate();
   String flag_str = v1.ToString();
   if(flag_str.Length() != 1)
      return Value(false);

   int flag = 0;
   if      ( flag_str == "U" )   // Unread, inverse of actual flag (SEEN)
   {
      flag = MailFolder::MSG_STAT_SEEN;
      set  = ! set;
   }
   else if ( flag_str == "D" )   // Deleted
      flag = MailFolder::MSG_STAT_DELETED;
   else if ( flag_str == "A" )   // Answered
      flag = MailFolder::MSG_STAT_ANSWERED;
   // The Recent flag is not changable.
   // else if ( flag_str == "R" )   // Recent
   //   flag = MailFolder::MSG_STAT_RECENT;
// else if ( flag_str == "S" )   // Search match
//    flag = MailFolder::MSG_STAT_SEARCHED;
   else if ( flag_str == "*" )   // Flagged/Important
      flag = MailFolder::MSG_STAT_FLAGGED;
   else
      return Value(false);

   // Set or clear the selected flag
   MailFolder_obj mf = p->GetFolder();
   if ( ! mf )
      return Value(false);
   return Value(mf->SetMessageFlag(p->GetMessageUId(), flag, set));
}

static Value func_setflag(ArgList *args, FilterRuleImpl *p)
{
   return func_do_setflag(args, p, true);
}

static Value func_clearflag(ArgList *args, FilterRuleImpl *p)
{
   return func_do_setflag(args, p, false);
}


/* * * * * * * * * * * * * * *
*
* Folder functionality
*
* * * * * * * * * * * * * */
static Value func_expunge(ArgList *args, FilterRuleImpl *p)
{
   if(args->Count() != 0)
      return Value(0);

   p->SetExpunged();

   return 1;
}

#ifdef TEST

/* * * * * * * * * * * * * * *
*
* Testing hacks
*
* * * * * * * * * * * * * */
static Value func_nargs(ArgList *args, FilterRuleImpl *p)
{
   return args->Count();
}
static Value func_arg(ArgList *args, FilterRuleImpl *p)
{
   if (args->Count() != 1)
      return 0;
   return args->GetArg(0)->Evaluate();
}

#endif // TEST

static const FunctionList *
BuiltinFunctions(void)
{
   static FunctionList s_builtinFuncList;
   if ( s_builtinFuncList.empty() )
   {
      #define Define(name, fn) \
         s_builtinFuncList.push_back(FunctionDefinition(name, fn))

         Define("message", func_msgbox);
         Define("log", func_log);
         Define("match", func_match);
         Define("contains", func_contains);
         Define("matchi", func_matchi);
         Define("containsi", func_containsi);
         Define("matchregex", func_matchregex);
         Define("subject", func_subject);
         Define("to", func_to);
         Define("recipients", func_recipients);
         Define("headerline", func_headerline);
         Define("from", func_from);
         Define("hasflag", func_hasflag);
         Define("header", func_header);
         Define("body", func_body);
         Define("text", func_text);
         Define("delete", func_delete);
         Define("zap", func_zap);
         Define("copy", func_copytofolder);
         Define("move", func_movetofolder);
         Define("print", func_print);
         Define("date", func_date);
         Define("size", func_size);
         Define("now", func_now);
         Define("isspam", func_isspam);
         Define("expunge", func_expunge);
         Define("python", func_python);
         Define("matchregexi", func_matchregexi);
         Define("setcolour", func_setcolour);
         Define("score", func_score);
         Define("setscore", func_setscore);
         Define("addscore", func_addscore);
         Define("istome", func_istome);
         Define("setflag", func_setflag);
         Define("clearflag", func_clearflag);
#ifdef TEST
         Define("nargs", func_nargs);
         Define("arg", func_arg);
#endif

#undef Define
   }

   return &s_builtinFuncList;
}

// ----------------------------------------------------------------------------
// FilterRuleImpl - the class representing a program
// ----------------------------------------------------------------------------

// TODO: this function should be factorized into several smaller ones
int
FilterRuleImpl::Apply(MailFolder *mf, UIdArray& msgs)
{
   // note the use of explicit scope qualifier to avoid conflicts with our
   // Error() member function
   CHECK( mf, FilterRule::Error, _T("FilterRule::Apply(): NULL parameter") );

   // no error yet
   int rc = 0;

#ifndef TEST
   if ( m_Program )
   {
      // remember the mail folder we operate on
      m_MailFolder = mf;
      m_MailFolder->IncRef();

      // the array holding the indices of the messages deleted by filters
      wxArrayLong indicesDeleted;

      // show the progress dialog while filtering
      size_t count = msgs.GetCount();

      wxFrame *frame = m_MailFolder->GetInteractiveFrame();
      MProgressDialog *pd;
      if ( frame )
      {
         pd = new MProgressDialog
                  (
                     wxString::Format
                     (
                        _("Filtering %u messages in folder '%s'..."),
                        count, m_MailFolder->GetName().c_str()
                     ),
                     "\n\n",  // must be tall enough for 3 lines
                     2*count,
                     frame,
                     false,   // !"disable parent only"
                     true     // show "cancel" button
                  );
      }
      else // don't show the progress dialog, just log in the status bar
      {
         pd = NULL;
      }

      // first decide what should we do with the messages: fill the arrays with
      // the operations to perform and the destination folder if the operation
      // involves copying the message
      wxArrayInt operations;
      wxArrayString destinations;

      bool doExpunge = false;

      // the text for the progress dialog (verbose) and for the log (terse)
      String textPD,
             textLog;
      size_t idx;
      for ( idx = 0; idx < count; idx++ )
      {
         // do it first so that the arrays have the right size even if we hit
         // "continue" below
         operations.Add(None);
         destinations.Add("");

         m_MessageUId = msgs[idx];

         if ( m_MessageUId == UID_ILLEGAL )
         {
            FAIL_MSG( _T("invalid UID in FilterRule::Apply()!") );

            continue;
         }

         m_MailMessage = m_MailFolder->GetMessage(m_MessageUId);

         if ( !m_MailMessage )
         {
            // maybe another session deleted it?
            wxLogDebug(
               _T("Filter error: message with UID %ld in folder '%s' doesn't exist any more."),
                       m_MessageUId, m_MailFolder->GetName().c_str());
            continue;
         }

         // update the GUI

         // TODO: make the format of the string inside the parentheses
         //       configurable

         String subject = MailFolder::DecodeHeader(m_MailMessage->Subject()),
                from = MailFolder::DecodeHeader(m_MailMessage->From());

         textLog.Printf(_("Filtering message %u/%u"), idx + 1, count);

         // make a multiline label for the progress dialog and a more concise
         // one for the status bar
         if ( pd )
         {
            textPD.clear();
            textPD << textLog << '\n'
                   << _("From: ") << from << '\n'
                   << _("Subject: ") << subject;
         }

         textLog << " (";

         if ( !from.empty() )
         {
            textLog << _("from ") << from << ' ';
         }

         if ( !subject.empty() )
         {
            textLog << _("about '") << subject << '\'';
         }
         else
         {
            textLog << _("without subject");
         }

         textLog << ')';

         // and use both of them
         if ( pd )
         {
            if ( !pd->Update(idx, textPD) )
            {
               // cancelled by user
               m_MailMessage->DecRef();

               break;
            }
         }
         else // no progress dialog
         {
            // don't pass it as the first argument because the string might
            // contain '%' characters!
            wxLogStatus("%s", textLog.c_str());
         }

         // do some heuristic optimizations: if our program contains requests
         // for the entire message header, get it first because like this it
         // will be cached and all other requests will use it - otherwise we'd
         // have to make several trips to server to get a few separate fields
         // first and only then retrieve the header
         //
         // in the same way, retrieve all the recipients if we need them anyhow
         // before retrieving "To" &c
         if ( m_hasHeaderFunc )
         {
            if ( m_hasToFunc || m_hasRcptFunc || m_hasHdrLineFunc )
            {
               // pre-retrieve the whole header
               (void)m_MailMessage->GetHeader();
            }
         }
         else if ( m_hasRcptFunc )
         {
            if ( m_hasToFunc )
            {
               // pre-fetch the recipient headers which include "To"
               (void)m_MailMessage->GetHeaderLines(headersRecipients);
            }
         }

         // reset the result flags
         m_operation = None;

         const Value retval = m_Program->Evaluate();

         // remember the result
         operations[idx] = m_operation;
         destinations[idx] = m_copiedTo;

         if ( m_operation & Expunge )
         {
            doExpunge = true;
         }

         // and show the result in the progress dialog
         String textExtra = " - ";

         if ( !retval.IsNumber() )
         {
            textExtra << _("error!");

            rc |= FilterRule::Error;
         }
         else // filter executed ok
         {
            // if it was caught as a spam, tell the user why do we think so
            if ( !gs_spamTest.empty() )
            {
               textExtra += String::Format
                                    (
                                       _("recognized as spam (%s); "),
                                       gs_spamTest.c_str()
                                    );
               gs_spamTest.clear();
            }

            bool wasDeleted = (m_operation & Deleted) != 0;
            if ( !m_copiedTo.empty() )
            {
               textExtra << (wasDeleted ? _("moved to ") : _("copied to "))
                         << m_copiedTo;

               m_copiedTo.clear();
            }
            else if ( wasDeleted )
            {
               textExtra << _("deleted");
            }
            else // not moved/copied/deleted
            {
               textExtra << _("done");
            }
         }

         textLog += textExtra;

         m_MailMessage->DecRef();

         if ( pd )
         {
            if ( !pd->Update(idx, textPD + textExtra) )
            {
               // cancelled by user
               break;
            }

            // show it in the log window so that the user knows what has
            // happened to the messages
            //
            // NB: textLog may contain '%'s itself, so don't let it be
            //     interpreted as a format string
            wxLogGeneric(M_LOG_WINONLY, "%s", textLog.c_str());
         }
         else // no progress dialog
         {
            // see comment above
            wxLogStatus("%s", textLog.c_str());
         }
      }

      // check if Cancel wasn't pressed (we'd exit the loop above by break then)
      wxString pdExecText = _("Executing filter actions...");
      if ( idx == count &&
           (!pd || pd->Update(count, pdExecText)) )
      {
         UIdArray uidsToDelete;
         wxArrayLong indicesDeleted;

         for ( idx = 0; idx < count; idx++ )
         {
            // note that our progress meter may accelerate towards the end as
            // we may copy more than one message initially - but it's ok, it's
            // better than slowing down towards the end, the users really hate
            // this!

            if ( pd && !pd->Update(count + idx) )
            {
               // cancelled by user
               break;
            }

            if ( operations[idx] & Copy )
            {
               // copy all messages we are copying to this destination at once
               UIdArray uidsToCopy;
               uidsToCopy.Add(msgs[idx]);

               String dst = destinations[idx];

               for ( size_t n = idx + 1; n < count; n++ )
               {
                  if ( (operations[n] & Copy) && destinations[n] == dst )
                  {
                     uidsToCopy.Add(msgs[n]);

                     // don't try to copy it when we reach it
                     operations[n] &= ~Copy;
                  }
               }

               if ( pd )
               {
                  pd->Update(count + idx, pdExecText + '\n' +
                             wxString::Format(_("Copying messages to '%s'..."),
                                           dst.c_str()));
               }

               if ( !m_MailFolder->SaveMessages(&uidsToCopy, dst) )
               {
                  rc |= FilterRule::Error;
               }
            }

            if ( operations[idx] & Delete )
            {
               indicesDeleted.Add(idx);
               uidsToDelete.Add(msgs[idx]);
            }
         }

         // again, stop right now if we were cancelled
         if ( idx == count )
         {
            if ( !uidsToDelete.IsEmpty() )
            {
               if ( pd )
               {
                  pd->Update(2*count, pdExecText + '\n' +
                            wxString::Format(_("Deleting moved messages...")));
               }

               if ( !m_MailFolder->DeleteMessages(&uidsToDelete) )
               {
                  rc |= FilterRule::Error;
               }
               else // deleted successfully
               {
                  rc |= FilterRule::Deleted;

                  // remove the deleted messages from msgs
                  // iterate from end because otherwise the indices would shift
                  // while we traverse them
                  size_t count = indicesDeleted.GetCount();
                  for ( size_t n = count; n > 0; n-- )
                  {
                     msgs.RemoveAt(indicesDeleted[n - 1]);
                  }
               }
            }

            // actual expunging will be done by the calling code
            if ( doExpunge )
            {
               rc |= FilterRule::Expunged;
            }
         }
      }
      //else: cancelled by user

      delete pd;

      m_MailFolder->DecRef();
      m_MailFolder = NULL;
   }
#endif // !TEST

   return rc;
}

FilterRuleImpl::FilterRuleImpl(const String &filterrule,
                               MInterface *minterface,
                               MModule_Filters *mod)
              : m_FilterModule(mod),
                m_MInterface(minterface)
{
#ifndef TEST
   // we cannot allow the module to disappear while we exist
   m_FilterModule->IncRef();
#endif

   m_hasToFunc =
   m_hasRcptFunc =
   m_hasHdrLineFunc =
   m_hasHeaderFunc = false;

   m_Program = Parse(filterrule);
   m_MessageUId = UID_ILLEGAL;
   m_MailMessage = NULL;
   m_MailFolder = NULL;
}

FilterRuleImpl::~FilterRuleImpl()
{
   SafeDecRef(m_MailFolder);
   delete m_Program;
#ifndef TEST
   m_FilterModule->DecRef();
#endif
}

// ----------------------------------------------------------------------------
// MModule_FiltersImpl - the module
// ----------------------------------------------------------------------------

#ifdef DEBUG
void FilterRuleImpl::Debug(void)
{
   Output(m_Program->Debug());
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

// ----------------------------------------------------------------------------
// Testing code from now on
// ----------------------------------------------------------------------------

#ifdef TEST        // test suite
#include "../classes/MObject.cpp"        // Gross hack...
class MyParser : public FilterRuleImpl
{
public:
   MyParser(String s, bool r = false)
      : FilterRuleImpl(s, 0, 0)
      { m_reject = r; }
   virtual void Error(const String &error);
   String CharLeft() { return FilterRuleImpl::CharLeft(); }
   String CharMid() { return FilterRuleImpl::CharMid(); }
private:
   bool m_reject;
};
void
MyParser::Error(const String &error)
{
   if (m_reject == false)
      printf(">>>%s\n", error.c_str());
}

void
Rejected(MyParser &p)
{
   printf("Rejected `%s<ERROR>%s'\n",
           p.CharLeft().c_str(),
           p.CharMid().c_str());
}

int        // test whether the string is rejected by the parser
TestExprFail(const char *s)
{
   MyParser p(s, true);
   const SyntaxNode *exp = p.ParseExpression();
   if (exp == NULL)
   {
      Rejected(p);
      return 0;
   }
   const Value v = exp->Evaluate();
   delete exp;
   printf("`%s' was accepted and evaluated as %ld\n", s, v.ToNumber());
   return 1;
}

int        // test whether the string is accepted by the parser
TestExpr(int arg, const char *s)
{
   MyParser p(s);
   const SyntaxNode *exp = p.ParseExpression();
   if (exp == NULL)
   {
      Rejected(p);
      return 1;
   }
   const Value v = exp->Evaluate();
   delete exp;
   if (v.ToNumber() != arg)
   {
      printf("`%s' was %ld instead of %d\n", s, v.ToNumber(), arg);
      return 1;
   }
   return 0;
}

int        // test whether the pgm is rejected by the parser
TestReject(const char *s)
{
   MyParser p(s, true);
   const SyntaxNode *pgm = p.ParseProgram();
   if (pgm == NULL)
   {
      Rejected(p);
      return 0;
   }
   const Value v = pgm->Evaluate();
   delete pgm;
   printf("`%s' was accepted and evaluated as %ld\n", s, v.ToNumber());
   return 1;
}

int        // test whether the pgm is accepted by the parser
TestAccept(const char *s)
{
   MyParser p(s);
   const SyntaxNode *pgm = p.ParseProgram();
   if (pgm == NULL)
   {
      Rejected(p);
      return 1;
   }
   delete pgm;
   return 0;
}

int        // test whether the pgm is accepted by the parser
TestPgm(int arg, const char *s)
{
   MyParser p(s);
   const SyntaxNode *pgm = p.ParseProgram();
   if (pgm == NULL)
   {
      Rejected(p);
      return 1;
   }
   const Value v = pgm->Evaluate();
   delete pgm;
   if (v.ToNumber() != arg)
   {
      printf("`%s' was %ld instead of %d\n", s, v.ToNumber(), arg);
      return 1;
   }
   return 0;
}

int
main(void)
{
   int errs = 0;
   for (;;)
   {
      int c = getchar();
      if (c == EOF)
         break;
      if (c == '\n')
         continue;
      if (c == '#')
      {
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
      if (cmd == "expr")
      {
         long val;
         if (opt == "reject")
            errs += TestExprFail(exp);
         else if (opt.ToLong(&val))
            errs += TestExpr(val, exp);
         else
            printf("Unknown option `%s' to expr command\n",
               opt.c_str());
      }
      else if (cmd == "pgm")
      {
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
      }
      else
         printf("Unknown command `%s'\n", cmd.c_str());
   }

   if (errs > 0)
      printf("%d errors found\n", errs);

   return errs != 0;
}
#endif // TEST
