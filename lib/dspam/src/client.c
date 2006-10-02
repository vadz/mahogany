/* $Id$ */

/*
 DSPAM
 COPYRIGHT (C) 2002-2006 JONATHAN A. ZDZIARSKI

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; version 2
 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

/*
 * client.c - client-based functions (for operating in client/daemon mode)c
 *
 * DESCRIPTION
 *   Client-based functions are called when --client is specified on the
 *   commandline or by dspamc (where --client is inferred). The client
 *   functions connect to a DSPAM server for processing (rather than the
 *   usual behavior which is to process the message itself). Client
 *   functions are also used when delivering via LMTP or SMTP.
 */

#ifdef HAVE_CONFIG_H
#include <auto-config.h>
#endif

#ifdef DAEMON

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#ifndef _WIN32
#include <unistd.h>
#include <pwd.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include "client.h"
#include "dspam.h"
#include "config.h"
#include "util.h"
#include "language.h"
#include "buffer.h"

/*
 * client_process(AGENT_CTX *, buffer *)
 *
 * DESCRIPTION
 *   connect to a dspam daemon socket and attempt to process a message
 *   this function is called by the dspam agent when --client is specified
 *
 * INPUT ARGUMENTS
 *      ATX     	agent context
 *      message		message to be processed
 *
 * RETURN VALUES
 *   returns 0 on success
*/

int client_process(AGENT_CTX *ATX, buffer *message) {
  char buf[1024], err[256];
  struct nt_node *node_nt;
  struct nt_c c_nt;
  int exitcode = 0, msglen;
  THREAD_CTX TTX;
  int i;

  TTX.sockfd = client_connect(ATX, 0);
  if (TTX.sockfd <0) {
    LOG(LOG_WARNING, ERR_CLIENT_CONNECT);
    STATUS(ERR_CLIENT_CONNECT);
    return TTX.sockfd;
  }

  TTX.packet_buffer = buffer_create(NULL);
  if (TTX.packet_buffer == NULL) 
    goto BAIL;

  /* LHLO / MAIL FROM - Authenticate on the server */

  if (client_authenticate(&TTX, ATX->client_args)<0) {
    LOG(LOG_WARNING, ERR_CLIENT_AUTH_FAILED);
    STATUS(ERR_CLIENT_AUTH_FAILED);
    goto QUIT;
  }

  /* RCPT TO - Send recipient information */

  strcpy(buf, "RCPT TO: ");
  node_nt = c_nt_first(ATX->users, &c_nt);
  while(node_nt != NULL) {
    const char *ptr = (const char *) node_nt->ptr;
    snprintf(buf, sizeof(buf), "RCPT TO: <%s>", ptr);
    if (send_socket(&TTX, buf)<=0) {
      STATUS(ERR_CLIENT_SEND_FAILED);
      goto BAIL;
    }

    if (client_getcode(&TTX, err, sizeof(err))!=LMTP_OK) {
      STATUS(err);
      goto QUIT;
    }

    node_nt = c_nt_next(ATX->users, &c_nt);
  }

  /* DATA - Send message */

  if (send_socket(&TTX, "DATA")<=0) 
    goto BAIL;

  if (client_getcode(&TTX, err, sizeof(err))!=LMTP_DATA) {
    STATUS(err);
    goto QUIT;
  }

  i = 0;
  msglen = strlen(message->data);
  while(i<msglen) {
    int r = send(TTX.sockfd, message->data+i, msglen - i, 0);
    if (r <= 0) {
      STATUS(ERR_CLIENT_SEND_FAILED);
      goto BAIL;
    }
    i += r;
  }

  if (message->data[strlen(message->data)-1]!= '\n') {
    if (send_socket(&TTX, "")<=0) {
     STATUS(ERR_CLIENT_SEND_FAILED);
     goto BAIL;
    }
  }

  if (send_socket(&TTX, ".")<=0) {
    STATUS(ERR_CLIENT_SEND_FAILED);
    goto BAIL;
  }

  /* Server response */

  if (ATX->flags & DAF_STDOUT || ATX->flags & DAF_SUMMARY ||
      ATX->operating_mode == DSM_CLASSIFY) 
   {
    char *line = NULL;
    int head = !(ATX->flags & DAF_STDOUT);

    if (ATX->flags & DAF_SUMMARY)
      head = 1;

    line = client_getline(&TTX, 300);

    while(line != NULL && strcmp(line, ".")) {
      chomp(line);
      if (!head) {
        head = 1;
        if (!strncmp(line, "250 ", 4)) {
          free(line);
          goto QUIT;
        }

        if (!strcmp(line, "X-Daemon-Classification: SPAM") && 
            _ds_match_attribute(agent_config, "Broken", "returnCodes")) 
        {
          exitcode = 99;
        }
      } else {
        printf("%s\n", line);
        if (ATX->flags & DAF_SUMMARY)
          break;
      } 
      free(line);
      line = client_getline(&TTX, 300);
      if (line) chomp(line);
    }
    free(line);
    if (line == NULL)
      goto BAIL;
  } else {
    for(i=0;i<ATX->users->items;i++) {
      char *input = client_getline(&TTX, 300);
      char *x;
      int code = 500;

      if (!input) {
        exitcode = EFAILURE;
        goto BAIL;
      }
      x = strtok(input, " ");
      if (x) {
        code = atoi(x);
        if (code != LMTP_OK) {
          if (exitcode > 0) 
            exitcode = 0;
          exitcode--;
        } else {
          if (_ds_match_attribute(agent_config, "Broken", "returnCodes")) {
            x = strtok(NULL, ":");
            if (x)
              x = strtok(NULL, ":");
            if (x && strstr(x, "SPAM") && exitcode == 0)
              exitcode = 99;
          }
        }
      }
    }
  }

  send_socket(&TTX, "QUIT");
  client_getcode(&TTX, err, sizeof(err));
  close(TTX.sockfd);
  buffer_destroy(TTX.packet_buffer);
  return exitcode;

QUIT:
  send_socket(&TTX, "QUIT");
  client_getcode(&TTX, err, sizeof(err));

BAIL:
  exitcode = EFAILURE;
  buffer_destroy(TTX.packet_buffer);
  close(TTX.sockfd);
  return exitcode;
}

