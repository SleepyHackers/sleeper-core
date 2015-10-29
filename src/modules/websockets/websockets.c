#include <sha.h>
#include <http_parser.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <linux/ioctl.h>
#include <asm-generic/ioctls.h>

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
  int die;
  pthread_t thread;
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
  wsock->die = 0;
  wsock->connected = 0;
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
  BUFFER     *buf = newBuffer(MAX_BUFFER);

  char b64in[MAX_BUFFER]; memset(b64in, 0, MAX_BUFFER);
  char sha1[40]; memset(sha1, 0, 40);
  char dest[b64max(40)];
  char *cSave, *cTok, *ch;

  /* Are we connected yet? If not attempt a handshake */
  if(sock->connected != 1) {
    log_string("new websocket, %d, attempting to connect", sock->uid);

    /* Did we roughly get a valid header? */
    if( ( strstr(sock->input_buf, "HTTP/1.") && strstr(sock->input_buf, "\r\n\r\n")) ||
	(!strstr(sock->input_buf, "HTTP/1.") && strstr(sock->input_buf, "\n"))) {

      ch = sock->input_buf;
      ch = strtok_r(ch, "\n", &cSave);
      while (ch != NULL) {
		
	if (strstr(ch, "Sec-WebSocket-Key:")) {
	  cTok = strtok_r(ch, ": ", &cSave);
	  cTok = strtok_r(NULL, ": ", &cSave);
	  
	  cTok[strlen(cTok)-1] = '\0';
	  
	  snprintf(b64in, MAX_BUFFER, "%s%s", cTok, GUID);
	  size_t len = strlen(b64in);
	  
	  if (!SHA1(b64in, len, sha1)) {
	    log_string("ERROR: websocket, %s, sha1() failed", sock->uid);
	    return;
	  }
	  
	  b64encode(sha1, dest, b64max(40));

	  /* Confirm websocket handshake was sent */
	  sock->connected = 1;
	  break;
	}
	ch = strtok_r(NULL, "\n", &cSave);
      }
      
      bprintf(buf, "HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\nUpgrade: websocket\r\nSec-WebSocket-Protocol: chat\r\n\r\n", dest);
      
      // send out the buf contents
      send(sock->uid, bufferString(buf), strlen(bufferString(buf)), 0);
    }
  } else {

    /* We are connected, check the input buffer for commands, */
    log_string("%d", sock->input_buf[0]);

    if(strstr(sock->input_buf, "ping")) {
      bprintf(buf, "pong: %i\r\n", sock->uid);
      send(sock->uid, bufferString(buf), strlen(bufferString(buf)), 0);
      log_string("%x", sock->input_buf);

    }
    sock->input_buf[0] = '\0';
    sock->input_length = 0;
  }
  // clean up our mess
  deleteBuffer(buf);

}

void websockets_loop(WEBSOCKET_DATA  *conn) {
  struct timeval tv = { 0, 0 }; // we don't wait for any action.
  fd_set read_fd;
  int peek = 0;

  while ( conn->die == 0 ) {
    usleep(5000);
    //int in_len = recv (conn->uid, &conn->input_buf, MAX_INPUT_LEN, NULL);
    // get our sets all done up
    
    ioctl(conn->uid, FIONREAD, &peek);
    if(peek > 0) {
      int in_len = read(conn->uid, conn->input_buf + conn->input_length,
			MAX_INPUT_LEN - conn->input_length - 1);
      log_string("length/ret: %i", in_len);
      log_string("length/ret: %x", conn->input_buf);
      if(in_len > 0) {
	conn->input_length += in_len;
	conn->input_buf[conn->input_length] = '\0';
      }
      else if(in_len <= 0) {
	closeWebSocket(conn);
	listRemove(ws_descs, conn);
	deleteWebSocket(conn);
	pthread_exit(0);
	return;
      }
      
      
      handleWebSocket(conn);
    }
  }

  pthread_exit(0);

}

void websockets_process(void *owner, void *data, char *arg) {
  WEBSOCKET_DATA  *conn = NULL;
  pthread_attr_t       attr;
  pthread_t            thread_lookup;

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
          pthread_attr_init(&attr);   
	  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	  pthread_create(&thread_lookup, &attr, &websockets_loop, (void*) conn);
	  conn->thread = thread_lookup;
    }
  }
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
  start_update(0xA, 0.1 SECOND, websockets_process, NULL, NULL, NULL);
}

void destroy_websockets() {
  //  hookRemove("receive_connection", (void*)doTest);
  WEBSOCKET_DATA  *conn = NULL;
  LIST_ITERATOR *conn_i = newListIterator(ws_descs);

  ITERATE_LIST(conn, conn_i) {
    closeWebSocket(conn);
    listRemove(ws_descs, conn);
    conn->die = 1;
    
    /* Wait for socket to die */
    pthread_join(conn->thread, NULL);
    log_string("Killed socket %d", conn->uid);

    deleteWebSocket(conn);
  } deleteListIterator(conn_i);
  interrupt_events_involving(0xA);
  shutdown(ws_uid, SHUT_RDWR);
}

bool onLoad() {
  init_websockets();
  return TRUE;
}

bool onUnload() {
  destroy_websockets();
  return TRUE;
}
