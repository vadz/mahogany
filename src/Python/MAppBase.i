/*-*- c++ -*-********************************************************
 * MApplication class: all non GUI specific application stuff       *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/

%nodefault;

%module   MAppBase
%{
//#include "Mswig.h"
#include "Mcommon.h"
#include "gui/wxMDialogs.h"
#include "gui/wxMApp.h"
#include "MFrame.h"
%}

%import typemaps.i
%import MString.i
%import MProfile.i
