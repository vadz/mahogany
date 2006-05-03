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
#   include "Mcommon.h"
#   include "Mdefaults.h"
#   include <wx/frame.h>                // for wxFrame
#endif // USE_PCH

#include "modules/Filters.h"
#include "MInterface.h"
#include "SpamFilter.h"

#include "UIdArray.h"
#include "Message.h"

#include "gui/wxMDialogs.h"             // for MProgressDialog

#include <wx/regex.h>   // wxRegEx::Flags

#ifdef USE_PYTHON
#    include "MPython.h"      // Python fix for PyObject / presult
#    include "PythonHelp.h"   // Python fix for PythonCallback
#endif

class MOption;

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_TREAT_AS_JUNK_MAIL_FOLDER;

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
static const wxChar *headersRecipients[] =
{
   _T("To"),
   _T("CC"),
   _T("Bcc"),
   _T("Resent-To"),
   _T("Resent-Cc"),
   _T("Resent-Bcc"),
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
class FilterRuleApply;

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
   int IsIdentifier(const wxChar *s) const
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

   friend class FilterRuleApply;

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

         return String::Format(_T("%ld"), m_Num);
      }

   void Abort() { m_abort = true; }
   bool ShouldAbort() const { return m_abort; }

private:
   void Init() { m_abort = false; }

   Type m_Type;

   // Can't use union here because m_String needs constructor.
   long   m_Num;
   String m_String;

   // if this flag is set, it means that the filter evaluation must be aborted
   // (e.g. because the message was deleted)
   bool m_abort;

   MOBJECT_NAME(Value)
};

// an object of this class is created to apply the given rule to the specified
// messages, it is a sort of "filter runner"
class FilterRuleApply
{
public:
   FilterRuleApply(FilterRuleImpl *parent, UIdArray& msgs);
   ~FilterRuleApply();

   int Run();

private:
   bool LoopEvaluate();
   bool LoopCopy();
   bool DeleteAll();

   void CreateProgressDialog();
   bool GetMessage();
   void GetSenderSubject(String& from, String& subject, bool full);
   bool TreatAsJunk();
   String CreditsCommon();
   String CreditsForDialog();
   String CreditsForStatusBar();
   String ResultsMessage();
   bool UpdateProgressDialog();
   void HeaderCacheHints();
   bool Evaluate();
   bool ProgressCopy();
   bool CopyToOneFolder();
   void CollectForDelete();
   void ProgressDelete();
   void IndicateDeleted();

   static String GetExecuteProgressString(const String& s)
   {
      String msg;
      msg << _("Executing filter actions...") << '\n' << s;
      return msg;
   }

   FilterRuleImpl *const m_parent;
   UIdArray& m_msgs;

   MProgressDialog *m_pd;
   wxArrayInt m_allOperations;
   wxArrayString m_destinations;
   bool m_doExpunge;
   UIdArray m_uidsToDelete;

   // the array holding the indices of the messages deleted by filters
   wxArrayLong m_indicesDeleted;

   // Index of actual message in m_messageList
   size_t m_idx;