/*
 * client_connect(AGENT_CTX ATX, int flags)
 *
 * DESCRIPTION
 *   establish a connection to a server
 *
 * INPUT ARGUMENTS
 *      ATX             agent context
 *      flags		connection flags
 *
 * FLAGS
 *    CCF_PROCESS     Use ClientHost as destination
 *    CCF_DELIVERY    Use DeliveryHost as destination
 *
 *  RETURN VALUES
 *    returns 0 on success
 */

int client_connect(AGENT_CTX *ATX, int flags) {
  struct sockaddr_in addr;
  struct sockaddr_un saun;
  int sockfd;
  int yes = 1;
  int port = 24;
  int domain = 0;
  int addr_len;
  char *host;

  if (flags & CCF_DELIVERY) {
    host = _ds_read_attribute(agent_config, "DeliveryHost");

    if (ATX->recipient && ATX->recipient[0]) {
      char *domain = strchr(ATX->recipient, '@');
      if (domain) {
        char key[128];
        snprintf(key, sizeof(key), "DeliveryHost.%s", domain+1);
        if (_ds_read_attribute(agent_config, key))
          host = _ds_read_attribute(agent_config, key);
      }
    }

    if (_ds_read_attribute(agent_config, "DeliveryPort"))
      port = atoi(_ds_read_attribute(agent_config, "DeliveryPort"));

    if (host && host[0] == '/') 
      domain = 1;

  } else {
    host = _ds_read_attribute(agent_config, "ClientHost");

    if (_ds_read_attribute(agent_config, "ClientPort"))
      port = atoi(_ds_read_attribute(agent_config, "ClientPort"));

    if (host && host[0] == '/')
      domain = 1;
  }

  if (host == NULL) {
    LOG(LOG_CRIT, ERR_CLIENT_INVALID_CONFIG);
    STATUS(ERR_CLIENT_INVALID_CONFIG);
    return EINVAL;
  }

  /* Connect (domain socket) */

  if (domain) {
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    saun.sun_family = AF_UNIX;
    strcpy(saun.sun_path, host);
    addr_len = sizeof(saun.sun_family) + strlen(saun.sun_path) + 1;

    LOGDEBUG(INFO_CLIENT_CONNECTING, host, 0);
    if(connect(sockfd, (struct sockaddr *)&saun, addr_len)<0) {
      LOG(LOG_ERR, ERR_CLIENT_CONNECT_SOCKET, host, strerror(errno));
      STATUS(strerror(errno));
      close(sockfd);
      return EFAILURE;
    }

  /* Connect (TCP socket) */

  } else {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(host);
    addr.sin_port = htons(port);
    addr_len = sizeof(struct sockaddr_in);
    LOGDEBUG(INFO_CLIENT_CONNECTING, host, port);
    if(connect(sockfd, (struct sockaddr *)&addr, addr_len)<0) {
      LOG(LOG_ERR, ERR_CLIENT_CONNECT_HOST, host, port, strerror(errno));
      STATUS(strerror(errno));
      close(sockfd);
      return EFAILURE;
    }
  }

  LOGDEBUG(INFO_CLIENT_CONNECTED);
  setsockopt(sockfd,SOL_SOCKET,TCP_NODELAY,&yes,sizeof(int));

  return sockfd;
}

