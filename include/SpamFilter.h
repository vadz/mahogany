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

      This method is used for correcting spam filter mistakes, see Train() for
      training the spam filter.

      @param msg the spam message
    */
   static void ClassifyAsSpam(const Message& msg)
      { Reclassify(msg, true); }

   /**
      Mark the message as non-spam.

      Like ClassifyAsSpam() this is normally only used to correct mistakes,
      i.e. for reclassifying a false positive.

      @param msg the ham (i.e. non-spam) message
    */
   static void ClassifyAsHam(const Message& msg)
      { Reclassify(msg, false); }

   /**
      Reclassify the message as either spam or ham.

      Common part of ClassifyAsSpam/Ham().

      Note that this should only be used if the spam filter had accidentally
      detected that the message was a spam, not to train it.


      @param msg the message to reclassify
      @param isSpam if true, reclassify as spam, otherwise as ham
    */
   static void Reclassify(const Message& msg, bool isSpam);

   /**
      Train the spam filter using the specified spam or ham message.

      Unlike Reclassify() which should only be used for the messages
      which had been mistakenly misclassified by the filter, this method should
      be used for the messages which had never been seen by the spam filter
      before.

      @param msg the message to train the filter with
      @param isSpam if true, this as a spam, otherwise -- ham
    */
   static void Train(const Message& msg, bool isSpam);

   /**
      Check if the message is a spam.

      @param msg the message to check
      @param params array containing the stringified options for the filters
                    (each element is of the form "name=options" where "name" is
                    the name of the filter and options depend on filter)
      @param result place holder for the message explaining why the message was
                    deemed to be spam, if the filter can tell it, ignored if
                    the return value is false
      @return true if the message is deemed to be a spam, false otherwise
    */
   static bool CheckIfSpam(const Message& msg,
                           const wxArrayString& params,
                           String *result);

   /**
      Unload all loaded spam filters.

      This should be called at least before the program termination to free
      memory used by the spam filters but can also be called during the program
      execution to force reloading of the spam filters during the next call to
      Reclassify() or CheckIfSpam().
    */
   static void UnloadAll();

protected:
   /**
      Default ctor.
    */
   SpamFilter() { m_next = NULL; }

   /**
      Loads all available spam filters.

      This is called as needed by Reclassify()/CheckIfSpam(), no need to
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

   /**
      Reclassify a message as either spam or ham.

      This is used by the public Reclassify().
    */
   virtual void DoReclassify(const Message& msg, bool isSpam) = 0;

   /**
      Train filter using this message.

      This is used by the public Train().
    */
   virtual void DoTrain(const Message& msg, bool isSpam) = 0;

   /**
      Process a message and return whether it is a spam.

      This is used by the public CheckIfSpam(). The parameter passed to it is
      the RHS of the element in the params array passed to CheckIfSpam() having
      the same name as this spam filter.
    */
   virtual bool DoCheckIfSpam(const Message& msg,
                              const String& param,
                              String *result) = 0;

   /**
      Return the internal name of this spam filter.

      This method is implemented by DECLARE_SPAM_FILTER() macro below.
    */
   virtual const wxChar *GetName() const = 0;

   /**
      Return the computational/speed cost of using this filter.

      Cheaper filters are applied first, i.e. those which run the fastest are
      applied before the slower ones.

      This method is implemented by DECLARE_SPAM_FILTER() macro below.
    */
   virtual unsigned int GetCost() const = 0;

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
  This macro must be used in SpamFilter-derived class declaration.

  It defines GetName() and GetCost() methods.

  There should be no semicolon after this macro.

  @param name short name of the filter
  @param cost performance cost of using this filter, from 0 to +infinity
              (cheaper filters are applied first)
 */
#define DECLARE_SPAM_FILTER(name, cost)                                       \
   public:                                                                    \
      virtual unsigned int GetCost() const { return cost; }                   \
      virtual const wxChar *GetName() const { return _T(name); }

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

