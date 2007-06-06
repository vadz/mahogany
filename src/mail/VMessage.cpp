//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/VMessage.cpp: implementation of MessageVirt class
// Purpose:     MessageVirt represents a message in MailFolderVirt
// Author:      Vadim Zeitlin
// Modified by:
// Created:     17.07.02
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include  "Mpch.h"

#ifndef USE_PCH
    #include "Mcommon.h"
#endif // USE_PCH

#include "MailFolder.h"

#include "mail/VMessage.h"

// ============================================================================
// MessageVirt implementation
// ============================================================================

/* static */
MessageVirt *MessageVirt::Create(MailFolder *mf,
                                 UIdType uid,
                                 int *flags,
                                 Message *message)
{
    CHECK( mf && flags && message, NULL, _T("NULL pointer in MessageVirt::Create") );

    return new MessageVirt(mf, uid, flags, message);
}

MessageVirt::MessageVirt(MailFolder *mf,
                         UIdType uid,
                         int *flags,
                         Message *message)
{
    m_mf = mf;
    m_mf->IncRef();

    m_uid = uid;
    m_flags = flags;

    m_message = message;
}

MessageVirt::~MessageVirt()
{
    m_message->DecRef();
    m_mf->DecRef();
}

