/*-*- c++ -*-********************************************************
 * SendMessageCC.h : class for holding messages during composition  *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef SENDMESSAGECC_H
#define SENDMESSAGECC_H

#ifdef   __GNUG__
#   pragma interface "SendMessageCC.h"
#endif

#include   "MessageCC.h"
#include   "Profile.h"

/** A class representing a message during composition.
 */

class SendMessageCC
{
   ProfileBase 	*profile;

   ENVELOPE	*env;
   BODY		*body;
   PART		*nextpart, *lastpart;
public:
   /** Creates an empty object.
       @param iprof optional pointer for a parent profile
   */
   SendMessageCC(ProfileBase *iprof = NULL);
   /** Creates an empty object, setting some initial values.
       @param iprof optional pointer for a parent profile
       @param subject message subject
       @param to message To: field
       @param cc message CC: field
       @param bcc message BCC: field
   */
   SendMessageCC(ProfileBase *iprof, String const &subject,
		 String const &to, String const &cc, String const
		 &bcc);

   /** Get the profile.
       @return pointer to the profile
   */
   inline ProfileBase *GetProfile(void) { return profile; }
   /** Adds a part to the message.
       @param type numeric mime type
       @param buf  pointer to data
       @param len  length of data
       @subtype if not empty, mime subtype to use
   */
   void	AddPart(int type, const char *buf, size_t len,
		String const &subtype = "");
   /// Sends the message.
   void Send(void);

   /// destructor
   ~SendMessageCC();

 private:
   /// 2nd stage constructor, see constructor
   void Create(ProfileBase *iprof = NULL);
   /// 2nd stage constructor, see constructor
   void	Create(ProfileBase *iprof, String const &subject,
	       String const &to, String const &cc, String const &bcc);
};


#endif