/*
 * client_authenticate(AGENT_CTX *ATX, const char *mode)
 *
 * DESCRIPTION
 *   greet and authenticate on a server
 *
 * INPUT ARGUMENTS
 *      ATX             agent context
 *      mode		processing mode
 *
 * NOTES
 *   the process mode is passed using the DSPAMPROCESSMODE service tag
 *
 * RETURN VALUES
 *   returns 0 on success
 */

int client_authenticate(THREAD_CTX *TTX, const char *mode) {
  char *ident = _ds_read_attribute(agent_config, "ClientIdent");
  char buf[1024], err[128];
  char *ptr;
  char pmode[1024];

  pmode[0] = 0;
  if (mode) {
    int pos = 0, cpos = 0;
    for(;mode[cpos]&&pos<(sizeof(pmode)-1);cpos++) {
      if (mode[cpos] == '"') {
        pmode[pos] = '\\';
        pos++;
      }
      pmode[pos] = mode[cpos];
      pos++;
    }
    pmode[pos] = 0;
  }

  if (!ident || !strchr(ident, '@')) {
    LOG(LOG_ERR, ERR_CLIENT_IDENT);
    return EINVAL;
  }

  ptr = client_expect(TTX, LMTP_GREETING, err, sizeof(err));
  if (ptr == NULL) {
    LOG(LOG_ERR, ERR_CLIENT_WHILE_AUTH, err);
    return EFAILURE;
  }
  free(ptr);

  snprintf(buf, sizeof(buf), "LHLO %s", strchr(ident, '@')+1);
  if (send_socket(TTX, buf)<=0) 
    return EFAILURE;

  if (client_getcode(TTX, err, sizeof(err))!=LMTP_OK) {
    return EFAILURE;
  }

  if (mode) {
    snprintf(buf, sizeof(buf), "MAIL FROM: <%s> DSPAMPROCESSMODE=\"%s\"", ident, pmode);
  } else {
    snprintf(buf, sizeof(buf), "MAIL FROM: <%s>", ident);
  }
  if (send_socket(TTX, buf)<=0) {
    return EFAILURE;
  }

  if (client_getcode(TTX, err, sizeof(err))!=LMTP_OK) {
    LOG(LOG_ERR, ERR_CLIENT_AUTHENTICATE);
    return EFAILURE;
  }

  return 0;
}

/*
 * client_expect(THREAD_CTX *TTX, int code, char *err, size_t len)
 *
 * DESCRIPTION
 *   wait for the appropriate return code, then return
 *
 * INPUT ARGUMENTS
 *    ATX    agent context
 *    code   return code to wait for
 *    err    error buffer
 *    len    buffer len
 *
 * RETURN VALUES
 *   allocated pointer to acknowledgement line, NULL on error
 *   err buffer is populated on error
 */

