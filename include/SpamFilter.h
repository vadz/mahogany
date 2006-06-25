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

#include "gui/listbook.h"

class Message;
class SpamOptionsPage;

class WXDLLEXPORT wxFrame;


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

      If param argument is empty, all filters are used with default options.
      Otherwise only the filters whose names appear in "param" are used so if
      you want the dspam filter to be used you must have "dspam=" in the param
      string, i.e. it must be present even though it currently has no options.

      @param msg the message to check
      @param param either empty or contains the stringified options for the
                   filters of the form "name1[=options1];...;nameN[=optionsN]"
                   where "name"s are the same as GetName() return values and
                   if options are not specified the defaults are used
      @param result place holder for the message explaining why the message was
                    deemed to be spam, if the filter can tell it, ignored if
                    the return value is false (may be NULL)
      @return true if the message is deemed to be a spam, false otherwise
    */
   static bool CheckIfSpam(const Message& msg,
                           const String& param = wxEmptyString,
                           String *result = NULL);

   /**
      Show a GUI dialog allowing the user to configure all spam filters.

      This function shows the global filter dialog.

      @param parent the parent frame for the dialog
      @return true if something was changed, false otherwise
    */
   static bool Configure(wxFrame *parent);

   /**
      Allow the user to edit the parameters of a spam rule.

      In the future this should allow specifying the options for the spam
      filters which have any here as well, but for now we only allow to select
      which filters are used and their order.

      @param parent the parent frame for the dialog
      @param params in/out parameters describing the filters options (must not
                    be NULL)
      @return true if something was changed, false otherwise
    */
   static bool EditParameters(wxFrame *parent, String *params);

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
      Virtual dtor for the base class.
    */
   virtual ~SpamFilter();

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
      Find filter by name.

      @param name the (internal) name to look for
      @return filter with given name or NULL if not found
    */
   static SpamFilter *FindByName(const String& name);

   /**
      Find filter by long name.

      @param lname the long name to look for
      @return filter with given name or NULL if not found
    */
   static SpamFilter *FindByLongName(const String& lname);


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

      @return true if this is a spam, false if we don't think it's a spam and
              -1 if we're sure this is not a spam
    */
   virtual int DoCheckIfSpam(const Message& msg,
                             const String& param,
                             String *result) = 0;

   /**
      Return the name of the icon used by the option page.

      This is rather ugly but unfortunately we need to have all the icons names
      first (to initialize the notebook image list) and we need a notebook in
      CreateOptionPage() so we can't combine these 2 functions into 1.

      @return the name of the icon to use or NULL
    */
   virtual const wxChar *GetOptionPageIconName() const { return NULL; }

   /**
      Return a pointer to the option page used for editing this spam filter
      options.

      @param notebook the parent for the page
      @param profile the profile to use for the page settings (no need to
                     Inc/DecRef() it)
      @return pointer to the page (which will be deleted by the caller) ot NULL
    */
   virtual SpamOptionsPage *CreateOptionPage(wxListOrNoteBook *notebook,
                                             Profile *profile) const;

   /**
      Return the name of the filter as shown to the user.
     */
   virtual String GetLongName() const = 0;

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

   friend class SpamOptionsDialog;
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

  @param name short name of the filter (internal)
  @param lname long name of the filter (shown to the user)
  @param cost performance cost of using this filter, from 0 to +infinity
              (cheaper filters are applied first)
 */
#define DECLARE_SPAM_FILTER(name, lname, cost)                                \
   public:                                                                    \
      virtual unsigned int GetCost() const { return cost; }                   \
      virtual const wxChar *GetName() const { return _T(name); }              \
      virtual String GetLongName() const { return lname; }

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

