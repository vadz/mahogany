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
#   include   "Message.h"
#   include   "MailFolder.h"
#endif

#include "MModule.h"
#include "Mversion.h"
#include "MInterface.h"

#include <stdlib.h>
#include <ctype.h>

#include "modules/Filters.h"

class Value;
class ArgList;
class Parser;

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
   /** Apply the filter to a single message, returns 0 on success. */
   virtual int Apply(class MailFolder *mf, UIdType uid) const;
   /** Apply the filter to the messages in a folder.
       @param folder - the MailFolder object
       @param NewOnly - if true, apply it only to new messages
       @return 0 on success
   */
   virtual int Apply(class MailFolder *folder, bool NewOnly = true) const;
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
   /** Returns a list of known function definitions. */
   virtual class FunctionList *GetFunctionList(void) = 0;
   /// check if a function is already defined
   virtual class FunctionDefinition *FindFunction(const String &name) = 0;
   /// Defines a function and overrides a given one
   virtual class FunctionDefinition *DefineFunction(const String &name, FunctionPointer fptr) = 0;


   /**@name for runtime information */
   //@{
   /// Set the message and folder to operate on:
   virtual void SetMessage(class MailFolder *folder,
                           UIdType uid = UID_ILLEGAL) = 0;
   /// Obtain the mailfolder to operate on:
   virtual class MailFolder * GetFolder(void) = 0;
   /// Obtain the message UId to operate on:
   virtual UIdType GetMessageUId(void) = 0;
   /// Obtain the message itself:
   virtual class Message * GetMessage(void) = 0;
   //@}

   /// virtual destructor
   virtual ~Parser(void) {}
   /** Constructor function, returns a valid Parser object.
       @param input the input string holding the text to parse
   */
   static Parser * Create(const String &input, MInterface *interface);
   MOBJECT_NAME(Parser)
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
   /// Returns a list of known function definitions. 
   virtual class FunctionList *GetFunctionList(void)
      { return m_FunctionList; }
   /// check if a function is already defined
   virtual class FunctionDefinition *FindFunction(const String &name);
   /// Defines a function and overrides a given one
   virtual class FunctionDefinition *DefineFunction(const String &name, FunctionPointer fptr);


   /**@name for runtime information */
   //@{
   /// Set the message and folder to operate on:
   virtual void SetMessage(class MailFolder *folder = NULL,
                           UIdType uid = UID_ILLEGAL)
      {
         SafeDecRef(m_MailFolder);
         m_MailFolder = folder;
         SafeIncRef(m_MailFolder);
         m_MessageUId = uid;
      }
   /// Obtain the mailfolder to operate on:
   virtual class MailFolder * GetFolder(void) { SafeIncRef(m_MailFolder); return m_MailFolder; }
   /// Obtain the message UId to operate on:
   virtual UIdType GetMessageUId(void) { return m_MessageUId; }
   /// Obtain the message itself:
   virtual class Message * GetMessage(void)
      { return m_MailFolder->GetMessage(m_MessageUId); }
   //@}

protected:
   inline void EatWhiteSpace(void)
      { while(isspace(m_Input[m_Position])) m_Position++; }
   inline const char Char(void) const
      { return m_Input[m_Position]; }
   inline const char CharInc(void)
      { return m_Input[m_Position++]; }
   ParserImpl(const String &input, MInterface *interface);
   ~ParserImpl();

   void AddBuiltinFunctions(void);
   friend class Parser;
private:
   String m_Input;
   size_t m_Position;
   MInterface *m_MInterface;
   class FunctionList *m_FunctionList;

   UIdType m_MessageUId;
   MailFolder *m_MailFolder;
   
   GCC_DTOR_WARN_OFF();
};

// ----------------------------------------------------------------------------
// ABC for logical operators.
// ----------------------------------------------------------------------------

/** This class represents logical operators. */
class Operator : public MObject
{
public:
   virtual Value Evaluate(class SyntaxNode *left, class SyntaxNode * right) const = 0;
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
   MOBJECT_NAME(Block)
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
   MOBJECT_NAME(Number)
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
   MOBJECT_NAME(StringConstant)
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
   MOBJECT_NAME(Negation)
};

/** Functioncall handling */

/** This little class contains the definition of a function,
    i.e. links its name to a bit of C code and maintains a use
    count. */

class FunctionDefinition : public MObject
{
public:
   static FunctionDefinition * Create(const String &name,
                                      FunctionPointer fptr)
      { return new FunctionDefinition(name, fptr); }

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
   ~FunctionDefinition()
      {
         MOcheck();
         ASSERT(m_UseCount == 0); // only used by kbList so far
      }

