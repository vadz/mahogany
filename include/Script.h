/*-*- c++ -*-********************************************************
 * Script.h : abstract base class for scripting modules             *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef SCRIPT_H
#define SCRIPT_H

#ifdef __GNUG__
#pragma interface "Script.h"
#endif

/**
   Script class, encapsulating something that can be run.
   */
class Script 
{
public:
   /// execute Script, returns returnvalue from program
   int Run(String const &parameters);

   /// set standard input for this script:
   void SetInput(String const & iinput)
      { input = iinput; }
   /// get standard output from running the script:
   String const &GetOutput(void)
      { return output; }
   
   /// gets the script's name
   String const &GetName(void)
      { return name; }
   /// gets a description text about the script
   String const &GetDescription(void)
      { return description; }
   /// empty default constructor:
   Script(void)
      {}
   /// virtual destructor:
   virtual ~Script()
      {}
private:
   /// no copy constructor
   Script(const Script &x);
   /// no assignment operator
   Script& operator=(const Script &x);
protected:
   /// the text to feed to stdin:
   String input;
   /// where to store stdout:
   String	output;
   /// name:
   String	name;
   /// the "thing" to call:
   String	command;
   /// a short description:
   String	description;
   /// execute Script, returns returnvalue from program
   virtual int run(String const &parameters) = 0;
};

/**
   external script class, representing shell scripts and programs
   */
class ExternalScript : public Script
{
public:
   /** Constructor:
       @param CmdLine the CmdLine to execute when running script.
       @param name the name of the script
       @param a short description of the string
   */
   ExternalScript(String const & CmdLine,
		  String const & name,
		  String const & Description);


protected:
   /// execute Script, returns returnvalue from program
   virtual int run(String const &parameters);
   
private:
   /// no copy constructor
   ExternalScript(const Script &x);
   /// no assignment operator
   ExternalScript& operator=(const Script &x);
};

#ifdef	USE_PYTHON
/**
   internal script class, representing python scripts
   */
class InternalScript : public Script
{
public:
   /** Constructor:
       @param CmdLine the python function to execute when running script.
       @param name the name of the script
       @param a short description of the string
   */
   InternalScript(String const & function,
		  String const & name,
		  String const & Description);

protected:
   /// execute Script, returns returnvalue from program
   virtual int run(String const &parameters);
   
private:
   /// no copy constructor
   InternalScript(const Script &x);
   /// no assignment operator
   InternalScript& operator=(const Script &x);
};
#endif	// USE_PYTHON
#endif  // M_SCRIPT
