///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MessageViewer.h: declaration of MessageViewer ABC
// Purpose:     MessageViewer is the interface which must be implemented by the
//              GUI viewers used by MessageView
// Author:      Vadim Zeitlin
// Modified by:
// Created:     24.07.01
// CVS-ID:      $Id$
// Copyright:   (c) 1997-2001 Mahogany team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MESSAGEVIEWER_H_
#define _MESSAGEVIEWER_H_

class WXDLLEXPORT wxBitmap;
class WXDLLEXPORT wxTextAttr;
class WXDLLEXPORT wxWindow;
class WXDLLEXPORT wxImage;

class ClickableInfo;
class MTextStyle;

#include "MModule.h"
#include "MessageView.h"      // we use MessageView in inline funcs below

#include <wx/gdicmn.h>        // for wxNullColour

// the message viewer module interface name
#define MESSAGE_VIEWER_INTERFACE "MessageViewer"

// ----------------------------------------------------------------------------
// MessageViewer: interface for GUI viewers
// ----------------------------------------------------------------------------

class MessageViewer
{
public:
   /** @name Creation, updating

       Before calling any other methods Create() must be caleld. It will
       really create the window (which can later be retrieved with GetWindow())
    */
   //@{

   /// create the window: this must be called before anything else!
   virtual void Create(MessageView *msgView, wxWindow *parent) = 0;

   /// clear the window
   virtual void Clear() = 0;

   /// update the window after clearing it
   virtual void Update() = 0;

   /// update the profile options which are not updated by reshowing a msg
   virtual void UpdateOptions() = 0;

   /// get the viewer window
   virtual wxWindow *GetWindow() const = 0;

   /// virtual dtor for the base class
   virtual ~MessageViewer() { }

   //@}

   /** @name Operations

       These methods directly correspond to the menu commands
    */
   //@{

   /// find the given string in the window
   virtual bool Find(const String& text) = 0;

   /// find the same string again (i.e. resume search)
   virtual bool FindAgain() = 0;

   /// select the entire contents of the viewer
   virtual void SelectAll() = 0;

   /// return the selection
   virtual String GetSelection() const = 0;

   /// copy selection to clipboard
   virtual void Copy() = 0;

   /// print the window contents
   virtual bool Print() = 0;

   /// show the print preview
   virtual void PrintPreview() = 0;

   //@}

   /** @name Headers

       These methods are called by MessageView to show various message headers.
    */
   //@{

   /// start showing headers
   virtual void StartHeaders() = 0;

   /// show all headers in the raw form
   virtual void ShowRawHeaders(const String& header) = 0;

   /// start showing the header
   virtual void ShowHeaderName(const String& name) = 0;

   /// insert some text as the header value
   virtual void ShowHeaderValue(const String& value,
                                wxFontEncoding encoding) = 0;

   /// insert an URL which is part of the header value
   virtual void ShowHeaderURL(const String& text,
                              const String& url) = 0;

   /// end of header
   virtual void EndHeader() = 0;

   /// show the X-Face for this message
   virtual void ShowXFace(const wxBitmap& bitmap) = 0;

   // finish showing headers
   virtual void EndHeaders() = 0;

   //@}


   /** @name Body

       These methods are called by MessageView sequentially to build the
       message body on screen. The viewer may only store the information they
       provide internally and show it when EndBody() is called.
    */
   //@{

   /// start showing the body
   virtual void StartBody() = 0;

   /// start showing a new MIME part
   virtual void StartPart() = 0;

   /// insert an arbitrary active object (takes ownership of ClickableInfo)
   virtual void InsertClickable(const wxBitmap& icon,
                                ClickableInfo *ci,
                                const wxColour& col = wxNullColour) = 0;

   /// insert an attechment (takes ownership of ClickableInfo)
   virtual void InsertAttachment(const wxBitmap& icon, ClickableInfo *ci) = 0;

   /// insert an inline image (only called if CanInlineImages() returned true)
   virtual void InsertImage(const wxImage& image, ClickableInfo *ci) = 0;

   /// insert raw message contents (only called if CanProcess() returned true)
   virtual void InsertRawContents(const String& data) = 0;

   /// insert a chunk of text and show it in this style
   virtual void InsertText(const String& text, const MTextStyle& style) = 0;

   /// insert an URL
   virtual void InsertURL(const String& text, const String& url) = 0;

   /// mark the end of the MIME part
   virtual void EndPart() = 0;

   /// mark the end of body
   virtual void EndBody() = 0;

   //@}


   /** @name Scrolling
    */
   //@{

   /// scroll line down, return false if already at bottom
   virtual bool LineDown() = 0;

   /// scroll line up, return false if already at top
   virtual bool LineUp() = 0;

   /// scroll page down, return false if already at bottom
   virtual bool PageDown() = 0;

   /// scroll page up, return false if already at top
   virtual bool PageUp() = 0;

   //@}

   /** @name Capabilities querying

       All viewers are supposed to be able to show the text and the attachments
       somehow but they may have extra features as well. MessageView queries
       them to find the best way to show the given MIME part.
    */
   //@{

   /// can this viewer show inline images? if not, show them as attachments
   virtual bool CanInlineImages() const = 0;

   /// can this viewer process this part directly?
   virtual bool CanProcess(const String& mimetype) const = 0;

   //@}

protected:
   /** @name Accessorts to MessageView data

       We provide these methods for derived class usage which allows us to make
       them private in MessageView and this class is its friend (unlike any
       derived ones)
    */
   //@{

   typedef MessageView::AllProfileValues ProfileValues;

   /// get the profile to use (NOT IncRef()'d!)
   Profile *GetProfile() const { return m_msgView->GetProfile(); }

   /// get the msg view options
   const ProfileValues& GetOptions() const
      { return m_msgView->GetProfileValues(); }

   //@}

   /// protected ctor as the objects of this class are never created directly
   MessageViewer() { m_msgView = NULL; }

   // back pointer to the message view (we need its profile)
   MessageView *m_msgView;
};

// ----------------------------------------------------------------------------
// Loading the viewers from the modules
// ----------------------------------------------------------------------------

class MessageViewerFactory : public MModule
{
public:
   virtual MessageViewer *Create() = 0;
};

// this macro must be used inside MessageViewer-derived class declaration
//
// not any more
#define DECLARE_MESSAGE_VIEWER()

// parameters of this macro are:
//    cname    - the name of the class (derived from MessageViewer)
//    desc     - the short description shown in the viewers dialog
//    cpyright - the module author/copyright string
#define IMPLEMENT_MESSAGE_VIEWER(cname, desc, cpyright)                    \
   class cname##Factory : public MessageViewerFactory                      \
   {                                                                       \
   public:                                                                 \
      virtual MessageViewer *Create() { return new cname; }                \
                                                                           \
      MMODULE_DEFINE();                                                    \
      DEFAULT_ENTRY_FUNC;                                                  \
   };                                                                      \
   MMODULE_BEGIN_IMPLEMENT(cname##Factory, #cname,                         \
                           MESSAGE_VIEWER_INTERFACE, desc, "1.00")         \
      MMODULE_PROP(MMODULE_AUTHOR_PROP, cpyright)                          \
   MMODULE_END_IMPLEMENT(cname##Factory)                                   \
   MModule *cname##Factory::Init(int /* version_major */,                  \
                                 int /* version_minor */,                  \
                                 int /* version_release */,                \
                                 MInterface * /* minterface */,            \
                                 int * /* errorCode */)                    \
   {                                                                       \
      return new cname##Factory();                                         \
   }

#endif // _MESSAGEVIEWER_H_