char * client_expect(THREAD_CTX *TTX, int code, char *err, size_t len) {
  char *inp, *dup, *ptr, *ptrptr;
  int recv_code;

  inp = client_getline(TTX, 300);
  while(inp != NULL) {
    recv_code = 0;
    dup = strdup(inp);
    if (!dup) {
      free(inp);
      LOG(LOG_CRIT, ERR_MEM_ALLOC);
      strlcpy(err, ERR_MEM_ALLOC, len);
      return NULL;
    }
    if (strncmp(dup, "250-", 4)) {
      ptr = strtok_r(dup, " ", &ptrptr);
      if (ptr) 
        recv_code = atoi(ptr);
      free(dup);
      if (recv_code == code) {
        err[0] = 0;
        return inp;
      }
      LOG(LOG_WARNING, ERR_CLIENT_RESPONSE_CODE, code, inp);
    }
    
    strlcpy(err, inp, len);
    free(inp);
    inp = client_getline(TTX, 300);
  }

  return NULL;
}

/*
 * client_parsecode(const char *err)
 *
 * DESCRIPTION
 *   parse response code from plain text
 *
 * INPUT ARGUMENTS
 *      err    error message to parse
 *
 * RETURN VALUES
 *   integer value of response code
 */

int client_parsecode(char *error) {
  char code[4];
  code[3] = 0;
  strncpy(code, error, 3);
  return atoi(code);
}

/*
 * client_getcode(THREAD_CTX *TTX, char *err, size_t len)
 *
 * DESCRIPTION
 *   retrieve a line of input and return response code
 *
 * INPUT ARGUMENTS  
 *     TTX    thread context containing sockfd
 *     err    error buffer
 *     len    buffer len
 *
 * RETURN VALUES
 *   integer value of response code
 */

int client_getcode(THREAD_CTX *TTX, char *err, size_t len) {
  char *inp, *ptr, *ptrptr;
  int i;

  inp = client_getline(TTX, 300);
  if (!inp)
    return EFAILURE;

  while(inp && !strncmp(inp, "250-", 4)) {
    free(inp);
    inp = client_getline(TTX, 300);
  }

  strlcpy(err, inp, len);
  ptr = strtok_r(inp, " ", &ptrptr);
  if (ptr == NULL)
    return EFAILURE;
  i = atoi(ptr);
  free(inp);
  return i;
}

/*
 * client_getline(THREAD_CTX *TTX, int timeout)
 *
 * DESCRIPTION
 *   read a complete line from a socket
 *
 * INPUT ARGUMENTS  
 *      TTX        thread context containing sockfd
 *      timeout    timeout (in seconds) to wait for input
 *
 * RETURN VALUES
 *   allocated pointer to input
 */

char *client_getline(THREAD_CTX *TTX, int timeout) {
  struct timeval tv;
  long recv_len;
  char buf[1024];
  char *pop;
  fd_set fds;
  int i;

  pop = pop_buffer(TTX);
  while(!pop) {
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(TTX->sockfd, &fds);
    i = select(TTX->sockfd+1, &fds, NULL, NULL, &tv);
    if (i<=0)
      return NULL;

    recv_len = recv(TTX->sockfd, buf, sizeof(buf)-1, 0);
    buf[recv_len] = 0;
    if (recv_len == 0)
      return NULL;
    buffer_cat(TTX->packet_buffer, buf);
    pop = pop_buffer(TTX);
  }

#ifdef VERBOSE
  LOGDEBUG("RECV: %s", pop);
#endif

  return pop;
}

/*
 * pop_buffer (THREAD_CTX *TTX)
 *
 * DESCRIPTION
 *   pop a line off the packet buffer
 *
 * INPUT ARGUMENTS
 *      TTX     thread context containing the packet buffer
 * 
 * RETURN VALUES
 *   allocated pointer to line, NULL if complete line isn't available
 */

char *pop_buffer(THREAD_CTX *TTX) {
  char *buf, *eol;
  long len;

  if (!TTX || !TTX->packet_buffer || !TTX->packet_buffer->data)
    return NULL;

  eol = strchr(TTX->packet_buffer->data, 10);
  if (!eol)
    return NULL;
  len = (eol - TTX->packet_buffer->data) + 1;
  buf = calloc(1, len+1);
  if (!buf) {
    LOG(LOG_CRIT, ERR_MEM_ALLOC);
    return NULL;
  }
  memcpy(buf, TTX->packet_buffer->data, len);
  memmove(TTX->packet_buffer->data, eol+1, strlen(eol+1)+1);
  TTX->packet_buffer->used -= len;
  return buf;
}

