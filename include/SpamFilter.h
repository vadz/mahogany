///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   SpamFilter.h
// Purpose:     SpamFilter is an abstract class for spam detection
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-07-04
// CVS-ID:      $Id$
// Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

/**
   @file SpamFilter.h
   @brief Declaration of SpamFilter class.

   SpamFilter is an abstract base class defining methods which should be
   implemented by all spam detection methods.
 */

#ifndef _M_SPAMFILTER_H_
#define _M_SPAMFILTER_H_

#include "MModule.h"

class Message;

/*
   Abstract base class for all concrete spam filters.

   This class also has static methods for working with the spam filters chain:
   all used spam filters are organized in a linked list and all operations work
   on all spam filters.
 */
class SpamFilter : public MObjectRC
{
public:
   /**
      Mark the message as spam.

      This method is used for training the spam filter and also for correcting
      its mistakes.

      @param msg the spam message
    */
   static void ClassifyAsSpam(const Message& msg)
      { ReclassifyAsSpam(msg, true); }

   /**
      Mark the message as non-spam.

      Unlike ClassifyAsSpam() this is normally only used to correct mistakes,
      i.e. for reclassifying a false positive.

      @param msg the ham (i.e. non-spam) message
    */
   static void ClassifyAsHam(const Message& msg)
      { ReclassifyAsSpam(msg, false); }

   /**
      Reclassify the message as either spam or ham.

      Common part of ClassifyAsSpam/Ham()

      @param msg the message to reclassify
      @param isSpam if true, reclassify as spam, otherwise as ham
    */
   static void ReclassifyAsSpam(const Message& msg, bool isSpam);

   /**
      Check if the message is a spam.

      @param msg the message to check
      @param probability if non NULL, return the probability of this message
                         being a spam, may be NULL if caller not interested
      @return true if the message is deemed to be a spam, false otherwise
    */
   static bool CheckIfSpam(const Message& msg, float *probability = NULL);

   /**
      Unload all loaded spam filters.

      This should be called at least before the program termination to free
      memory used by the spam filters but can also be called during the program
      execution to force reloading of the spam filters during the next call to
      ReclassifyAsSpam() or CheckIfSpam().
    */
   static void UnloadAll();

protected:
   /**
      Default ctor.
    */
   SpamFilter() { m_next = NULL; }

   /**
      Loads all available spam filters.

      This is called as needed by ReclassifyAsSpam()/CheckIfSpam(), no need to
      call it explicitly.
    */
   static inline void LoadAll()
   {
      if ( !ms_loaded )
         DoLoadAll();
   }


   /**
      @name Methods to be implemented in the derived classes.

      These methods provide a spam filter API. Client code doesn't have direct
      access to them because they're invoked on all spam filters at once via
      our static public methods.
    */
   //@{

   /// Reclassify a message as either spam or ham
   virtual void Reclassify(const Message& msg, bool isSpam) = 0;

   /// Process a message and return whether it is a spam
   virtual bool Process(const Message& msg, float *probability) = 0;

   //@}

private:
   // the meat of LoadAll(), only called if !ms_loaded and sets it to true
   // after doing its work
   static void DoLoadAll();


   // the head of the linked list of spam filters
   static SpamFilter *ms_first;

   // true if we had already loaded all available spam filters
   static bool ms_loaded;


   // the next filter in the linked list or NULL
   SpamFilter *m_next;
};


// ----------------------------------------------------------------------------
// Loading spam filters from modules support
// ----------------------------------------------------------------------------

class SpamFilterFactory : public MModule
{
public:
   /// create a new spam filter
   virtual SpamFilter *Create() const = 0;
};

// the spam filter module interface name
#define SPAM_FILTER_INTERFACE _T("SpamFilter")

/**
  This macro must be used in the .cpp file containing the implementation of a
  spam filter.

  @param cname    the name of the class (derived from SpamFilter)
  @param desc     the short description (shown to the user)
  @param cpyright the module author/copyright string
 */
#define IMPLEMENT_SPAM_FILTER(cname, desc, cpyright)                          \
   class cname##Factory : public SpamFilterFactory                            \
   {                                                                          \
   public:                                                                    \
      virtual SpamFilter *Create() const { return new cname; }                \
                                                                              \
      MMODULE_DEFINE();                                                       \
      DEFAULT_ENTRY_FUNC                                                      \
   };                                                                         \
   MMODULE_BEGIN_IMPLEMENT(cname##Factory, _T(#cname),                        \
                           SPAM_FILTER_INTERFACE, desc, _T("1.00"))           \
      MMODULE_PROP(_T("author"), cpyright)                                    \
   MMODULE_END_IMPLEMENT(cname##Factory)                                      \
   MModule *cname##Factory::Init(int /* version_major */,                     \
                                 int /* version_minor */,                     \
                                 int /* version_release */,                   \
                                 MInterface * /* minterface */,               \
                                 int * /* errorCode */)                       \
   {                                                                          \
      return new cname##Factory();                                            \
   }

#endif // _M_SPAMFILTER_H_

