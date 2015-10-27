#include <sha.h>
#include <http_parser.h>
#include <string.h>
#include <stdlib.h>
#include "mud.h"
#include "character.h"
#include "utils.h"
#include "list.h"
#include "room.h"
#include "socket.h"
#include "hooks.h"
#include "event.h"
#include "inform.h"
#include "wsutil.h"
#include "websockets.h"

#define GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"


const char   MODULE_NAME[]     = "websockets";
const char   MODULE_DESC[]     = "Websocket handler";
const char   MODULE_DEPENDS[]  = "";
const double MODULE_VERSION    = 1.0;

LIST *ws_descs = NULL;
int ws_uid;

// the datstructure for a web descriptor
typedef struct websocket_data {
  int uid;
  struct sockaddr_in stAddr;
  char input_buf[MAX_INPUT_LEN];
  int input_length;
  int connected;
} WEBSOCKET_DATA;

// used by handleWebSocket. Replaces one char with another in the buf.
// Returns how many replacements were made
int replace_char(char *buf, char from, char to) {
  int replacements = 0;
  for(;*buf != '\0'; buf++) {
    if(*buf == from) {
      *buf = to;
      replacements++;
    }
  }
  return replacements;
}



void doTest(char *info) {
  SOCKET_DATA *sock = NULL;
  hookParseInfo(info, &socket);
  //  char *cmd = listPop(sock->input);
  int uid = socketGetUID(sock);
  log_string("%d", uid);
}

WEBSOCKET_DATA *newWebSocket() {
  WEBSOCKET_DATA *wsock = calloc(1, sizeof(WEBSOCKET_DATA));
  return wsock;
}


void deleteWebSocket(WEBSOCKET_DATA *wsock) {
  free(wsock);
}

void closeWebSocket(WEBSOCKET_DATA *wsock) {
  close(wsock->uid);
}

void bufferASCIIHTML(BUFFER *buf) {
  bufferReplace(buf, "\r", "",         TRUE);
  bufferReplace(buf, "\n", "<br>",     TRUE);
  bufferReplace(buf, "  ", " &nbsp;",  TRUE);
  bufferReplace(buf, "{n", "<font color=\"green\">",   TRUE);
  bufferReplace(buf, "{g", "<font color=\"green\">",   TRUE);
  bufferReplace(buf, "{w", "<font color=\"silver\">",  TRUE);
  bufferReplace(buf, "{p", "<font color=\"purple\">",  TRUE);
  bufferReplace(buf, "{b", "<font color=\"navy\">",    TRUE);
  bufferReplace(buf, "{y", "<font color=\"olive\">",   TRUE);
  bufferReplace(buf, "{r", "<font color=\"maroon\">",  TRUE);
  bufferReplace(buf, "{c", "<font color=\"teal\">",    TRUE);
  bufferReplace(buf, "{d", "<font color=\"black\">",   TRUE);
  bufferReplace(buf, "{G", "<font color=\"lime\">",    TRUE);
  bufferReplace(buf, "{W", "<font color=\"white\">",   TRUE);
  bufferReplace(buf, "{P", "<font color=\"magenta\">", TRUE);
  bufferReplace(buf, "{B", "<font color=\"blue\">",    TRUE);
  bufferReplace(buf, "{Y", "<font color=\"yellow\">",  TRUE);
  bufferReplace(buf, "{R", "<font color=\"red\">",     TRUE);
  bufferReplace(buf, "{C", "<font color=\"aqua\">",    TRUE);
  bufferReplace(buf, "{D", "<font color=\"grey\">",    TRUE);
}

void handleWebSocket(WEBSOCKET_DATA *sock) {
  static char get[SMALL_BUFFER];
  static char key[SMALL_BUFFER];
  char    *argstr = NULL;
  BUFFER     *buf = newBuffer(MAX_BUFFER);
  HASHTABLE *args = newHashtable();

  char b64in[MAX_BUFFER]; memset(b64in, 0, MAX_BUFFER);
  char sha1[40]; memset(sha1, 0, 40);
  char dest[b64max(40)];
  char *cSave;
  
  // clear everything
  *get = *key = '\0';

  char *ch = strtok_r(sock->input_buf, "\n", &cSave);
  while (ch != NULL) {
    log_string("%s", ch);

    if (strstr(ch, "Sec-WebSocket-Key:")) {
      ch = strtok_r(ch, ": ", &cSave);
      ch = strtok_r(NULL, ": ", &cSave);

      snprintf(b64in, MAX_BUFFER, "%s%s", ch, GUID);
      size_t len = strlen(b64in);

      if (!SHA1(b64in, len, sha1)) {
	log_string("Failed to hash");
	return;
      }
      
      b64encode(ch, dest, b64max(40));
      break;
    }
    ch = strtok_r(NULL, "\n", &cSave);

  }

  if(sock->connected != 1) {
    bprintf(buf, "HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\nUpgrade: websocket\r\n\r\n", dest);
    sock->connected = 1;
    log_string("%s", bufferString(buf));

  }
  // send out the buf contents
  send(sock->uid, bufferString(buf), strlen(bufferString(buf)), 0);

  // clean up our mess
  deleteBuffer(buf);
  /*  deleteBuffer(buf);
  if(argstr) free(argstr);
  if(hashSize(args) > 0) {
    const char    *h_key = NULL;
    char          *h_val = NULL;
    HASH_ITERATOR *arg_i = newHashIterator(args);
    ITERATE_HASH(h_key, h_val, arg_i) {
      free(h_val);
    } deleteHashIterator(arg_i);
  } 
  deleteHashtable(args); */
}


