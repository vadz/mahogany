/*-*- c++ -*-********************************************************
 * SendMessageCC.h : class for holding messages during composition  *
 *                                                                  *
 * (C) 1998,1999 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/



#ifndef SENDMESSAGECC_H
#define SENDMESSAGECC_H

#ifdef   __GNUG__
#   pragma interface "SendMessageCC.h"
#endif

class ProfileBase;

/// another scandoc fix
#define   SM_INLINE "INLINE"

/** A class representing a message during composition.
 */

class SendMessageCC
{
   ProfileBase 	*profile;

   ENVELOPE	*env;
   BODY		*body;
   PART		*nextpart, *lastpart;
public:
   /** SendMessageCC supports two different protocols:
    */
   enum Protocol { Prot_SMTP, Prot_NNTP, Prot_Default = Prot_SMTP };
   
   /** Creates an empty object, setting some initial values.
       @param iprof optional pointer for a parent profile
       @param protocol which protocol to use for sending
   */
   SendMessageCC(ProfileBase *iprof,
                 Protocol protocol = Prot_Default);

   /** Sets the message subject.
       @param subject the subject
   */
   void SetSubject(const String &subject);
   
   /** Sets the address fields, To:, CC: and BCC:.
       @param To primary address to send mail to
       @param CC carbon copy addresses
       @param BCC blind carbon copy addresses
   */
   void SetAddresses(const String &To,
                     const String &CC = "",
                     const String & = "");
   
   void SetNewsgroups(const String &groups);
   
   /** Get the profile.
       @return pointer to the profile
   */
   inline ProfileBase *GetProfile(void) { return profile; }
   /** Adds a part to the message.
       @param type numeric mime type
       @param buf  pointer to data
       @param len  length of data
       @param subtype if not empty, mime subtype to use
       @param disposition either INLINE or ATTACHMENT
       @param dlist list of disposition parameters
       @param plist list of parameters
   */
   void	AddPart(MessageContentType type,
                 const char *buf, size_t len,
                const String &subtype = M_EMPTYSTRING,
                const String &disposition = SM_INLINE,
                MessageParameterList const *dlist = NULL,
                MessageParameterList const *plist = NULL);

   /** Writes the message to a String
       @param output string to write to
   */
   void WriteToString(String  &output);

   /** Writes the message to a file
       @param filename file where to write to
       @param append if false, overwrite existing contents
   */
   void WriteToFile(const String &filename, bool append = true);
   
   /** Writes the message to a folder.
       @param foldername file where to write to
       @param type folder type
   */
   void WriteToFolder(const String &foldername,
                      MailFolder::Type type = MF_PROFILE );

   /** Adds an extra header line.
       @param entry name of header entry
       @param value value of header entry
   */
   void AddHeaderEntry(const String &entry, const String &value);
   
   /** Sends the message.
       @return true on success
   */
   bool Send(void);

   /// destructor
   ~SendMessageCC();

   enum Mode
   { Mode_SMTP, Mode_NNTP };
protected:
   /** Builds the message, i.e. prepare to send it. */
   void Build(void);
private:
   /// 2nd stage constructor, see constructor
   void	Create(Protocol protocol, ProfileBase *iprof);
   /// Protocol used for sending
   Protocol m_protocol;
   /** @name variables managed by Build() */
   //@{
   /// names of header lines
   const char **m_headerNames;
   /// values of header lines
   const char **m_headerValues;
   /// A list of all headers to be looked up in profile for addition.
   kbStringList m_headerList;
   /// A list of all extra headerslines to add to header.
   kbStringList m_ExtraHeaderLinesNames;
   /// A list of all extra headerslines to add to header.
   kbStringList m_ExtraHeaderLinesValues;

   /// a list of folders to save copies in
   kbStringList m_FccList;
   /// Parses string for folder aliases, removes them and stores them in m_FccList.
   void ExtractFccFolders(String &addresses);
//@}
   
};


#endif
