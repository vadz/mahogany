/*-*- c++ -*-********************************************************
 * MailFolder class: handling of Unix mail folders                  *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.2  1998/03/26 23:05:42  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:27  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#include  "Mpch.h"
#include  "Mcommon.h"

#include	"Message.h"
#include	"Profile.h"
#include	"FolderView.h"

#include	"MailFolder.h"

CB_IMPLEMENT_CLASS(MailFolder, CommonBase);

MailFolder::~MailFolder()
{
}
