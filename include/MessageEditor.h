///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MessageEditor.h: declaration of MessageEditor ABC
// Purpose:     MessageEditor is the interface which must be implemented by the
//              GUI viewers used by Composer
// Author:      Vadim Zeitlin
// Modified by:
// Created:     13.03.02
// CVS-ID:      $Id$
// Copyright:   (c) 1997-2002 Mahogany team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MESSAGEEDITOR_H_
#define _MESSAGEEDITOR_H_

class WXDLLEXPORT wxBitmap;
class WXDLLEXPORT wxPoint;
class WXDLLEXPORT wxTextAttr;
class WXDLLEXPORT wxWindow;

class EditorContentPart;
class Composer;
class Profile;
struct ComposerOptions;

#include <wx/fontenc.h>         // for wxFontEncoding

#include "MimePart.h"         // for MimeType

#include "MModule.h"

// the message editor module interface name
#define MESSAGE_EDITOR_INTERFACE "MessageEditor"

// ----------------------------------------------------------------------------
// MessageEditor: interface for an editor used by the composer
// ----------------------------------------------------------------------------

class MessageEditor
{
public:
   /**
     @name Constants
   */
   //@{

   /// possible values for the 2nd parameter of InsertText()
   enum InsertMode
   {
      /// append the text to the existing contents
      Insert_Append,

      /// replace the (text part of the) current contents with text
      Insert_Replace,

      /// insert text at the cursor position
      Insert_Insert
   };

   //@}

   /**
     @name Creation, updating

     Before calling any other methods Create() must be caleld. It will
     really create the window (which can later be retrieved with GetWindow())
    */
   //@{

   /// create the window: this must be called before anything else!
   virtual void Create(Composer *composer, wxWindow *parent) = 0;

   /// update the profile options which are not updated by reshowing a msg
   virtual void UpdateOptions() = 0;

   /// can we send or save the message?
   virtual bool FinishWork() = 0;
   
   /// virtual dtor for the base class
   virtual ~MessageEditor() { }

   //@}

   /**
     @name Accessors
   */

   //@{

   /// get the (main) editor window
   virtual wxWindow *GetWindow() const = 0;

   /// were we modified by the user?
   virtual bool IsModified() const = 0;

   /// do we contain anything at all?
   virtual bool IsEmpty() const = 0;

   /// compute the text hash
   virtual unsigned long ComputeHash() const = 0;

   //@}

   /** @name Operations

       These methods directly correspond to the menu commands
    */
   //@{

   /// clear the window
   virtual void Clear() = 0;

   /// enable or disable editing
   virtual void Enable(bool enable) = 0;

   /// reset the dirty flag, i.e. calling IsDirty() should return false now
   virtual void ResetDirty() = 0;

   /// set the encoding to use for the text in the editor
   virtual void SetEncoding(wxFontEncoding encoding) = 0;

   /// copy selection to clipboard
   virtual void Copy() = 0;

   /// cut selection to clipboard
   virtual void Cut() = 0;

   /// paste selection from clipboard
   virtual void Paste() = 0;

   /// print the window contents
   virtual bool Print() = 0;

   /// show the print preview
   virtual void PrintPreview() = 0;

   /// move the cursor to the initial position
   virtual void MoveCursorTo(unsigned long x, unsigned long y) = 0;

   /// move the cursor by the given displacement
   virtual void MoveCursorBy(long x, long y) = 0;

   /// set focus to the window where the user may type the text
   virtual void SetFocus() = 0;

   //@}

   /** @name Content

       The InsertXXX() methods are called by MessageEditor when the user
       chooses to attach a file or insert it as text.

       The GetXXX() are used to build the text of the message to send or to
       save.
    */
   //@{

   /// insert an attechment (takes ownership of EditorContentPart)
   virtual void InsertAttachment(const wxBitmap& icon, EditorContentPart *mc) = 0;

   /// insert a chunk of text, appending to or replacing the existing text
   virtual void InsertText(const String& text, InsertMode insMode) = 0;

   /// get the first part, returns NULL if the editor is empty
   virtual EditorContentPart *GetFirstPart() = 0;

   /// get the next part, returns NULL if no more
   virtual EditorContentPart *GetNextPart() = 0;

   /// edit the properties of the given attachment when the user clicks on it
   virtual void EditAttachmentProperties(EditorContentPart *part);

   /// show popup menu for an (already attached) attachment
   virtual void ShowAttachmentMenu(EditorContentPart *part, const wxPoint& pt);

   //@}

protected:
   /**
     @name Accessorts to Composer data

     We provide these methods for derived class usage which allows us to make
     them private in Composer and this class is its friend (unlike any derived
     ones)
    */
   //@{

   /// get the profile to use (NOT IncRef()'d!)
   Profile *GetProfile() const;

   /// get the msg view options
   const ComposerOptions& GetOptions() const;

   /// call the Composer method on derived class behalf
   bool OnFirstTimeFocus();

   /// call the Composer method on derived class behalf
   void OnFirstTimeModify();

   //@}

   /// protected ctor as the objects of this class are never created directly
   MessageEditor() { m_composer = NULL; }

