/*-*- c++ -*-********************************************************
 * ScoreModule - a pluggable module that can score messages         *
 *                                                                  *
 * (C) 1999 by Karsten Ballüder (karsten@phy.hw.ac.uk)              *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef SCOREMODULE_H
#define SCOREMODULE_H

#include "MModule.h"

/**
   This is the interface for the Mahogany scoring module.
   It just provides one function which calculates a score function
   from a HeaderInfo pointer.
*/
class MModule_Scoring : public MModule
{
public:
   MModule_Scoring() : MModule() { }

   /** Calculates the score, which can be both positive or negative.
       @param mf pointer to the MailFolder in question
       @param headerInfo the HeaderInfo structure for the message to score
       @return the score
   */
   virtual long  ScoreMessage(class MailFolder *mf,
                              class HeaderInfo *headerInfo) = 0;
};

#define MMODULE_INTERFACE_SCORING   "Scoring"

#endif // SCOREMODULE_H
