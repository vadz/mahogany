/*-*- c++ -*-********************************************************
 * PalmOSModule -  a module to synchronise to PalmOS based organiser*
 *                                                                  *
 * (C) 1999 by Karsten Ballüder (karsten@phy.hw.ac.uk)              *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef PALMOSMODULE_H
#define PALMOSMODULE_H

#include "MModule.h"

/**
   The interface for the PalmOS device synchronisation module.
 */
class MModule_PalmOS : public MModule
{
 public:
   virtual void  Synchronise(void) = 0;
};

#define MMODULE_INTERFACE_PALMOS   "PalmOS"

#endif // PALMOSMODULE_H
