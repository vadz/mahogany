/*-*- c++ -*-********************************************************
 * MailFolder class: handling of Unix mail folders                  *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$ 
 *******************************************************************/

#include  "Mpch.h"
#include  "Mcommon.h"

#include	"Message.h"
#include	"Profile.h"
#include	"FolderView.h"

#include   "MailFolder.h"
#include   "MailFolderCC.h"

MailFolder *
MailFolder::OpenFolder(MailFolderType type, String const &name)
{
   if(type == MF_PROFILE)
      return MailFolderCC::OpenFolder(name);
   else
      return NULL;
}


MailFolder::~MailFolder()
{
}