/*
 * send_socket(THREAD_CTX *TTX, const char *text)
 *
 * DESCRIPTION
 *   send a line of text to a socket
 *
 * INPUT ARGUMENTS
 *      TTX     thread context containing sockfd
 *      text    text to send
 *
 * RETURN VALUES
 *   number of bytes sent
 */

int send_socket(THREAD_CTX *TTX, const char *text) {
  int i = 0, r, msglen;

#ifdef VERBOSE
  LOGDEBUG("SEND: %s", text);
#endif

  msglen = strlen(text);
  while(i<msglen) {
    r = send(TTX->sockfd, text+i, msglen-i, 0);
    if (r <= 0) {
      return r;
    }
    i += r;
  }

  r = send(TTX->sockfd, "\r\n", 2, 0);
  return i+2;
}

/*
 * deliver_socket(AGENT_CTX *ATX, const char *msg, int proto)
 *
 * DESCRIPTION
 *   delivers message via LMTP or SMTP (instead of TrustedDeliveryAgent) 
 *
 *   If LMTP/SMTP delivery was specified in dspam.conf, this function will be 
 *   called by deliver_message(). This function connects to and delivers the 
 *   message using standard LMTP or SMTP. Depending on how DSPAM was originally 
 *   called, either the address supplied with the incoming RCPT TO or the 
 *   address supplied on the commandline with --rcpt-to  will be used. If 
 *   neither are present, the username will be used. 
 *
 * INPUT ARGUMENTS
 *     ATX      agent context
 *     msg      message to send
 *     proto    protocol to use
 *
 * PROTOCOLS
 *     DDP_LMTP    LMTP
 *     DDP_SMTP    SMTP
 *
 * RETURN VALUES
 *   returns 0 on success
 */

