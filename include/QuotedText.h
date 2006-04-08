///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   QuotedText.h
// Purpose:     functions for dealing with quoted text in mail messages
// Author:      Vadim Zeitlin
// Created:     2006-04-08 (extracted from src/modules/viewflt/QuoteURL.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 2002-2006 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _M_QUOTEDTEXT_H_
#define _M_QUOTEDTEXT_H_

/**
    An opaque struct used by CountQuoteLevel() to store global information
    about the message.
 */
class QuoteData
{
public:
   QuoteData() { levelWrapped = 0; linePrev = NULL; }

   // functions for querying/setting quote prefix for the given (0-based) level
   bool HasQuoteAtLevel(size_t level) const
   {
      return m_quoteAtLevel.size() > level;
   }

   void SetQuoteAtLevel(size_t level, const String& s)
   {
      if ( level >= m_quoteAtLevel.size() )
      {
         ASSERT_MSG( level == m_quoteAtLevel.size(),
                        _T("should set quote prefixes in order") );

         m_quoteAtLevel.push_back(s);
      }
      else // modify existing quote
      {
         m_quoteAtLevel[level] = s;
      }
   }

   const String& GetQuoteAtLevel(size_t level) const
   {
      return m_quoteAtLevel[level];
   }


   // pointer to previous line we examined
   const char *linePrev;

   // set to > 0 if the next line is wrapped tail of this one
   int levelWrapped;

private:
   // array of quote markers for all quoting levels we have seen so far
   wxArrayString m_quoteAtLevel;
};

/**
    Count levels of quoting on the first line of passed string.

    It understands standard e-mail quoting methods such as ">" and "XY>".

    @param string the string to check
    @param max_white max number of white characters before quotation mark
    @param max_alpha max number of A-Z characters before quotation mark
    @param quoteData global quoting data, should be saved between function
                     calls for the same text
    @return number of quoting levels (0 for unquoted text)
  */
int
CountQuoteLevel(const char *string,
                int max_white,
                int max_alpha,
                QuoteData& quoteData);

#endif // _M_QUOTEDTEXT_H_