   /// back pointer to the composer (we need its profile), must be set manually
   Composer *m_composer;
};

// ----------------------------------------------------------------------------
// EditorContentPart represents text or an attachement in the composer
// ----------------------------------------------------------------------------

class EditorContentPart : public MObjectRC
{
public:
   // constants
   enum EditorContentType
   {
      Type_None,
      Type_File,
      Type_Data,
      Type_Text
   };

   /**
      @name Ctor & dtor
   */
   //@{

   /// default ctor leaves object in an invalid state, call SetXXX() later
   EditorContentPart() { Init(); m_Type = Type_None; }

   /// ctor for text contents
   EditorContentPart(const String& text,
                     wxFontEncoding encoding = wxFONTENCODING_SYSTEM)
      { Init(); SetText(text); m_encoding = encoding; }

   //@}

   /**
      @name Initializion functions
   */
   //@{

   /// give us the data to attach, we will free() it (must be !NULL)
   void SetData(void *data, size_t length, const wxChar* name = NULL, const wxChar *filename = NULL);

   /// give us a file to attach - will be done only when we'll be sent
   void SetFile(const String& filename);

   /// give us the text we represent
   void SetText(const String& text);

   //@}

   /**
      @name Other setters
    */
   //@{

   /// set the MIME type for this attachment, must be called if !Type_Text
   void SetMimeType(const String& mimeType);

   /// set the name for Content-Disposition (same as filename by default)
   void SetName(const String& name);

   /// set disposition parameter of Content-Disposition header
   void SetDisposition(const String& disposition);

   //@}

   /**
      @name Accessors
    */
   //@{

   /// get the attachment type
   EditorContentType GetType() const { return m_Type; }

   /// get the MIME type object
   const MimeType& GetMimeType() const { return m_MimeType; }

   /// get the filename, may be empty
   const String& GetFileName() const { return m_FileName; }

   /// get the name, may be empty
   const String& GetName() const { return m_Name; }

   /// get our disposition string
   const String& GetDisposition() const { return m_Disposition; }

   /// get our text, only valid for text parts
   const String& GetText() const
   {
      ASSERT_MSG( m_Type == Type_Text, _T("this attachment doesn't have any text") );

      return m_Text;
   }

   /// get our text length, only valid for text parts
   size_t GetLength() const
   {
      ASSERT_MSG( m_Type == Type_Text, _T("this attachment doesn't have any text") );

      return m_Text.length();
   }

   /// get the encoding (ISO8859-1, KOI8-R, UTF-8, ...) of the text part
   wxFontEncoding GetEncoding() const
   {
      ASSERT_MSG( m_Type == Type_Text, _T("this attachment doesn't have any text") );

      return m_encoding;
   }

   /// get the pointer to attachment data, only valid for data attachments
   const void *GetData() const
   {
      ASSERT_MSG( m_Type == Type_Data, _T("this attachment doesn't have any data") );

      return m_Data;
   }

   /// get the size of the attachment data, only valid for data attachments
   size_t GetSize() const
   {
      ASSERT_MSG( m_Type == Type_Data, _T("this attachment doesn't have any data") );

      return m_Length;
   }

   //@}

private:
   // common part of all ctors
   void Init();

   // private dtor, we're ref counted
   virtual ~EditorContentPart();

   EditorContentType m_Type;

   void     *m_Data;
   size_t    m_Length;
   String    m_FileName,
             m_Name,
             m_Disposition,
             m_Text;

   MimeType m_MimeType;
   wxFontEncoding m_encoding;

   MOBJECT_DEBUG(EditorContentPart)

   GCC_DTOR_WARN_OFF
};

// ----------------------------------------------------------------------------
// Loading the viewers from the modules
// ----------------------------------------------------------------------------

class MessageEditorFactory : public MModule
{
public:
   virtual MessageEditor *Create() = 0;
};

// this macro must be used inside MessageEditor-derived class declaration
//
// not any more
#define DECLARE_MESSAGE_EDITOR()

// parameters of this macro are:
//    cname    - the name of the class (derived from MessageEditor)
//    desc     - the short description shown in the viewers dialog
//    cpyright - the module author/copyright string
#define IMPLEMENT_MESSAGE_EDITOR(cname, desc, cpyright)                    \
   class cname##Factory : public MessageEditorFactory                      \
   {                                                                       \
   public:                                                                 \
      virtual MessageEditor *Create() { return new cname; }                \
                                                                           \
      MMODULE_DEFINE();                                                    \
      DEFAULT_ENTRY_FUNC;                                                  \
   };                                                                      \
   MMODULE_BEGIN_IMPLEMENT(cname##Factory, #cname,                         \
                           MESSAGE_EDITOR_INTERFACE, desc, "1.00")         \
      MMODULE_PROP("author", cpyright)                                     \
   MMODULE_END_IMPLEMENT(cname##Factory)                                   \
   MModule *cname##Factory::Init(int /* version_major */,                  \
                                 int /* version_minor */,                  \
                                 int /* version_release */,                \
                                 MInterface * /* minterface */,            \
                                 int * /* errorCode */)                    \
   {                                                                       \
      return new cname##Factory();                                         \
   }

#endif // _MESSAGEEDITOR_H_

