/*-*- c++ -*-********************************************************
 * CommonBase: common base class                                    *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifndef COMMONBASE_H
#define	COMMONBASE_H

#ifdef         USE_IOSTREAMH
#   ifdef    OS_WIN
    // remember these 8.3 names
#      include	<strstrea.h>
#   else
#      include	<strstream.h>
#   endif
#else
#      include	<strstream>
#endif  // VC++

#ifdef	USE_MEMDEBUG
#	include	"magic.h"
#else
#	define	DEFINE_MAGIC(classname,magic)	/**/
#     	define	SET_MAGIC(magic)		/**/
#endif

#ifdef USE_COMMONBASE
#ifndef	USE_CLASSINFO
#	define	CB_IMPLEMENT_CLASS(newclass, parent) 
#	define	CB_DECLARE_CLASS(newclass, parent)	
#else
#	ifdef	USE_WXOBJECT
#		define	CB_IMPLEMENT_CLASS(newclass, parent) IMPLEMENT_CLASS(newclass, parent)	
#		define CB_DECLARE_CLASS(newclass) DECLARE_CLASS(newclass,  parent)
#	else
#		define	CB_IMPLEMENT_CLASS(newclass, parent) 
#		define CB_DECLARE_CLASS(newclass,parent)
#	endif
#endif

/**
   CommonBase: common base class for debugging purposes.
   This class contains lots of debugging information which can be
   queried at run time. It also defines a minimal set of virtual
   methods to be implemented by all other classes. Furtheron it
   provides an interface for debugging and error message output.
   */
class CommonBase
#ifdef USE_WXOBJECT
: public class wxObject
#endif
{
#ifdef	USE_MEMDEBUG
   unsigned long		cb_magic;
   static	unsigned long	cb_class_magic;
#endif
   
#ifdef	USE_WXOBJECT
   DECLARE_CLASS(CommonBase)
#endif

public:
   /// constructor
   CommonBase() { SET_MAGIC(MAGIC_COMMONBASE); }
   
   /// virtual destructor to get deallocation right
   virtual ~CommonBase() { Validate(); }

   // /check wether object is initialised
   //   virtual bool		IsInitialised(void) const = 0;

   // / return ClassName structure
   //static const ClassName &GetClassName(void) = 0;

   CB_DECLARE_CLASS(CommonBase, CommonBase)

#ifndef NDEBUG
   /// prints some debugging information
   virtual void Debug(void) const; 
#endif
#ifdef USE_MEMDEBUG
   void Validate(void);
#else
   void Validate(void) {}
#endif
};

/// macro to call common base class debugging function
#	define	CBDEBUG()	CommonBase::Debug()
#	ifndef NDEBUG
#		define	VAR(x)		wxLogTrace(#x " = %s", x)
#	endif
#else
// empty definition
class CommonBase
{
};

#	define	CLASSINIT(name)
#	define	CBDEBUG()
#	define	VAR(x)
#endif // USE_COMMONBASE

#endif	// COMMONBASE_H
