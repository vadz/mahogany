/*-*- c++ -*-********************************************************
 * miscutil.h : miscellaneous utility functions                     *
 *                                                                  *
 * (C) 1999 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/


#ifndef MISCUTIL_H
#define MISCUTIL_H

#ifndef  USE_PCH
#  include  "Mconfig.h"
#endif

/**@name Miscellaneous utility functions */
//@{
/** Automatic collection of an email address/name pair into the given subgroup
    of the given addressbook.
    @param email email address string
    @param name  User name or empty
    @param autocollectFlag the 0,1,2 value whether to do auto collection or not
    @param collectNamed if true, only collect addresses with a name
    @param bookName the address book name for auto collection
    @param frame optional pointer to a frame for displaying status messages
*/
void AutoCollectAddresses(const String &email,
                          String name,
                          int autocollectFlag,
                          bool collectNamed,
                          const String& bookName,
                          const String& groupName,
                          MFrame *frame = NULL);

//@}
#endif

