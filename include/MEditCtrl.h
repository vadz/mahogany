/*-*- c++ -*-********************************************************
 * MEditCtrl class: API for message display and edit controls     *
 *                                                                  *
 * (C) 2000 by Karsten Ballüder (ballueder@gmx.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/
#ifndef MEDITCTRL_H
#define MEDITCTRL_H

#ifdef __GNUG__
#   pragma interface "MEditCtrl.h"
#endif

#include "Mcommon.h"
#include "MObject.h"
#include "strutil.h"

/** A small helper class holding some data.
    MimeContent holds the following:
    - a MIME type specification string
    - a reference number which is used by whoever creates it
      (called partno as it usually refers to a message part)
*/
class MimeContent : public MObject
{
public:
   /// create a text object
   MimeContent(const String &text, UIdType partno)
      {
         Create("text/plain",text, partno);
      }
   /// create a text object of a non-standard type
   MimeContent(const String &type, const String &text, UIdType partno)
      {
         Create(type, text, partno);
      }
   /** create a data object with some raw, non text data
       Takes control of the data pointer
   */
   MimeContent(const String &type, void * data, size_t len, UIdType partno)
      {
         m_Data = data;
         m_Len = len;
         m_PartNo = partno;
         SetType(type);
      }
   /// return the mime type
   const String & GetType(void) const
      { MOcheck(); return m_Type; }
   const wxString & GetText(void) const
      { MOcheck(); return m_Text; }
   /// return the part number
   UIdType GetPartNumber() const { MOcheck(); return m_PartNo; }

   const String &GetFileName(void) const { return m_FileName; }

   const void *GetData(void) const
      {
         MOcheck();
         if(m_Data)
            return m_Data;
         else
            return m_Text.c_str();
      }
   size_t GetSize(void) const
      {
         MOcheck();
         if(m_Data)
            return m_Len;
         else
            return m_Text.Length();
      }
   void SetFileName(const String &fn)
      {
         MOcheck();
         m_FileName = fn;
      }

   void SetType(const String &t)
      {
         MOcheck();
         m_Type = t;
         strutil_tolower(m_Type);
      }
   void SetDisposition(const String &d)
      {
         MOcheck();
         m_Disposition = d;
         strutil_tolower(m_Disposition);
      }
   const String &GetDisposition(void) const
      {
         return m_Disposition;
      }

   ~MimeContent()
      {
         MOcheck();
         FreeData();
      }
   void MakeDataCopy(void)
      {
         MOcheck();
         CHECK_RET(m_Data != NULL && m_Len > 0, "attempt to copy NULL data");
         void *newdata = malloc(m_Len);
         memcpy(newdata, m_Data, m_Len);
         m_Data = newdata;
      }
protected:
   void FreeData()
      {
         MOcheck();
         if(m_Data) free(m_Data);
         m_Data = NULL;
         m_Len = 0;
      }
   void Create(const String &type, const String &text, UIdType partno)
      {
         m_Data = NULL;
         m_Len = 0;
         m_Type = type;
         m_Text = text;
         m_PartNo = partno;
      }
private:
   String m_Type;
   UIdType m_PartNo;
   String m_FileName;
   String m_Disposition;
   String m_Text;
   void *m_Data;
   size_t m_Len;
};


/** This class interfaces with the edit control for callbacks. */
class MEditCtrlCbHandler : public MObject
{
public:
   /**@name Callbacks for editor control */
   //@{
   /// when clicked on mimepart
   virtual void OnMouseLeft(UIdType partno) = 0;
   /// when right-clicked on mimepart
   virtual void OnMouseRight(UIdType partno) = 0;
   /// when clicked on url
   virtual void OnUrl(const wxString &url) = 0;
   //@}
   virtual ~MEditCtrlCbHandler() {}
};

class MEditCtrl : public MObject
{
public:
   /// clear the control
   virtual void Clear(void) = 0;

   /** Redraws the window. */
   virtual void RequestUpdate(void) = 0;

   /** Enable redrawing while we add content */
   virtual void EnableAutoUpdate(bool enable = TRUE) = 0;

   /** Set dirty flag */
   virtual void SetDirty(bool dirty = TRUE) = 0;
   virtual bool GetDirty(void) const = 0;
   
   /** This inserts some mime content of a non-text type into the
       control. 
   */
   virtual void Append(const MimeContent *mc) = 0;
   /** Bog-standard text-append to the control. Text is treated as
       "text/plain" */
   virtual void Append(const String &txt) = 0;

   virtual bool Find(const wxString &needle,
                     wxPoint * fromWhere = NULL,
                     const wxString &configPath = "MsgViewFindString")
      = 0;

   /**@name Clipboard interaction */
   //@{
   /// Pastes text from clipboard.
   virtual void Paste(bool privateFormat = FALSE,
                      bool usePrimarySelection = FALSE) = 0;
   /** Copies selection to clipboard.
       @param invalidate used internally, see wxllist.h for details
   */
   virtual bool Copy(bool invalidate = true,
                     bool privateFormat = FALSE, bool primary = FALSE) = 0;
   /// Copies selection to clipboard and deletes it.
   virtual bool Cut(bool privateFormat = FALSE,
                    bool usePrimary = FALSE) = 0;
   //@}

   /// prints the currently displayed message
   virtual void Print(void) = 0;
   /// print-previews the currently displayed message
   virtual void PrintPreview(void) = 0;

   virtual ~MEditCtrl() {}
private:
};
#endif
