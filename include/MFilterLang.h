///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MFilterLang.h -- constants describing the filter language
// Purpose:     this is a separate file so that we can include it from
//              modules/Filters.cpp as well
// Author:      Vadim Zeitlin
// Modified by:
// Created:     27.06.02
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _M_FILTERLANG_H_
#define _M_FILTERLANG_H_

/** @name the spam detection tests
 */
//@{

/// X-Spam-Status: Yes header?
#define SPAM_TEST_SPAMASSASSIN "spamassassin"

/// does the subject contain too many (unencoded) 8 bit chars?
#define SPAM_TEST_SUBJ8BIT "subj8bit"

/// does the subject contain too many capital letters?
#define SPAM_TEST_SUBJCAPS "subjcaps"

/// is the subject of the "...            xyz-12-foo"?
#define SPAM_TEST_SUBJENDJUNK "subjendjunk"

/// korean charset?
#define SPAM_TEST_KOREAN "korean"

/// suspicious X-Authentication-Warning header?
#define SPAM_TEST_XAUTHWARN "xauthwarn"

/// suspicious Received: headers?
#define SPAM_TEST_RECEIVED "received"

/// HTML contents at top level?
#define SPAM_TEST_HTML "html"

/// suspicious MIME structure?
#define SPAM_TEST_MIME "badmime"

/// blacklisted by the RBL?
#define SPAM_TEST_RBL "rbl"

//@}

#endif // _M_FILTERLANG_H_

