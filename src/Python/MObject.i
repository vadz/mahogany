// -*- c++ -*-/////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MObject.h - the base class for all ref counted objects
// Purpose:     MObject provides the standard lock/unlock methods and deletes
//              itself when it's ref count reaches 0. As a consequence, it
//              should only be allocated with "new" and never deleted.
// Author:      Vadim Zeitlin
// Modified by: 
// Created:     09.08.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

%nodefault;

%module   MObject
%{
#include "Mswig.h"
#include "MObject.h"
%}

%import MString.i

class MObject
{
public:
#ifdef DEBUG
    virtual String DebugDump() const;
#endif
    static void CheckLeaks();
};

class MObjectRC : public MObject
{
public:
   virtual void IncRef();
   virtual bool DecRef();
};