void websockets_loop(void *owner, void *data, char *arg) {
  WEBSOCKET_DATA  *conn = NULL;
  struct timeval tv = { 0, 0 }; // we don't wait for any action.
  fd_set read_fd;


  // get our sets all done up
  FD_ZERO(&read_fd);
  FD_SET(ws_uid, &read_fd);

  // check to see if something happens
  select(FD_SETSIZE, &read_fd, NULL, NULL, &tv);

  // check for a new connection
  if(FD_ISSET(ws_uid, &read_fd)) {
    conn = newWebSocket();
    unsigned int socksize;

    // try to accept the connection
    if((conn->uid = accept(ws_uid, (struct sockaddr *)&conn->stAddr,
			       &socksize)) > 0) {
      listQueue(ws_descs, conn);
      FD_SET(conn->uid, &read_fd);
    }
  }


  // do input handling
  LIST_ITERATOR *conn_i = newListIterator(ws_descs);
  ITERATE_LIST(conn, conn_i) {
    if(FD_ISSET(conn->uid, &read_fd)) {
      int in_len = read(conn->uid, conn->input_buf + conn->input_length,
			MAX_INPUT_LEN - conn->input_length - 1);
      if(in_len > 0) {
	conn->input_length += in_len;
	conn->input_buf[conn->input_length] = '\0';
      }
      else if(in_len < 0) {
		closeWebSocket(conn);
	listRemove(ws_descs, conn);
	deleteWebSocket(conn);
      }
    }
  } deleteListIterator(conn_i);

  // do output handling
  conn_i = newListIterator(ws_descs);
  ITERATE_LIST(conn, conn_i) {
    // which version are we dealing with, and do we have a request terminator?
    // If we haven't gotten the terminator yet, don't handle or close the socket
    if( ( strstr(conn->input_buf, "HTTP/1.") && strstr(conn->input_buf, "\r\n\r\n")) ||
	(!strstr(conn->input_buf, "HTTP/1.") && strstr(conn->input_buf, "\n"))) {
           handleWebSocket(conn);
	   /*	           closeWebSocket(conn);
            listRemove(ws_descs, conn);
            deleteWebSocket(conn);*/
    }
  } deleteListIterator(conn_i);
}


void init_websockets() {
  //  hookAdd("receive_connection", (void*)doTest);

  struct sockaddr_in my_addr;
  int iSock, reuse = 1;

  // grab a socket
  iSock = socket(AF_INET, SOCK_STREAM, 0);

  // set its values
  my_addr.sin_family      = AF_INET;
  my_addr.sin_addr.s_addr = INADDR_ANY;
  my_addr.sin_port        = htons(WEB_PORT);

  // fix thread problems?
  if (setsockopt(iSock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1) {
    log_string("Websockets error in setsockopt()");
    exit(1);
  }

  // bind the port
  bind(iSock, (struct sockaddr *) &my_addr, sizeof(struct sockaddr));

  // start listening for connections
  listen(iSock, 3);

  // set the socket control
  ws_uid = iSock;

  // set up our list of connected sockets, and get the updater rolling
  ws_descs   = newList();
  //  query_table = newHashtable();
  start_update(NULL, 0.1 SECOND, websockets_loop, NULL, NULL, NULL);

  // set up our basic queries
  //  add_query("who", build_who_html);
  
}

void destroy_websockets() {
  hookRemove("receive_connection", (void*)doTest);
  shutdown(ws_uid, SHUT_RDWR);
  WEBSOCKET_DATA  *conn = NULL;
  LIST_ITERATOR *conn_i = newListIterator(ws_descs);
  ITERATE_LIST(conn, conn_i) {
    closeWebSocket(conn);
    listRemove(ws_descs, conn);
    deleteWebSocket(conn);
  } deleteListIterator(conn_i);
}

bool onLoad() {
  init_websockets();
  return TRUE;
}

bool onUnload() {
  destroy_websockets();
  return TRUE;
}
