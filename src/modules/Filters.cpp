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
#ifndef USE_PCH
#   include   "Mcommon.h"
#   include   "kbList.h"
#   include   "Message.h"
#   include   "MailFolder.h"
#endif

#include "MModule.h"
#include "Mversion.h"
#include "MInterface.h"
#include "MPython.h"

#include <stdlib.h>
#include <ctype.h>
#include <time.h>  // mktime()

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
   /** Apply the filter to the messages in a folder.
       @param folder - the MailFolder object
       @param msgs - the list of messages to apply to
       @return 0 on success
   */
   virtual int Apply(class MailFolder *folder, UIdArray msgs) const;
   static FilterRule * Create(const String &filterrule,
                              MInterface *interface, MModule_Filters *mod)
      { return new FilterRuleImpl(filterrule, interface, mod); }
#ifdef DEBUG
   void Debug(void);
#endif
   
protected:
   FilterRuleImpl(const String &filterrule,
                  MInterface *interface,
                  MModule_Filters *fmodule);
   ~FilterRuleImpl();
   /// common code for the two Apply() functions
   int ApplyCommonCode(class MailFolder *folder,
                       UIdArray *msgs,
                       bool newOnly) const;
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
{ TT_Invalid = -1, TT_Char, TT_Identifier, TT_Keyword, TT_String, TT_Number, TT_EOF };

/** Tokens are either an ASCII character or a value greater >255.
 */
class Token
{
public:
   Token() { m_type = TT_Invalid; }
   inline TokenType GetType(void) const
      { return m_type; }
   inline char GetChar(void) const
      { ASSERT(m_type == TT_Char); return m_char; }
   inline const String & GetString(void) const
      { ASSERT(m_type == TT_String); return m_string; }
   inline const String & GetIdentifier(void) const
      { ASSERT(m_type == TT_Identifier); return m_string; }
   inline const String & GetKeyword(void) const
      { ASSERT(m_type == TT_Keyword); return m_string; }
   inline long GetNumber(void)
      { ASSERT(m_type == TT_Number); return m_number; }
   
   inline void SetChar(char c)
      { m_type = TT_Char; m_char = c; }
   inline void SetString(String const &s)
      { m_type = TT_String; m_string = s; }
   inline void SetIdentifier(String const &s)
      { m_type = TT_Identifier; m_string = s; }
   inline void SetKeyword(String const &s)
      { m_type = TT_Keyword; m_string = s; }
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
   /// Print out a message in a message box.
   virtual void Output(const String &msg) = 0;
   /// Print out a message.
   virtual void Log(const String &imsg, int level = M_LOG_DEFAULT) = 0;
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

   PROGRAM :=
     | BLOCK [ BLOCK ]
    
   BLOCK :=
     | CONDITION ;
     | { BLOCK RESTBLOCK}