   inline const String &GetName(void) const { return m_Name; }
   inline FunctionPointer GetFPtr(void) const { return m_FunctionPtr; }
private:
   FunctionDefinition(const String &name, FunctionPointer fptr)
      {
         m_Name = name; m_FunctionPtr = fptr; m_UseCount = 0;
         ASSERT(m_FunctionPtr);
         m_UseCount = 1;
      }
   int             m_UseCount;
   String          m_Name;
   FunctionPointer m_FunctionPtr;
   MOBJECT_NAME(FunctionDefinition)
};

KBLIST_DEFINE(FunctionList, FunctionDefinition);

/** This class represents a function call. It also maintains, via
    static member functions, a table of known functions. */
class FunctionCall : public SyntaxNode
{
public:
   FunctionCall(const String &name, ArgList *args, Parser *p);
   ~FunctionCall()
      {
         ASSERT(m_fd);
         m_fd->DecRef();
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
   MOBJECT_NAME(FunctionCall)
};

FunctionCall::FunctionCall(const String &name, ArgList *args,
                           Parser *p)
   : SyntaxNode(p)
{
   m_name = name;
   m_args = args;
   m_fd = p->FindFunction(name);
   m_function = m_fd->GetFPtr();
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
         {
            ASSERT(m_Left); ASSERT(m_Right);
            return m_Op->Evaluate(m_Left, m_Right);
         }
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
   MOBJECT_NAME(Expression)
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
   virtual Value Evaluate(SyntaxNode *left, SyntaxNode *right) const \
      { \
         MOcheck(); ASSERT(left); ASSERT(right); \
         return left->Evaluate() oper right->Evaluate(); \
      } \
   virtual String Debug(void) const \
      { MOcheck(); return #oper; } \
}

#else
#define IMPLEMENT_OP(name, oper) \
class Operator##name : public Operator \
{ \
public: \
   virtual Value Evaluate(SyntaxNode *left, SyntaxNode *right) const \
      { MOcheck(); return left->Evaluate() oper right->Evaluate(); } \
} 
#endif

IMPLEMENT_VALUE_OP(||,left.GetString().Length(),right.GetString().Length());
IMPLEMENT_VALUE_OP(&&,left.GetString().Length(),right.GetString().Length());
IMPLEMENT_VALUE_OP(+, left.GetString(),right.GetString());
IMPLEMENT_VALUE_OP(-, left.GetString().Length(),right.GetString().Length());
IMPLEMENT_VALUE_OP(*, left.GetString().Length(),right.GetString().Length());
IMPLEMENT_VALUE_OP(/, left.GetString().Length(),right.GetString().Length());

IMPLEMENT_OP(Plus,+);
IMPLEMENT_OP(Minus,-);
IMPLEMENT_OP(Times,*);
IMPLEMENT_OP(Divide,/);

class OperatorAnd : public Operator
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
};

class OperatorOr : public Operator
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
};

/* static */
Parser *
Parser::Create(const String &input, MInterface *i)
{
   return new ParserImpl(input, i);
}


ParserImpl::ParserImpl(const String &input, MInterface *interface)
{
   m_Input = input;
   Rewind();
   m_MInterface = interface;
   m_FunctionList = new FunctionList(false); // must clean up manually
   m_MailFolder = NULL;
   m_MessageUId = UID_ILLEGAL;

   AddBuiltinFunctions();
}

ParserImpl::~ParserImpl()
{
   /* Make sure all function definitions are gone. They are
      refcounted, so the kbList cannot clean them up properly and
      would trigger asserts if it tried. */
   FunctionList *fl = GetFunctionList();
   FunctionList::iterator i = fl->begin();
   while(i != fl->end())
   {
      (*i)->DecRef();
      fl->erase(i);
   }
   delete m_FunctionList;

   SafeDecRef(m_MailFolder);
}


FunctionDefinition *
ParserImpl::DefineFunction(const String &name, FunctionPointer fptr)
{
   FunctionList *fl = GetFunctionList();
   FunctionDefinition *fd = FindFunction(name);
   if(fd)
   {
      fd->DecRef();
      return NULL; // already exists, not overridden
   }
   fd = FunctionDefinition::Create(name, fptr);
   fl->push_back(fd);
   return fd;
}

