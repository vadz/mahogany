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

#include <wx/regex.h>   // wxRegEx::Flags

// for the func_print() function:
#include "gui/wxMessageView.h"

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
extern "C" {
   typedef Value (* FunctionPointer)(ArgList *args, FilterRuleImpl *p);
};

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
   virtual int Apply(MailFolder *mf, UIdType uid,
                     bool *changeflag);
   virtual int Apply(MailFolder *folder, bool NewOnly,
                     bool *changeflag);
   virtual int Apply(MailFolder *folder,
                     UIdArray msgs,
                     bool ignoreDeleted,
                     bool *changeflag);
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

   /// get the string to display in the status bar
   String GetStatusString(Message *msg) const;

   /// common code for the two Apply() functions
   int ApplyCommonCode(MailFolder *folder,
                       UIdArray *msgs,
                       bool newOnly,
                       bool ignoreDeleted,
                       bool *changeflag);

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
   /// Set the message and folder to operate on:
   void SetMessage(MailFolder *folder = NULL, UIdType uid = UID_ILLEGAL)
      {
         SafeDecRef(m_MailFolder);
         m_MailFolder = folder;
         SafeIncRef(m_MailFolder);
         m_MessageUId = uid;

         m_expungedMsgs = FALSE;
      }
   /// Has filter rule caused a change to the folder/message?
   bool GetChanged(void) const
      {
         ASSERT(m_MailFolder);

         // m_deletedMsgs doesn't count here as if the messages were only
         // deleted we don't need to rebuild the folder listing
         return m_expungedMsgs;
      }
   /// Obtain the mailfolder to operate on:
   MailFolder * GetFolder(void)
      { SafeIncRef(m_MailFolder); return m_MailFolder; }
   /// Obtain the message UId to operate on:
   UIdType GetMessageUId(void) { return m_MessageUId; }
   /// Obtain the message itself:
   Message * GetMessage(void)
      {
         return m_MailFolder ?
            m_MailFolder->GetMessage(m_MessageUId) :
            NULL ;
      }
   //@}
   MInterface * GetInterface(void) { return m_MInterface; }

   /// called if messages were expunged from folder
   void SetExpunged() { m_expungedMsgs = true; m_deletedMsgs.Empty(); }

   /// called by func_delete() to tell us that a message was deleted
   void SetDeleted() { m_deletedMsgs.Add(m_msgno); }

   /// is the current message marked as deleted?
   bool IsMsgDeleted() const
      { return m_deletedMsgs.Index(m_msgno) != wxNOT_FOUND; }

   /// called by func_copytofolder()
   void SetCopiedTo(const String& copiedTo) { m_copiedTo = copiedTo; }

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

   UIdType m_MessageUId;
   MailFolder *m_MailFolder;

   // the number of the message being processed
   size_t m_msgno;

   // and the total number of messages
   size_t m_msgnoMax;

   // this flag is set during evalutation if any filter calls Expunge()
   bool m_expungedMsgs;

   // this array contains the msgnos of the messages which were deleted from the
   // filter rule action part
   wxArrayLong m_deletedMsgs;

   // the folder the message was copied to or empty
   String m_copiedTo;

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
         if (m_Type == Type_String) {
            int v = 0, k = m_String.length();
            for (int i = 0; i < k; ++i)
            {
               int c = m_String[i];
               if (c < '0' || c > '9')
                  return false;
               v = v*10 + (c - '0');
            }
            Value *that = (Value*)this;
            that->m_Num = v;
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
         String str;
         str.Printf("%ld", m_Num);
         return str;
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
   inline const char *GetName(void) const { return m_Name; }
   inline FunctionPointer GetFPtr(void) const { return m_FunctionPtr; }
private:
   String          m_Name;
   FunctionPointer m_FunctionPtr;
};
M_LIST(FunctionList,FunctionDefinition);

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
   QueryOp(const SyntaxNode *cond, const SyntaxNode *left, const SyntaxNode *right)
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
         else
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
         else
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
      if (name == i->GetName())
         return i.operator->();
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
LeftAssoc(BAnds,BAndOp,Relational,_("Expected expression after bit AND operator"))

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
      default: return NULL;
   }
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
      default: return NULL;
   }
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
      default: return NULL;
   }
}
LeftAssoc(Factor,MulOp,Unary,_("Expected factor after multiply/divide/modulus operator"))

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

#endif

// VZ: VC++ doesn't allow to have extern "C" functions returning a class
#ifndef _MSC_VER
extern "C"
{
#endif // VC++
   static Value func_checkSpam(ArgList *args, FilterRuleImpl *p)
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
            for(int i = 0; gs_RblSites[i] && ! rc ; ++i)
               rc |= CheckRBL(a,b,c,d,gs_RblSites[i]);
         }
      }
      testHeader = received;
      while( (!rc) && (testHeader.Length() > 0) )
      {
         if(findIP(testHeader, '[', ']', &a, &b, &c, &d))
         {
            for(int i = 0; gs_RblSites[i] && ! rc ; ++i)
               rc |= CheckRBL(a,b,c,d,gs_RblSites[i]);
         }
      }

      /*FIXME: if it is a hostname, maybe do a DNS lookup first? */
