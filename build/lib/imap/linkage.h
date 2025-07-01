
/* Minimal linkage.h for Unix builds */
extern DRIVER imapdriver;
extern DRIVER nntpdriver;
extern DRIVER pop3driver;
extern DRIVER dummydriver;
#define CREATEPROTO dummyproto
#define APPENDPROTO dummyproto
extern AUTHENTICATOR auth_log;