FunctionDefinition *
ParserImpl::FindFunction(const String &name)
{
   FunctionList *fl = GetFunctionList();
   
   for(FunctionList::iterator i = fl->begin();
       i != fl->end();
       i++)
      if(name == (**i).GetName())
      {
         (**i).IncRef();
         return *i;
      }
   return NULL;
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
   static Value func_print(ArgList *args, Parser *p)
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

/* * * * * * * * * * * * * * *
 *
 * Tests for message contents
 *
 * * * * * * * * * * * * * */
   static Value func_matchi(ArgList *args, Parser *p)
   {
      if(args->Count() != 2)
         return 0;
      Value v1 = args->GetArg(0)->Evaluate();
      Value v2 = args->GetArg(1)->Evaluate();
      String haystack = v1.ToString();
      String needle = v2.ToString();
      strutil_tolower(haystack); strutil_tolower(needle);
      return haystack.Find(needle) != -1;
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

   static Value func_delete(ArgList *args, Parser *p)
   {
      if(args->Count() != 0)
         return Value("");
      MailFolder *mf = p->GetFolder();
      if(! mf) return Value(0);
      UIdType uid = p->GetMessageUId();
      int rc = mf->DeleteMessage(uid);
      mf->DecRef();
      return Value(rc);
   }

   static Value func_copytofolder(ArgList *args, Parser *p)
   {
      if(args->Count() != 1)
         return Value("");
      Value fn = args->GetArg(0)->Evaluate();
      MailFolder *mf = p->GetFolder();
      UIdType uid = p->GetMessageUId();
      INTARRAY ia;
      ia.Add(uid);
      int rc = mf->SaveMessages(&ia, fn.ToString(), true);
      mf->DecRef();
      return Value(rc);
   }
};

void
ParserImpl::AddBuiltinFunctions(void)
{
   ASSERT(DefineFunction("print", func_print) != NULL);
   ASSERT(DefineFunction("matchi", func_matchi) != NULL);
   ASSERT(DefineFunction("subject", func_subject) != NULL);
   ASSERT(DefineFunction("delete", func_delete) != NULL);
   ASSERT(DefineFunction("copyto", func_copytofolder) != NULL);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * FilterRuleImpl - the class representing a program
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int FilterRuleImpl::Apply(class MailFolder *mf, UIdType uid) const
{
   if(! m_Program || ! m_Parser)
      return 0;
   ASSERT(mf);
   ASSERT(uid != UID_ILLEGAL);
   mf->IncRef();
   if(mf->Lock())
   {
      m_Parser->SetMessage(mf, uid);
      Value rc = m_Program->Evaluate();
      m_Parser->SetMessage(NULL);
      mf->UnLock();
      mf->DecRef();
      return (int) rc.GetNumber();
   }
   else
   {
      mf->DecRef();
      return 0; //FIXME: error message?
   }
}

int
FilterRuleImpl::Apply(class MailFolder *mf, bool NewOnly) const
{
   if(! m_Program || ! m_Parser)
      return 0;
   int rc = -1;
   ASSERT(mf);
   mf->IncRef();
   HeaderInfoList *hil = mf->GetHeaders();
   if(hil)
   {
      rc = 0; // no error so far
      for(size_t i = 0; i < hil->Count() && rc == 0; i++)
      {
         const HeaderInfo * hi = (*hil)[i];
         if( ! NewOnly || // handle all or only new ones:
             (
                (hi->GetStatus() & MailFolder::MSG_STAT_RECENT)
                && ! (hi->GetStatus() & MailFolder::MSG_STAT_SEEN)
                ) // new == recent and not seen
            )
         {
            rc = Apply(mf, hi->GetUId());
         }
      }
      hil->DecRef();
   }
   mf->DecRef();
   return rc;
}

FilterRuleImpl::FilterRuleImpl(const String &filterrule,
                               MInterface *interface,
                               MModule_Filters *mod
   ) 
{

   m_FilterModule = mod;
   m_Parser = Parser::Create(filterrule, interface);
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

/** A small test for the filter module. */
static
int FilterTest(MInterface *interface, MModule_Filters *that)
{

   String program = interface->GetMApplication()->
      GetProfile()->readEntry("FilterTest","matchi(subject(),\"html\")&print(\"Message: '\", subject(), \"'\");");

   FilterRule * fr = that->GetFilter(program);

   String folderName =
      interface->GetMApplication()->GetProfile()->
      readEntry(MP_NEWMAIL_FOLDER,MP_NEWMAIL_FOLDER_D);

   MailFolder * mf = MailFolder::OpenFolder(MF_PROFILE_OR_FILE, folderName); 

   if(mf)
   {
      // apply this rule to all messages
      int rc = fr->Apply(mf, false);
      mf->DecRef();
   }
   fr->DecRef();
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
   DEFAULT_ENTRY_FUNC
protected:
   MModule_FiltersImpl(MInterface *interface)
      {
         m_Interface = interface;
#ifdef DEBUG
         FilterTest(m_Interface, this);
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
                                 (MModule_FiltersImpl *) this); 
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