#endif
      return rc;
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
      return DoMatchRegEx(args, p, wxRegExBase::RE_ICASE);
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
      p->SetExpunged(); // worst case guess
      return rc ? (result != 0) : false;
#else
      return 0;
#endif
   }
#else

   static Value func_python(ArgList *args, FilterRuleImpl *p)
   {
      p->Error(_("Python support for filters is not available."));
      return false;
   }
#endif

   static Value func_print(ArgList *args, FilterRuleImpl *p)
   {
#ifndef TEST        // uses functions not in libraries
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
      for(int i = 0; headers[i]; ++i)
      {
         msg->GetHeaderLine(headers[i], tmp);
         if(tmp[0] && result[0])
            result << ',';
         result << tmp;
      }
      msg->DecRef();
      return Value(result);
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

   static Value func_delete(ArgList *args, FilterRuleImpl *p)
   {
      if(args->Count() != 0)
         return 0;
      MailFolder *mf = p->GetFolder();
      if(! mf) return Value(0);
      UIdType uid = p->GetMessageUId();
      int rc = mf->DeleteMessage(uid);    // without expunging
      mf->DecRef();
      p->SetDeleted();

      Value v(rc);
      if ( rc )
      {
         // we shouldn't evaluate any subsequent filters after deleting the
         // message
         v.Abort();
      }

      return v;
   }

   static Value func_copytofolder(ArgList *args, FilterRuleImpl *p)
   {
#ifndef TEST                // UIdArray not instantiated
      if(args->Count() != 1)
         return 0;
      const Value fn = args->GetArg(0)->Evaluate();
      MailFolder *mf = p->GetFolder();
      UIdType uid = p->GetMessageUId();
      UIdArray ia;
      ia.Add(uid);

      String mfName = fn.ToString();
      bool rc = mf->SaveMessages(&ia, mfName, true, false);
      mf->DecRef();
      if ( rc )
      {
         p->SetCopiedTo(mfName);
      }

      return Value(rc);
#else
      return 0;
#endif
   }

   static Value func_movetofolder(ArgList *args, FilterRuleImpl *p)
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

   static Value func_getstatus(ArgList *args, FilterRuleImpl *p)
   {
      if(args->Count() != 0)
         return Value(-1);
      Message *msg = p->GetMessage();
      int rc = msg->GetStatus();
      msg->DecRef();
      return Value(rc);
   }

   static Value func_date(ArgList *args, FilterRuleImpl *p)
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
      unsigned long size;
      (void) msg->GetStatus(&size);
      msg->DecRef();
      return Value(size / 1024); // return KiloBytes
   }

   static Value func_score(ArgList *args, FilterRuleImpl *p)
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

   static Value func_addscore(ArgList *args, FilterRuleImpl *p)
   {
      if(args->Count() != 1)
         return Value(-1);
      const Value d = args->GetArg(0)->Evaluate();
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
   static Value func_setcolour(ArgList *args, FilterRuleImpl *p)
   {
      if(args->Count() != 1)
         return Value(-1);
      const Value c = args->GetArg(0)->Evaluate();
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
   static Value func_expunge(ArgList *args, FilterRuleImpl *p)
   {
      if(args->Count() != 0)
         return Value(0);
      MailFolder *mf = p->GetFolder();
      mf->ExpungeMessages();
      mf->DecRef();
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

#endif

#ifndef _MSC_VER
};
#endif // VC++

static const FunctionList *
BuiltinFunctions(void)
{
   static FunctionList *defaults;
   if (defaults)
      return defaults;
   defaults = new FunctionList;
#define Define(name, fn) defaults->push_back(FunctionDefinition(name, fn))
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
   Define("header", func_header);
   Define("body", func_body);
   Define("text", func_text);
   Define("delete", func_delete);
   Define("copy", func_copytofolder);
   Define("move", func_movetofolder);
   Define("print", func_print);
   Define("date", func_date);
   Define("size", func_size);
   Define("now", func_now);
   Define("isspam", func_checkSpam);
   Define("expunge", func_expunge);
#ifdef USE_PYTHON
   Define("python", func_python);
#endif
   Define("matchregexi", func_matchregexi);
   Define("setcolour", func_setcolour);
   Define("score", func_score);
   Define("addscore", func_addscore);
#ifdef TEST
   Define("nargs", func_nargs);
   Define("arg", func_arg);
#endif
   return defaults;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * FilterRuleImpl - the class representing a program
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

String FilterRuleImpl::GetStatusString(Message *msg) const
{
   String text;
   if ( msg )
   {
      // only subject is probably enough, no need to have from here
      text.Printf(_("Filtering message %u/%u (%s)"),
                  m_msgno + 1, m_msgnoMax, msg->Subject().c_str());
   }

   return text;
}

int FilterRuleImpl::Apply(MailFolder *mf, UIdType uid, bool *changeflag)
{
   if(! m_Program)
      return 0;

   CHECK( mf, 0, "no mailfolder to apply filter rule to" );
   CHECK( uid != UID_ILLEGAL, 0, "no message to apply filter rules to" );

   mf->IncRef();
   SetMessage(mf, uid);

   // put something into the status bar
   Message *msg = GetMessage();
   String text;
   if ( msg )
   {
      text = GetStatusString(msg);

      GetInterface()->StatusMessage(text + "...");
   }

   const Value rc = m_Program->Evaluate();

   // and now show the result in the status bar too
   if ( msg )
   {
      text += " - ";

      bool wasDeleted = IsMsgDeleted();
      if ( !m_copiedTo.empty() )
      {
         text << (wasDeleted ? _("moved to ") : _("copied to "))
              << m_copiedTo;

         m_copiedTo.clear();
      }
      else if ( wasDeleted )
      {
         text << _("deleted");
      }
      else // not moved/copied/deleted
      {
         text << _("done");
      }

      GetInterface()->StatusMessage(text);

      msg->DecRef();
   }

   if ( changeflag )
      *changeflag = GetChanged();

   // count the messages filtered for the status string message
   m_msgno++;

   SetMessage(NULL);
   mf->DecRef();

   return rc.IsNumber() ? (int) rc.GetNumber() : -1;
}

int
FilterRuleImpl::Apply(MailFolder *mf, bool NewOnly, bool *changeflag)
{
   return ApplyCommonCode(mf, NULL, NewOnly, NewOnly, changeflag);
}

int
FilterRuleImpl::Apply(MailFolder *folder,
                      UIdArray msgs,
                      bool ignoreDeleted,
                      bool *changeflag)
{
#ifndef TEST                // UIdArray not instantiated
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
   if ( ignoreDeleted && (status & MailFolder::MSG_STAT_DELETED) )
      return false;

   if ( newOnly )
   {
      if ( !(status & MailFolder::MSG_STAT_RECENT) ||
            (status & MailFolder::MSG_STAT_SEEN) )
      {
         // either not recent or recent but seen - hence not new
         return false;
      }
   }

   return true;
}

int
FilterRuleImpl::ApplyCommonCode(MailFolder *mf,
                                UIdArray *msgs,
                                bool newOnly,
                                bool ignoreDeleted,
                                bool *changeflag)
{
#ifndef TEST                // UIdArray not instantiated
   CHECK(mf, 0, "no folder for filtering");

   if(! m_Program)
      return 0;

   int rc = true; // no error yet
   mf->IncRef();

   bool changed = FALSE;
   bool changedtemp;
   HeaderInfoList_obj hil = mf->GetHeaders();
   CHECK(hil, 0, "Cannot get header info list");

   if (msgs) // apply to all messages in list
   {
      m_msgnoMax = msgs->Count();
      for(size_t idx = 0; idx < m_msgnoMax; idx++)
      {
         const HeaderInfo * hi = hil->GetEntryUId((*msgs)[idx]);
         ASSERT(hi);
         if(hi && CheckStatusMatch(hi->GetStatus(),
                                   newOnly, ignoreDeleted))
         {
            rc &= Apply(mf, (*msgs)[idx], &changedtemp);
            changed |= changedtemp;
         }
      }
   }
   else // apply to all or all recent messages
   {
      m_msgnoMax = hil->Count();
      for(size_t i = 0; i < m_msgnoMax; i++)
      {
         const HeaderInfo * hi = hil->GetItemByIndex(i);
         ASSERT(hi);
         if(hi && CheckStatusMatch(hi->GetStatus(),
                                   newOnly, ignoreDeleted))
         {
            rc &= Apply(mf, hi->GetUId(), &changedtemp);
            changed |= changedtemp;
         }
      }
   }

   mf->DecRef();

   if ( changeflag )
      *changeflag = changed;

   return rc;
#else
   return 0;
#endif
}

FilterRuleImpl::FilterRuleImpl(const String &filterrule,
                               MInterface *minterface,
                               MModule_Filters *mod)
              : m_FilterModule(mod),
                m_MInterface(minterface),
                m_MailFolder(NULL)
{
#ifndef TEST
   // we cannot allow the module to disappear while we exist
   ASSERT(m_FilterModule); m_FilterModule->IncRef();
#endif
   m_Program = Parse(filterrule);
   m_MessageUId = UID_ILLEGAL;

   m_msgno =
   m_msgnoMax = 0u;
}

FilterRuleImpl::~FilterRuleImpl()
{
   SafeDecRef(m_MailFolder);
   delete m_Program;
#ifndef TEST
   m_FilterModule->DecRef();
#endif
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * MModule_FiltersImpl - the module
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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
#endif