   RESTBLOCK :=
     | BLOCK ; RESTBLOCK  
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
   class Block      * ParseProgram(void);
   class SyntaxNode * ParseIfElse(void);
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
   virtual void Log(const String &imsg, int level)
      {
         String msg = _("Parser: ");
         msg << imsg;
         m_MInterface->Log(level, msg);
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
   {
      return m_MailFolder ?
         m_MailFolder->GetMessage(m_MessageUId) : 
         NULL ;
   }
   //@}
   virtual MInterface * GetInterface(void) { return m_MInterface; }

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
   
   GCC_DTOR_WARN_OFF
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
   MOBJECT_NAME(FunctionCall)
};

FunctionCall::FunctionCall(const String &name, ArgList *args,
                           FunctionDefinition *fd,
                           Parser *p)
   : SyntaxNode(p)
{
   m_name = name;
   m_args = args;
   m_fd = fd;
   m_fd->IncRef();
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


/** An if-else statement. */
class IfElse : public SyntaxNode
{
public:
   IfElse(SyntaxNode *condition,
          SyntaxNode *ifBlock,
          SyntaxNode *elseBlock,
          Parser *p)
      : SyntaxNode(p)
      {
         m_Condition = condition;
         m_IfBlock = ifBlock;
         m_ElseBlock = elseBlock;
      }
   virtual Value Evaluate(void) const
      {
         MOcheck();
         ASSERT(m_Condition != NULL);
         ASSERT(m_IfBlock != NULL);
         Value rc = m_Condition->Evaluate();
         if(rc.ToNumber())
            return m_IfBlock->Evaluate();
         else
         {
            if(m_ElseBlock)
               return m_ElseBlock->Evaluate();
            else
               return Value(0); // is this a sensible return value?
         }
      }
   void SetIfBlock(SyntaxNode *left)
      { MOcheck(); ASSERT(m_IfBlock == NULL); m_IfBlock = left; }
   void SetElseBlock(SyntaxNode *right)
      { MOcheck(); ASSERT(m_ElseBlock == NULL); m_ElseBlock = right; }
   void SetCondition(SyntaxNode *right)
      { MOcheck(); ASSERT(m_Condition == NULL); m_Condition = right; }
   ~IfElse()
      {
         MOcheck();
         if(m_Condition) delete m_Condition;
         if(m_IfBlock) delete m_IfBlock;
         if(m_ElseBlock)  delete m_ElseBlock;
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
   /* Now check for keywords, which so far have been treated as normal 
      identifiers. */
   if(token.GetType() == TT_Identifier)
   {
      if(token.GetIdentifier() == "if")
         token.SetKeyword("if");
      else if(token.GetIdentifier() == "else")
         token.SetKeyword("else");
   }
   return token;
}

SyntaxNode *
ParserImpl::Parse(void)
{
   MOcheck();
   return ParseProgram();
}

SyntaxNode *
ParserImpl::ParseIfElse(void)
{
   /* IFELSE :=
      IF ( CONDITION ) BLOCK [ ELSE BLOCK ] 
   */
   MOcheck();

   SyntaxNode
      *condition = NULL,
      *ifBlock = NULL,
      *elseBlock = NULL;
   
   Token t = PeekToken();
   ASSERT(t.GetKeyword() == "if");
   (void) GetToken(); // swallow "if"
   t = PeekToken();
   if(t.GetType() != TT_Char || t.GetChar() != '(')
   {
      Error(_("'(' expected after 'if'."));
      return NULL;
   }
   (void) GetToken(); // swallow '('

   condition = ParseCondition();
   if(! condition)
      goto ifElseBailout;

   t = PeekToken();
   if(t.GetType() != TT_Char || t.GetChar() != ')')
   {
      Error(_("')' expected after condition in if statement."));
      goto ifElseBailout;
   }
   (void) GetToken(); // swallow ')'

   ifBlock = ParseBlock();
   if(! ifBlock)
      goto ifElseBailout;

   t = PeekToken();
   if(t.GetType() == TT_Keyword && t.GetKeyword() == "else")
   {
      // we must parse the else branch, too:
      (void) GetToken(); // swallow the "else"
      elseBlock = ParseBlock();
      if(! elseBlock)
         goto ifElseBailout;
   }
   // if we reach here, everything was parsed alright:

   return new IfElse(condition, ifBlock, elseBlock, this);
   
   // all errors will jump to this label:
 ifElseBailout:
   if(condition) delete condition;
   if(ifBlock)   delete ifBlock;
   if(elseBlock) delete elseBlock;
   return NULL;
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
      else
         Error(_("Expected an expression in argument list."));
      t = PeekToken();
      if(t.GetType() != TT_Char)
      {
         Error(_("Expected ',' or ')' behind argument."));
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
      Error(_("Expected ')' at end of argument list."));
   else
      GetToken(); // swallow it
   
   FunctionDefinition *fd = FindFunction(functionName);
   SyntaxNode *sn = NULL;
   if(fd)
   {
      sn = new FunctionCall(functionName, args, fd, this);
      fd->DecRef();
   }
   else
   {
      String err;
      err.Printf(_("Attempt to call undefined function '%s'."),
                 functionName.c_str());
      Error(err);
      if(args)
         delete args;
   }
   return sn;
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
            Error(_("Expected ')' after expression."));
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
   // sn == NULL, illegal factor
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
ParserImpl::ParseProgram(void)
{
   MOcheck();
   /* PROGRAM :=
      | BLOCK [ BLOCK ]
   */
   bool empty = true;
   Block * pgm = new Block(this);
   Block * block = NULL;
   Token t;
   for(;;)
   {
      t = PeekToken();
      if(t.GetType() == TT_EOF)
         break;
      block = ParseBlock();
      if(block == NULL)
         break;
      pgm->AddNode(block);
      empty = false;
   }
   if(empty)
   {
      delete pgm;
      pgm = NULL;
   }

   if(pgm == NULL)
      Error(_("Parse error, cannot find valid program."));
   return pgm;
}

Block *
ParserImpl::ParseBlock(void)
{
   MOcheck();
   /* BLOCK :=
      | { BLOCK RESTBLOCK }
      | CONDITION ;
      | IFELSE
   */

   Token t = PeekToken();

   // Is there an IF() ELSE statement?
   if(t.GetType() == TT_Keyword && t.GetKeyword() == "if")
   {
      SyntaxNode *ifelse = ParseIfElse();
      if(! ifelse)
         return NULL;
      Block *block = new Block(this);
      block->AddNode(ifelse);
      return block;
   }
   
   // Normal block:
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
      t = PeekToken();
      if(t.GetType() != TT_Char || t.GetChar() != '}')
      {
         Error(_("Expected Condition after conditional operator."));
         delete stmt;
         return NULL;
      }
      GetToken();
      return stmt;
   }
   else
   {
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
}
      
void
ParserImpl::ParseRestBlock(Block *parent)
{
   MOcheck();
   /* RESTBLOCK :=
      | BLOCK ; RESTBLOCK
      | EMPTY
   */

   Block *stmt;

   while((stmt = ParseBlock()) != NULL)
   {
      parent->AddNode(stmt);
      stmt = ParseBlock();
   }
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


extern "C"
{
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

   static Value func_matchregex(ArgList *args, Parser *p)
   {
      ASSERT(args);
      if(args->Count() != 2)
         return 0;
      Value v1 = args->GetArg(0)->Evaluate();
      Value v2 = args->GetArg(1)->Evaluate();
      String haystack = v1.ToString();
      String needle = v2.ToString();
      class strutil_RegEx * re =
         p->GetInterface()->strutil_compileRegEx(needle);
      if(! re) return FALSE;
      bool rc = p->GetInterface()->strutil_matchRegEx(re, haystack,0);
      p->GetInterface()->strutil_freeRegEx(re);
      return rc;
   }

#ifdef USE_PYTHON   
   static Value func_python(ArgList *args, Parser *p)
   {
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
      return rc ? (result != 0) : false;
   }
#endif

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
      return Value(rc);
   }

   static Value func_copytofolder(ArgList *args, Parser *p)
   {
      if(args->Count() != 1)
         return 0;
      Value fn = args->GetArg(0)->Evaluate();
      MailFolder *mf = p->GetFolder();
      UIdType uid = p->GetMessageUId();
      UIdArray ia;
      ia.Add(uid);
      int rc = mf->SaveMessages(&ia, fn.ToString(), true);
      mf->DecRef();
      return Value(rc);
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
      tms.tm_sec = 0; tms.tm_min = 0; tms.tm_hour = 0;
      tms.tm_wday = -1; tms.tm_yday = -1; // I hope this works!
      (void) msg->GetStatus(NULL,
                            (unsigned int *)&tms.tm_mday,
                            (unsigned int *)&tms.tm_mon,
                            (unsigned int *)&tms.tm_year);
      msg->DecRef();
      return Value(mktime(&tms));
   }

   static Value func_now(ArgList *args, Parser *p)
   {
      if(args->Count() != 0)
         return Value(-1);
      return Value(time(NULL));
   }

   static Value func_size(ArgList *args, Parser *p)
   {
      if(args->Count() != 0)
         return Value(-1);
      Message *msg = p->GetMessage();
      unsigned long size;
      (void) msg->GetStatus(&size);
      msg->DecRef();
      return Value(size);
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
      return 1;
   }
};



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
   DefineFunction("from", func_from);
   DefineFunction("header", func_header);
   DefineFunction("body", func_body);
   DefineFunction("text", func_text);
   DefineFunction("delete", func_delete);
   DefineFunction("copy", func_copytofolder);
   DefineFunction("move", func_movetofolder);
   DefineFunction("date", func_date);
   DefineFunction("size", func_size);
   DefineFunction("now", func_now);
   DefineFunction("isspam", func_checkSpam);
   DefineFunction("expunge", func_expunge);
#ifdef USE_PYTHON
   DefineFunction("python", func_python);
#endif
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
//FIXME   ASSERT(mf->IsLocked()); // must be called on a locked mailfolder
   mf->IncRef();
   m_Parser->SetMessage(mf, uid);
   Value rc = m_Program->Evaluate();
   m_Parser->SetMessage(NULL);
   mf->DecRef();
   return (int) rc.GetNumber();
}

int
FilterRuleImpl::Apply(class MailFolder *mf, bool NewOnly) const
{
   return ApplyCommonCode(mf, NULL, NewOnly);
}

int
FilterRuleImpl::Apply(class MailFolder *folder, UIdArray msgs) const
{
   return ApplyCommonCode(folder, &msgs, FALSE);
}

int
FilterRuleImpl::ApplyCommonCode(class MailFolder *mf,
                                UIdArray *msgs,
                                bool newOnly) const
{
   if(! m_Program || ! m_Parser)
      return 0;
   int rc = 1; // no error yet
   ASSERT(mf);
   mf->IncRef();

   if(msgs) // apply to all messages in list
   {
      for(size_t idx = 0; idx < msgs->Count(); idx++)
         rc &= Apply(mf, (*msgs)[idx]);
   }
   else // apply to all or all recent messages
   {
      HeaderInfoList *hil = mf->GetHeaders();
      if(hil)
      {
         for(size_t i = 0; i < hil->Count(); i++)
         {
            const HeaderInfo * hi = (*hil)[i];
            if( (! newOnly) || // handle all or only new ones:
                (
                   (hi->GetStatus() & MailFolder::MSG_STAT_RECENT)
                   && ! (hi->GetStatus() & MailFolder::MSG_STAT_SEEN)
                   && ! (hi->GetStatus() & MailFolder::MSG_STAT_DELETED)
                   ) // n   ew == rece   nt and not seen
               )
            {
               rc &= Apply(mf, hi->GetUId());
            }
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

void FilterRuleImpl::Debug(void)
{
   m_Parser->Output(m_Program->Debug());
}


#if 0
/** A small test for the filter module. */
static
int FilterTest(MInterface *interface, MModule_Filters *that)
{

   String program = interface->GetMApplication()->
      GetProfile()->readEntry("FilterTest",
                              "if(matchi(subject(),\"html\"))"
                              "{copy(\"Trash\")&message(\"copied message to trash:\", subject());}"
                              "else{message(\"skipped message\");}"
         );
   if( program.Length() == 0)
      return 1;
   
   FilterRule * fr = that->GetFilter(program);

   ( (FilterRuleImpl *)fr)->Debug();
   
   String folderName =
      interface->GetMApplication()->GetProfile()->
      readEntry(MP_NEWMAIL_FOLDER,MP_NEWMAIL_FOLDER_D);

   MailFolder * mf = MailFolder::OpenFolder(MF_PROFILE_OR_FILE, folderName); 

   int rc = -1;
   if(mf)
   {
      // apply this rule to all messages
      rc = fr->Apply(mf, false);
      mf->DecRef();
   }
   fr->DecRef();
   return rc;
}
#endif
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
//         (void) FilterTest(m_Interface, this);
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



