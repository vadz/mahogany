// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////
// Project:     M - cross platform e-mail GUI client
// File name:   adb/ProvPalm.h 
// Purpose:     Palm-ADBProvider defines an interface for the Palm ADB
// Author:      Daniel Seifert
// Modified by:
// Created:     18.02.2000
// CVS-ID:      
// Copyright:   (c) 2000 Daniel Seifert <dseifert@gmx.de>
// Licence:     M license
// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////

#ifndef   _PROVPALM_H
#define   _PROVPALM_H

#include "adb/AdbEntry.h"    // the base class declaration
#include "adb/AdbBook.h"

// fwd decl
class PalmEntry;
class PalmEntryGroup;
class PalmBook;

class PalmEntry : public AdbEntryStoredInMemory {
public:
  PalmEntry(void);
  PalmEntry(PalmEntryGroup *pGroup, const String& strName, bool bNew = FALSE);
  virtual AdbEntryGroup *GetGroup() const;
  void Load(struct Address);
    
  const char *GetName() const;
  bool IsOk() const;
};

class PalmEntryGroup : public AdbEntryGroup {
public:
  PalmEntryGroup(PalmEntryGroup *pParent, const wxString& strName, bool bNew);
  PalmEntryGroup(void);
  virtual AdbEntryGroup *GetGroup() const;
  virtual String GetName() const;

  virtual size_t GetEntryNames(wxArrayString& aNames) const;
  virtual size_t GetGroupNames(wxArrayString& aNames) const;
  virtual AdbEntry *GetEntry(const String& name) const;
  virtual AdbEntryGroup *GetGroup(const String& name) const;
  virtual bool Exists(const String& path) const;
  virtual void AddEntry(PalmEntry* p_Entry);
  virtual AdbEntry *CreateEntry(const String& strName);
  virtual AdbEntryGroup *CreateGroup(const String& strName);
  virtual void DeleteEntry(const String& strName);
  virtual void DeleteGroup(const String& strName);
  virtual AdbEntry *FindEntry(const char *szName);
};

class PalmBook : public AdbBook {
public:
  PalmBook(const String& filename);
  virtual AdbEntryGroup *GetGroup() const { return NULL; }
  virtual AdbEntry *GetEntry(const String& name) const;
  virtual bool Exists(const String& path) const;
  virtual size_t GetEntryNames(wxArrayString& aNames) const;
  virtual size_t GetGroupNames(wxArrayString& aNames) const;
  virtual AdbEntryGroup *GetGroup(const String& name) const;
  virtual AdbEntry *CreateEntry(const String& strName);
  virtual AdbEntryGroup *CreateGroup(const String& strName);
  virtual void DeleteEntry(const String& strName);
  virtual void DeleteGroup(const String& strName);
  virtual AdbEntry *FindEntry(const char *szName);
  virtual bool IsSameAs(const String& name) const;
  virtual String GetName() const;
  virtual void SetUserName(const String& name);
  virtual String GetUserName() const;

  virtual void SetDescription(const String& desc);
  virtual String GetDescription() const;

  virtual size_t GetNumberOfEntries() const;

  virtual bool IsLocal() const;
  virtual bool IsReadOnly() const;
};

#endif  //_PROVPALM_H
