class SendMessageCC
{
   ProfileBase 	*profile;

   ENVELOPE	*env;
   BODY		*body;
   PART		*nextpart, *lastpart;
public:
   SendMessageCC(ProfileBase *iprof = NULL);
   SendMessageCC(ProfileBase *iprof, String const &subject,
		 String const &to, String const &cc, String const
		 &bcc);
   void Create(ProfileBase *iprof = NULL);
   void	Create(ProfileBase *iprof, String const &subject,
	       String const &to, String const &cc, String const &bcc);

   inline ProfileBase *GetProfile(void) { return profile; }
   void	AddPart(int type, const char *buf, size_t len,
		String const &subtype = "");
   void Send(void);
   ~SendMessageCC();
};
