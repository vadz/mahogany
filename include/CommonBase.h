/*-*- c++ -*-********************************************************
 * CommonBase: common base class                                    *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$             *
 ********************************************************************
 * $Log$
 * Revision 1.4  1998/05/02 15:21:31  KB
 * Fixed the #if/#ifndef etc mess - previous sources were not compilable.
 *
 * Revision 1.3  1998/05/01 14:02:39  KB
 * Ran sources through conversion script to enforce #if/#ifdef and space/TAB
 * conventions.
 *
 * Revision 1.2  1998/03/26 23:05:36  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:10  karsten
 * first try at a complete archive
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
#		define CB_DECLARE_CLASS(newclass,parent) virtual const char *GetClassName(void)const { return #newclass; }
#	endif
#endif

/**
   CommonBase: common base class for debugging purposes.
   This class contains lots of debugging information which can be
   queried at run time. It also defines a minimal set of virtual
   methods to be implemented by all other classes. Furtheron it
   provides an interface for debugging and error message output.
   */
#ifdef USE_WXOBJECT
class	CommonBase : public class wxObject
#else
class	CommonBase
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
#		define	VAR(x)		cerr << #x << " = " << x << endl;
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