int deliver_socket(AGENT_CTX *ATX, const char *msg, int proto) {
  THREAD_CTX TTX;
  char buf[1024], err[256];
  char *ident = _ds_read_attribute(agent_config, "DeliveryIdent");
  int exitcode = EFAILURE;
  int msglen, code;
  char *inp;
  int i;

  err[0] = 0;

  TTX.sockfd = client_connect(ATX, CCF_DELIVERY);
  if (TTX.sockfd <0) {
    STATUS(ERR_CLIENT_CONNECT);
    LOG(LOG_ERR, ERR_CLIENT_CONNECT);
    return TTX.sockfd;
  }

  TTX.packet_buffer = buffer_create(NULL);
  if (TTX.packet_buffer == NULL) {
    LOG(LOG_CRIT, ERR_MEM_ALLOC);
    STATUS(ERR_MEM_ALLOC);
    goto BAIL;
  }

  inp = client_expect(&TTX, LMTP_GREETING, err, sizeof(err));
  if (inp == NULL) {
    LOG(LOG_ERR, ERR_CLIENT_ON_GREETING, err);
    STATUS(err);
    goto BAIL;
  }
  free(inp);

  /* LHLO / HELO */

  snprintf(buf, sizeof(buf), "%s %s", (proto == DDP_LMTP) ? "LHLO" : "HELO",
           (ident) ? ident : "localhost");
  if (send_socket(&TTX, buf)<=0) {
    LOG(LOG_ERR, ERR_CLIENT_SEND_FAILED);
    STATUS(ERR_CLIENT_SEND_FAILED);
    goto BAIL;
  }

  /* MAIL FROM */

  inp = client_expect(&TTX, LMTP_OK, err, sizeof(err));
  if (inp == NULL) {
    LOG(LOG_ERR, ERR_CLIENT_INVALID_RESPONSE,
      (proto == DDP_LMTP) ? "LHLO" : "HELO", err);
    STATUS("%s: %s", (proto == DDP_LMTP) ? "LHLO" : "HELO", err);
    goto QUIT;
  }
  free(inp);

  if (proto == DDP_LMTP) {
    snprintf(buf, sizeof(buf), "MAIL FROM:<%s> SIZE=%ld", 
             ATX->mailfrom, (long) strlen(msg));
  } else {
    snprintf(buf, sizeof(buf), "MAIL FROM:<%s>", ATX->mailfrom);
  }

  if (send_socket(&TTX, buf)<=0) {
    LOG(LOG_ERR, ERR_CLIENT_SEND_FAILED);
    STATUS(ERR_CLIENT_SEND_FAILED);
    goto BAIL;
  }

  code = client_getcode(&TTX, err, sizeof(err));

  if (code!=LMTP_OK) {
    LOG(LOG_ERR, ERR_CLIENT_RESPONSE, code, "MAIL FROM", err);
    if (code >= 500) 
      exitcode = EINVAL;
    chomp(err);
    STATUS((code >= 500) ? "Fatal: %s" : "Deferred: %s", err);
    goto QUIT;
  }

  /* RCPT TO */

  snprintf(buf, sizeof(buf), "RCPT TO:<%s>", (ATX->recipient) ? ATX->recipient : "");
  if (send_socket(&TTX, buf)<=0) {
    LOG(LOG_ERR, ERR_CLIENT_SEND_FAILED);
    STATUS(ERR_CLIENT_SEND_FAILED);
    goto BAIL;
  }

  code = client_getcode(&TTX, err, sizeof(err));

  if (code!=LMTP_OK) {
    LOG(LOG_ERR, ERR_CLIENT_RESPONSE, code, "RCPT TO", err);
    if (code >= 500)
      exitcode = EINVAL;  
    chomp(err);
    STATUS((code >= 500) ? "Fatal: %s" : "Deferred: %s", err);
    goto QUIT;
  }

  /* DATA */

  if (send_socket(&TTX, "DATA")<=0) {
    LOG(LOG_ERR, ERR_CLIENT_SEND_FAILED);
    STATUS(ERR_CLIENT_SEND_FAILED);
    goto BAIL;
  }

  code = client_getcode(&TTX, err, sizeof(err));
  if (code!=LMTP_DATA) {
    LOG(LOG_ERR, ERR_CLIENT_RESPONSE, code, "DATA", err);
    if (code >= 500)
      exitcode = EINVAL;
    chomp(err);
    STATUS((code >= 500) ? "Fatal: %s" : "Deferred: %s", err);
    goto QUIT;
  }

  i = 0;
  msglen = strlen(msg);
  while(i<msglen) {
    int r = send(TTX.sockfd, msg+i, msglen - i, 0);
    if (r <= 0) {
      LOG(LOG_ERR, ERR_CLIENT_SEND_FAILED);
      STATUS(ERR_CLIENT_SEND_FAILED);
      goto BAIL;
    }
    i += r;
  }

  if (msg[strlen(msg)-1]!= '\n') {
    if (send_socket(&TTX, "")<=0) {
      LOG(LOG_ERR, ERR_CLIENT_SEND_FAILED);
      STATUS(ERR_CLIENT_SEND_FAILED);
      goto BAIL;
    }
  }

  if (send_socket(&TTX, "\r\n.")<=0) {
    LOG(LOG_ERR, ERR_CLIENT_SEND_FAILED);
    STATUS(ERR_CLIENT_SEND_FAILED);
    goto BAIL;
  }

  /* server response */

  code = client_getcode(&TTX, err, sizeof(err));
  if (code < 200 || code >= 300) {
    LOG(LOG_ERR, ERR_CLIENT_RESPONSE, code, "message data", err);
    if (code >= 500)
      exitcode = EINVAL;
    chomp(err);
    STATUS((code >= 500) ? "Fatal: %s" : "Deferred: %s", err);
    goto QUIT;
  }

  send_socket(&TTX, "QUIT");
  client_getcode(&TTX, err, sizeof(err));
  close(TTX.sockfd);
  buffer_destroy(TTX.packet_buffer);
  return 0;

QUIT:
  send_socket(&TTX, "QUIT");
  client_getcode(&TTX, err, sizeof(err));

BAIL:
  LOG(LOG_ERR, ERR_CLIENT_DELIVERY_FAILED);
  buffer_destroy(TTX.packet_buffer);
  close(TTX.sockfd);
  return exitcode;
}

#endif /* DAEMON */
