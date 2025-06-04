/*-*- c++ -*-********************************************************
 * wxMIds.h : define numeric ids and menu names                     *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/
#ifndef	WXMIDS_H
#define WXMIDS_H

/** Definition of all numeric GUI element IDs.
*/
enum
{
   M_WXID_ANY = -1,
   M_WXID_FOLDERVIEW = 1,
   M_WXID_PEP_OK, M_WXID_PEP_UNDO, M_WXID_PEP_CANCEL,
   M_WXID_PEP_RADIO,
   M_WXID_FOLDERVIEW_LISTCTRL,
   M_WXID_HELP = 999   // In Britain, 999 calls help. :-)
};

#endif