   // Result of evaluating filter
   Value m_retval;
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
      { MOcheck(); String s; s.Printf(_T("%ld"), m_value); return s; }
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
   DECLARE_NO_COPY_CLASS(StringConstant)
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
         s << _T("!(") << m_Sn->Debug() << _T(')');
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
         s << _T("!(") << m_Sn->Debug() << _T(')');
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
   FunctionDefinition(const wxChar *name, FunctionPointer fptr)
      : m_Name(name), m_FunctionPtr(fptr)
      { ASSERT(m_Name); ASSERT(m_FunctionPtr); }
   const wxChar *GetName(void) const { return m_Name; }
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
         String temp;
         temp << _T("FunctionCall(") << m_fd->GetName() << _T(")");
         return temp;
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
         String s = _T("(");
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
   virtual const wxChar *OperName(void) const = 0;
   String Debug(void) const
      {
         MOcheck();
         String s = _T("(");
         s << m_Left->Debug() << OperName() << m_Right->Debug() << _T(')');
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
   virtual const wxChar *OperName(void) const { return _T(#oper); } \
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
   virtual const wxChar *OperName(void) const { return _T("&&"); }
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
   virtual const wxChar *OperName(void) const { return _T("||"); }
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
         String s = _T("if(");
         s << m_Condition->Debug() << _T("){");
         if(m_IfBlock)
            s << m_IfBlock->Debug();
         s << _T('}');
         if(m_ElseBlock)
         {
            s << _T("else{")
              << m_ElseBlock->Debug()
              << _T('}');
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
         if ( name == _T("to") )
            m_hasToFunc = true;
         else if ( name == _T("recipients") )
            m_hasRcptFunc = true;
         else if ( name == _T("headerline") )
            m_hasHdrLineFunc = true;
         else if ( name == _T("header") )
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
      const wxChar *cptr = input->c_str()+1;
      String filename;
      while(*cptr && *cptr != '\n' && *cptr != '\r')
         filename += *cptr++;
      while(*cptr && (*cptr == '\r' || *cptr == '\n'))
         cptr++;
      *input = cptr;
      FILE *fp = wxFopen(filename, _T("rt"));
      if(fp)
      {
         fseek(fp, 0, SEEK_END);
         long len = ftell(fp);
         fseek(fp, 0, SEEK_SET);
         if(len > 0)
         {
            wxChar *cp = new wxChar[len + 1];
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
   if(token.IsIdentifier(_T("if")))
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

   ASSERT(token.IsIdentifier(_T("if")));
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
   if(token.IsIdentifier(_T("else")))
   {
      // we must parse the else branch, too:
      NextToken(); // swallow the "else"
      if(token.IsIdentifier(_T("if")))
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
   if(token.IsIdentifier(_T("if")))
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
   else if (t.IsIdentifier(_T("or")))
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
   else if (t.IsIdentifier(_T("and")))
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

static Value func_isspam(ArgList *args, FilterRuleImpl *p)
{
   Message_obj msg(p->GetMessage());
   if ( !msg )
      return false;

   wxArrayString params;
   if ( args->Count() != 1 )
      return 0;

   const wxString param(args->GetArg(0)->Evaluate().GetString());
   gs_spamTest.clear();

   return SpamFilter::CheckIfSpam(*msg, param, &gs_spamTest);
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

static Value func_contains(ArgList *args, FilterRuleImpl *)
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

static Value func_match(ArgList *args, FilterRuleImpl *)
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
   Message_obj msg(p->GetMessage());
   if ( !msg )
      return 0;

   String funcName = args->GetArg(0)->Evaluate().ToString();

   int result = 0;
   if ( !PythonFunction(funcName, msg.Get(), "Message", "i", &result) )
      return 0;

   return result;
#else
   return 0;
#endif
}
#else // !USE_PYTHON
static Value func_python(ArgList *, FilterRuleImpl *p)
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
      return Value(wxEmptyString);

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
      return Value(wxEmptyString);
   Message_obj msg(p->GetMessage());
   if(! msg)
      return Value(wxEmptyString);

   return Value(msg->Subject());
}

static Value func_from(ArgList *args, FilterRuleImpl *p)
{
   if(args->Count() != 0)
      return Value(wxEmptyString);
   Message_obj msg(p->GetMessage());
   if(! msg)
      return Value(wxEmptyString);

   return Value(msg->From());
}

static Value func_to(ArgList *args, FilterRuleImpl *p)
{
   if(args->Count() != 0)
      return Value(wxEmptyString);
   Message_obj msg(p->GetMessage());
   if(! msg)
      return Value(wxEmptyString);

   String tostr;
   msg->GetHeaderLine(_T("To"), tostr);
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

   MailFolder_obj mf(p->GetFolder());
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
   Message_obj msg(p->GetMessage());

   if ( msg )
   {
      String value;
      if ( msg->GetHeaderLine(_T("List-Post"), value) )
      {
         return Value(true);
      }
   }

   // this message doesn't seem to be addresses to us
   return Value(false);
}

static Value func_isfromme(ArgList *args, FilterRuleImpl *p)
{
   Value value = func_from(args, p);

   MailFolder_obj mf(p->GetFolder());
   if ( !mf )
      return Value(false);

   return p->GetInterface()->contains_own_address(value.GetString(),
                                                  mf->GetProfile());
}

static Value func_nop(ArgList *args, FilterRuleImpl *p)
{
   // the sole meaning of this function is to prevent the subsequent actiosn
   // from being evaluated, so do prevent them from running
   Value v = 1;
   v.Abort();

   return v;
}

static Value func_hasflag(ArgList *args, FilterRuleImpl *p)
{
   if(args->Count() != 1)
      return Value(false);
   const Value v1 = args->GetArg(0)->Evaluate();
   String flag_str = v1.ToString();
   if(flag_str.Length() != 1)
      return Value(false);
   Message_obj msg(p->GetMessage());
   if(! msg)
      return Value(false);
   int status = msg->GetStatus();

   if      ( flag_str == _T("U") )   // Unread (result is inverted)
      return Value(status & MailFolder::MSG_STAT_SEEN     ? false : true);
   else if ( flag_str == _T("D") )   // Deleted
      return Value(status & MailFolder::MSG_STAT_DELETED  ? true  : false);
   else if ( flag_str == _T("A") )   // Answered
      return Value(status & MailFolder::MSG_STAT_ANSWERED ? true  : false);
   else if ( flag_str == _T("R") )   // Recent
      return Value(status & MailFolder::MSG_STAT_RECENT   ? true  : false);
// else if ( flag_str == _T("S") )   // Search match
//    return Value(status & MailFolder::MSG_STAT_SEARCHED ? true  : false);
   else if ( flag_str == _T("*") )   // Flagged/Important
      return Value(status & MailFolder::MSG_STAT_FLAGGED  ? true  : false);

   return Value(false);
}

static Value func_header(ArgList *args, FilterRuleImpl *p)
{
   if(args->Count() != 0)
      return Value(wxEmptyString);
   Message * msg = p->GetMessage();
   if(! msg)
      return Value(wxEmptyString);
   String subj = msg->GetHeader();
   msg->DecRef();
   return Value(subj);
}

static Value func_headerline(ArgList *args, FilterRuleImpl *p)
{
   if(args->Count() != 1)
      return Value(wxEmptyString);
   const Value v1 = args->GetArg(0)->Evaluate();
   String field = v1.ToString();
   Message * msg = p->GetMessage();
   if(! msg)
      return Value(wxEmptyString);
   String result;
   msg->GetHeaderLine(field, result);
   msg->DecRef();
   return Value(result);
}

static Value func_body(ArgList *args, FilterRuleImpl *p)
{
   if(args->Count() != 0)
      return Value(wxEmptyString);
   Message * msg = p->GetMessage();
   if(! msg)
      return Value(wxEmptyString);
   String str;
   msg->WriteToString(str, false);
   msg->DecRef();
   return Value(str);
}

static Value func_text(ArgList *args, FilterRuleImpl *p)
{
   if(args->Count() != 0)
      return Value(wxEmptyString);
   Message * msg = p->GetMessage();
   if(! msg)
      return Value(wxEmptyString);
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

static Value func_now(ArgList *args, FilterRuleImpl *)
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

static Value func_score(ArgList *args, FilterRuleImpl *)
{
   if(args->Count() != 0)
      return Value(0);

   int score = 0;
#ifdef USE_HEADER_SCORE
   MailFolder_obj mf(p->GetFolder());
   if ( mf )
   {
      HeaderInfoList_obj hil(mf->GetHeaders());
      if(hil)
      {
         Message_obj msg(p->GetMessage());
         if ( msg )
         {
            score = hil->GetEntryUId( msg->GetUId() )->GetScore();
         }
      }
   }
#endif // USE_HEADER_SCORE

   return Value(score);
}

static Value func_setscore(ArgList *args, FilterRuleImpl *)
{
   if(args->Count() != 1)
      return Value(-1);

   const Value d = args->GetArg(0)->Evaluate();

#ifdef USE_HEADER_SCORE
   MailFolder_obj mf(p->GetFolder());
   if ( mf )
   {
      HeaderInfoList_obj hil(mf->GetHeaders());
      if(hil)
      {
         Message_obj msg(p->GetMessage());
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

static Value func_addscore(ArgList *args, FilterRuleImpl *)
{
   if(args->Count() != 1)
      return Value(-1);

   const Value d = args->GetArg(0)->Evaluate();

#ifdef USE_HEADER_SCORE
   MailFolder_obj mf(p->GetFolder());
   if ( mf )
   {
      HeaderInfoList_obj hil(mf->GetHeaders());
      if(hil)
      {
         Message_obj msg(p->GetMessage());
         if ( msg )
         {
            hil->GetEntryUId( msg->GetUId() )->AddScore(d.ToNumber());
         }
      }
   }
#endif // USE_HEADER_SCORE

   return Value(0);
}

static Value func_setcolour(ArgList *args, FilterRuleImpl *)
{
   if(args->Count() != 1)
      return Value(-1);

#ifdef USE_HEADER_COLOUR
   const Value c = args->GetArg(0)->Evaluate();
   String col = c.ToString();
   MailFolder_obj mf(p->GetFolder());
   if ( mf )
   {
      HeaderInfoList_obj hil(mf->GetHeaders());
      if(hil)
      {
         Message_obj msg(p->GetMessage());
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
   if      ( flag_str == _T("U") )   // Unread, inverse of actual flag (SEEN)
   {
      flag = MailFolder::MSG_STAT_SEEN;
      set  = ! set;
   }
   else if ( flag_str == _T("D") )   // Deleted
      flag = MailFolder::MSG_STAT_DELETED;
   else if ( flag_str == _T("A") )   // Answered
      flag = MailFolder::MSG_STAT_ANSWERED;
   // The Recent flag is not changable.
   // else if ( flag_str == _T("R") )   // Recent
   //   flag = MailFolder::MSG_STAT_RECENT;
// else if ( flag_str == _T("S") )   // Search match
//    flag = MailFolder::MSG_STAT_SEARCHED;
   else if ( flag_str == _T("*") )   // Flagged/Important
      flag = MailFolder::MSG_STAT_FLAGGED;
   else
      return Value(false);

   // Set or clear the selected flag
   MailFolder_obj mf(p->GetFolder());
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

         Define(_T("message"), func_msgbox);
         Define(_T("log"), func_log);
         Define(_T("match"), func_match);
         Define(_T("contains"), func_contains);
         Define(_T("matchi"), func_matchi);
         Define(_T("containsi"), func_containsi);
         Define(_T("matchregex"), func_matchregex);
         Define(_T("subject"), func_subject);
         Define(_T("to"), func_to);
         Define(_T("recipients"), func_recipients);
         Define(_T("headerline"), func_headerline);
         Define(_T("from"), func_from);
         Define(_T("hasflag"), func_hasflag);
         Define(_T("header"), func_header);
         Define(_T("body"), func_body);
         Define(_T("text"), func_text);
         Define(_T("delete"), func_delete);
         Define(_T("zap"), func_zap);
         Define(_T("copy"), func_copytofolder);
         Define(_T("move"), func_movetofolder);
         Define(_T("print"), func_print);
         Define(_T("date"), func_date);
         Define(_T("size"), func_size);
         Define(_T("now"), func_now);
         Define(_T("isspam"), func_isspam);
         Define(_T("expunge"), func_expunge);
         Define(_T("python"), func_python);
         Define(_T("matchregexi"), func_matchregexi);
         Define(_T("setcolour"), func_setcolour);
         Define(_T("score"), func_score);
         Define(_T("setscore"), func_setscore);
         Define(_T("addscore"), func_addscore);
         Define(_T("istome"), func_istome);
         Define(_T("setflag"), func_setflag);
         Define(_T("clearflag"), func_clearflag);
         Define(_T("isfromme"), func_isfromme);
         Define(_T("nop"), func_nop);
#ifdef TEST
         Define(_T("nargs"), func_nargs);
         Define(_T("arg"), func_arg);
#endif

#undef Define
   }

   return &s_builtinFuncList;
}

// ----------------------------------------------------------------------------
// FilterRuleImpl - the class representing a program
// ----------------------------------------------------------------------------

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

      FilterRuleApply apply(this, msgs);

      rc = apply.Run();

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

#ifdef DEBUG

void FilterRuleImpl::Debug(void)
{
   Output(m_Program->Debug());
}

#endif // DEBUG

// ----------------------------------------------------------------------------
// FilterRuleApply
// ----------------------------------------------------------------------------

FilterRuleApply::FilterRuleApply(FilterRuleImpl *parent, UIdArray& msgs)
               : m_parent(parent), m_msgs(msgs)
{
   m_pd = NULL;
   m_doExpunge = false;
}

FilterRuleApply::~FilterRuleApply()
{
   delete m_pd;
}

int
FilterRuleApply::Run()
{
   // no error yet
   int rc = 0;

   CreateProgressDialog();

   if( !LoopEvaluate() )
   {
      rc |= FilterRule::Error;
   }

   // check if Cancel wasn't pressed (we'd exit the loop above by break then)
   if ( m_idx == m_msgs.GetCount() &&
        (!m_pd ||
            m_pd->Update(m_msgs.GetCount(), GetExecuteProgressString(wxEmptyString))) )
   {
      if ( !LoopCopy() )
      {
         rc |= FilterRule::Error;
      }

      // again, stop right now if we were cancelled
      if ( m_idx == m_msgs.GetCount() )
      {
         if ( !DeleteAll() )
         {
            rc |= FilterRule::Error;
         }
         else // deleted successfully
         {
            rc |= FilterRule::Deleted;
         }

         // actual expunging will be done by the calling code
         if ( m_doExpunge )
         {
            rc |= FilterRule::Expunged;
         }
      }
   }
   //else: cancelled by user

   return rc;
}

bool
FilterRuleApply::LoopEvaluate()
{
   bool allOk = true;

   // first decide what should we do with the messages: fill the arrays with
   // the operations to perform and the destination folder if the operation
   // involves copying the message
   for ( m_idx = 0; m_idx < m_msgs.GetCount(); m_idx++ )
   {
      // do it first so that the arrays have the right size even if we hit
      // "continue" below
      m_allOperations.Add(FilterRuleImpl::None);
      m_destinations.Add(wxEmptyString);

      if ( !GetMessage() )
      {
         continue;
      }

      HeaderCacheHints();

      if ( !Evaluate() )
      {
         allOk = false;
      }

      if ( !UpdateProgressDialog() )
      {
         m_parent->m_MailMessage->DecRef();

         break;
      }

      m_parent->m_MailMessage->DecRef();
   }

   return allOk;
}

bool
FilterRuleApply::LoopCopy()
{
   bool allOk = true;

   for ( m_idx = 0; m_idx < m_msgs.GetCount(); m_idx++ )
   {
      if ( m_allOperations[m_idx] & FilterRuleImpl::Copy )
      {
         if ( !ProgressCopy() )
         {
            // cancelled by user
            break;
         }

         if ( !CopyToOneFolder() )
         {
            allOk = false;
         }
      }
   }

   return allOk;
}

bool
FilterRuleApply::DeleteAll()
{
   CollectForDelete();

   if ( !m_uidsToDelete.IsEmpty() )
   {
      ProgressDelete();

      if ( !m_parent->m_MailFolder->DeleteMessages(&m_uidsToDelete) )
      {
         return false;
      }

      // deleted successfully
      IndicateDeleted();
   }

   return true;
}

void
FilterRuleApply::CreateProgressDialog()
{
   wxFrame *frame = m_parent->m_MailFolder->GetInteractiveFrame();
   if ( frame )
   {
      m_pd = new MProgressDialog
               (
                  wxString::Format
                  (
                     _("Filtering %u messages in folder \"%s\":"),
                     m_msgs.GetCount(),
                     m_parent->m_MailFolder->GetName().c_str()
                  ),
                  _T("\n\n\n"),  // must be tall enough for 4 lines
                  2*m_msgs.GetCount(),
                  frame
               );
   }
   // else: don't show the progress dialog, just log in the status bar
}

bool
FilterRuleApply::GetMessage()
{
   m_parent->m_MessageUId = m_msgs[m_idx];

   if ( m_parent->m_MessageUId == UID_ILLEGAL )
   {
      FAIL_MSG( _T("invalid UID in FilterRule::Apply()!") );

      return false;
   }

   m_parent->m_MailMessage
      = m_parent->m_MailFolder->GetMessage(m_parent->m_MessageUId);

   if ( !m_parent->m_MailMessage )
   {
      // maybe another session deleted it?
      wxLogDebug(
         _T("Filter error: message with UID %ld in folder '%s' doesn't exist any more."),
                 m_parent->m_MessageUId,
                 m_parent->m_MailFolder->GetName().c_str());
      return false;
   }

   return true;
}

void
FilterRuleApply::HeaderCacheHints()
{
   // do some heuristic optimizations: if our program contains requests
   // for the entire message header, get it first because like this it
   // will be cached and all other requests will use it - otherwise we'd
   // have to make several trips to server to get a few separate fields
   // first and only then retrieve the header
   //
   // in the same way, retrieve all the recipients if we need them anyhow
   // before retrieving "To" &c
   if ( m_parent->m_hasHeaderFunc )
   {
      if ( m_parent->m_hasToFunc || m_parent->m_hasRcptFunc
         || m_parent->m_hasHdrLineFunc )
      {
         // pre-retrieve the whole header
         (void)m_parent->m_MailMessage->GetHeader();
      }
   }
   else if ( m_parent->m_hasRcptFunc )
   {
      if ( m_parent->m_hasToFunc )
      {
         // pre-fetch the recipient headers which include "To"
         (void)m_parent->m_MailMessage->GetHeaderLines(headersRecipients);
      }
   }
}

bool
FilterRuleApply::Evaluate()
{
   // reset the result flags
   m_parent->m_operation = FilterRuleImpl::None;

   m_retval = m_parent->m_Program->Evaluate();

   // remember the result
   m_allOperations[m_idx] = m_parent->m_operation;
   m_destinations[m_idx] = m_parent->m_copiedTo;

   if ( m_parent->m_operation & FilterRuleImpl::Expunge )
   {
      m_doExpunge = true;
   }

   return m_retval.IsNumber();
}

void FilterRuleApply::GetSenderSubject(String& from, String& subject, bool full)
{
   subject = MailFolder::DecodeHeader(m_parent->m_MailMessage->Subject());

   AddressList_obj addrList(m_parent->m_MailMessage->GetAddressList(MAT_FROM));
   Address *addr = addrList ? addrList->GetFirst() : NULL;
   if ( addr )
   {
      if ( full )
      {
         from = addr->GetAddress();
      }
      else // short form
      {
         // show just the personal name if any, otherwise show the address
         from = addr->GetName();
         if ( from.empty() )
            from << _T('<') << addr->GetEMail() << _T('>');
      }
   }
   else // no valid sender address
   {
      from = _("unknown sender");
   }
}

bool FilterRuleApply::TreatAsJunk()
{
   if ( m_parent->m_copiedTo.empty() )
      return false;
   RefCounter<MFolder> folder(MFolder::Get(m_parent->m_copiedTo));
   CHECK( folder, false, _T("Copied to null folder?") );
   RefCounter<Profile> profile(folder->GetProfile());
   return READ_CONFIG_BOOL(profile.get(),MP_TREAT_AS_JUNK_MAIL_FOLDER);
}

String FilterRuleApply::CreditsCommon()
{
   String common(_("Filtering message"));

   // don't append "1/1" as it carries no useful information
   const size_t count = m_msgs.GetCount();
   if ( count != 1 )
      common += String::Format(_T(" %lu/%lu"),
                               (unsigned long)m_idx + 1,
                               (unsigned long)count);

   return common;
}

String FilterRuleApply::CreditsForDialog()
{
   // TODO: make the format of the string inside the parentheses configurable
   String textPD;

   if( m_pd )
   {
      textPD = CreditsCommon();

      // make multiline label for the progress dialog
      if( !TreatAsJunk() )
      {
         String from;
         String subject;
         GetSenderSubject(from, subject, true /* full */);

         textPD << _T("\n    ") << _("From: ") << from
                << _T("\n    ") << _("Subject: ") << subject;
      }
   }

   return textPD;
}

String FilterRuleApply::CreditsForStatusBar()
{
   String textLog = CreditsCommon();

   if( !TreatAsJunk() )
   {
      String from;
      String subject;
      GetSenderSubject(from, subject, false /* short */);

      textLog << _T(" (");

      if ( !from.empty() )
      {
         textLog << _("from ") << from << ' ';
      }

      if ( !subject.empty() )
      {
         // the length of status bar text is limited under Windows and, anyhow,
         // status bar is not that wide so truncate subjects to something
         // reasonable
         if ( subject.length() > 40 )
         {
            subject = subject.Left(18) + _T("...") + subject.Right(18);
         }

         textLog << _("about '") << subject << '\'';
      }
      else
      {
         textLog << _("without subject");
      }

      textLog << ')';
   }

   return textLog;
}

String FilterRuleApply::ResultsMessage()
{
   String textResult;

   if ( !m_retval.IsNumber() )
   {
      textResult << _("error!");
   }
   else // filter executed ok
   {
      // if it was caught as a spam, tell the user why do we think so
      if ( !gs_spamTest.empty() )
      {
         textResult += String::Format
                              (
                                 _("recognized as spam (%s); "),
                                 gs_spamTest.c_str()
                              );
         gs_spamTest.clear();
      }

      bool wasDeleted = (m_parent->m_operation
         & FilterRuleImpl::Deleted) != 0;
      if ( !m_parent->m_copiedTo.empty() )
      {
         textResult << (wasDeleted ? _("moved to ") : _("copied to "))
                   << m_parent->m_copiedTo;
      }
      else if ( wasDeleted )
      {
         textResult << _("deleted");
      }
      else // not moved/copied/deleted
      {
         textResult << _("done");
      }
   }

   return textResult;
}

bool
FilterRuleApply::UpdateProgressDialog()
{
   // update the GUI

   // the text for the progress dialog (verbose) and for the log (terse)
   String textPD = CreditsForDialog();
   String textLog = CreditsForStatusBar();

   // and show the result in the progress dialog
   const String textResult = ResultsMessage();

   textLog += _T(" - ") + textResult;

   if ( m_pd )
   {
      textPD += _("\nResult: ");
      textPD += textResult;

      if ( !m_pd->Update(m_idx, textPD) )
      {
         // cancelled by user
         return false;
      }

      // show it in the log window so that the user knows what has
      // happened to the messages
      //
      // NB: textLog may contain '%'s itself, so don't let it be
      //     interpreted as a format string
      wxLogGeneric(M_LOG_WINONLY, _T("%s"), textLog.c_str());
   }
   else // no progress dialog
   {
      // see comment above
      wxLogStatus(_T("%s"), textLog.c_str());
   }

   // We don't need this anymore
   m_parent->m_copiedTo.clear();

   return true;
}

bool
FilterRuleApply::ProgressCopy()
{
   // note that our progress meter may accelerate towards the end as
   // we may copy more than one message initially - but it's ok, it's
   // better than slowing down towards the end, the users really hate
   // this!
   if ( m_pd )
   {

      if( !m_pd->Update(m_msgs.GetCount() + m_idx,
                        GetExecuteProgressString(
                          wxString::Format(_("Copying messages to '%s'..."),
                                           m_destinations[m_idx].c_str()))) )
      {
         return false;
      }
   }

   return true;
}

bool
FilterRuleApply::CopyToOneFolder()
{
   // copy all messages we are copying to this destination at once
   UIdArray uidsToCopy;
   wxArrayLong indexesToCopy;

   uidsToCopy.Add(m_msgs[m_idx]);
   indexesToCopy.Add(m_idx);

   for ( size_t n = m_idx + 1; n < m_msgs.GetCount(); n++ )
   {
      if ( (m_allOperations[n] & FilterRuleImpl::Copy)
         && m_destinations[n] == m_destinations[m_idx] )
      {
         uidsToCopy.Add(m_msgs[n]);
         indexesToCopy.Add(n);
      }
   }

   bool copyOk = m_parent->m_MailFolder->SaveMessages(
      &uidsToCopy, m_destinations[m_idx]);

   for ( size_t copiedIndex = 0; copiedIndex < indexesToCopy.GetCount();
      ++copiedIndex )
   {
      // don't try to copy it when we reach it
      m_allOperations[indexesToCopy[copiedIndex]] &= ~FilterRuleImpl::Copy;

      if ( !copyOk )
      {
         m_allOperations[indexesToCopy[copiedIndex]]
            &= ~FilterRuleImpl::Delete;
      }
   }

   return copyOk;
}

void
FilterRuleApply::CollectForDelete()
{
   m_uidsToDelete.Empty();
   m_indicesDeleted.Empty();

   for ( m_idx = 0; m_idx < m_msgs.GetCount(); m_idx++ )
   {
      if ( m_allOperations[m_idx] & FilterRuleImpl::Delete )
      {
         m_indicesDeleted.Add(m_idx);
         m_uidsToDelete.Add(m_msgs[m_idx]);
      }
   }
}

void
FilterRuleApply::ProgressDelete()
{
   if ( m_pd )
   {
      m_pd->Update(2*m_msgs.GetCount(),
                   GetExecuteProgressString(_("Deleting moved messages...")));
   }
}

void
FilterRuleApply::IndicateDeleted()
{
   // remove the deleted messages from msgs
   // iterate from end because otherwise the indices would shift
   // while we traverse them
   size_t count = m_indicesDeleted.GetCount();
   for ( size_t n = count; n > 0; n-- )
   {
      m_msgs.RemoveAt(m_indicesDeleted[n - 1]);
   }
}

// ----------------------------------------------------------------------------
// MModule_FiltersImpl - the module
// ----------------------------------------------------------------------------

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
                        _T("Filters"),
                        MMODULE_INTERFACE_FILTERS,
                        _("Filtering capabilities plugin"),
                        _T("0.00"))
   MMODULE_PROP(_T("description"),
                _("This plug-in provides a filtering language for Mahogany.\n"
                  "\n"
                  "It is an interpreter for a simplified algebraic language "
                  "which allows one to apply different tests and operations "
                  "to messages, like sorting, replying or moving them "
                  "automatically."))
   MMODULE_PROP(_T("author"), _T("Karsten Ballüder <karsten@phy.hw.ac.uk>"))
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
                          MInterface * /* minterface */, int *errorCode)
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
