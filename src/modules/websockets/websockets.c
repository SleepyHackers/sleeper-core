#include <sha.h>
#include <http_parser.h>
#include <string.h>
#include <stdlib.h>
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
#define WS_FIN     0
#define WS_RSV1    1
#define WS_RSV2    2
#define WS_RSV3    3
#define WS_OPCODE  4
#define WS_MASK    8
#define WS_LEN     9
#define WS_EXLEN   16
#define WS_MASK    16

#define WS_MASK_SIZE 4;

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
  LIST *frame_frags;
} WEBSOCKET_DATA;

typedef struct websocket_frame {
  bool fin;
  bool rsv1;
  bool rsv2;
  bool rsv3;
  int opcode;
  bool masked;
  int payload_len;
  uint16_t payload_len16;
  uint64_t payload_len64;
  char mask[4];
  char *payload;
} WEBSOCKET_FRAME;



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

WEBSOCKET_FRAME *websocket_create_frame() {
  WEBSOCKET_FRAME *frame = calloc(1, sizeof(WEBSOCKET_FRAME));;
  frame->payload = (char*)malloc(MAX_INPUT_LEN);
  return frame;
}

void websocket_delete_frame(WEBSOCKET_FRAME *frame) {
  free(frame->payload);;
  free(frame);
}

int websocket_disconnect(WEBSOCKET_DATA *conn) {

}

int websocket_ping(WEBSOCKET_DATA *conn) {

}

int websocket_pong(WEBSOCKET_DATA *conn) {

}

int websocket_bin_send(char *msg, WEBSOCKET_DATA *conn) {

}

int websocket_frame(uint8_t type, char *msg) {
  int msglen = strlen(msg);

  if(msglen <= 126) {

  } else {

  }
  /*
    if(strstr(buffy, "ping")) {
      log_string("%s", buffy);

      log_string("Got a ping from socket FD %i", sock->uid);
      char msg[8];
      memset(msg, 0, 8);
      msg[0] = msg[0] | 1;
      msg[0] = msg[0] | 1<<7;
      msg[1] = msg[1] | 1<<2;
      msg[2] = 'p';
      msg[3] = 'o';
      msg[4] = 'n';
      msg[5] = 'g';
      msg[6] = '\0';
      log_string("0x%2x 0x%2x 0x%2x 0x%2x", msg[0], msg[1], msg[2], msg[3]);

      send(sock->uid, msg, strlen(msg), 0);
    }
  */  
}

WEBSOCKET_FRAME *websocket_deframe(char *input) {

      /* Frame is as follows:
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-------+-+-------------+-------------------------------+
     |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
     |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     | |1|2|3|       |K|             |                               |
     +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
     |     Extended payload length continued, if payload len == 127  |
     + - - - - - - - - - - - - - - - +-------------------------------+
     |                               |Masking-key, if MASK set to 1  |
     +-------------------------------+-------------------------------+
     | Masking-key (continued)       |          Payload Data         |
     +-------------------------------- - - - - - - - - - - - - - - - +
     :                     Payload Data continued ...                :
     + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
     |                     Payload Data continued ...                |
     +---------------------------------------------------------------+
      */

    int bit, count=0;
    int m = 0, j = 0, n= 0;
    int c, i;
    WEBSOCKET_FRAME *frame = websocket_create_frame();

    frame->fin = !!(input[0] & 0x80);
    frame->rsv1 = !!(input[0] & (0x80<<WS_RSV1));
    frame->rsv2 = !!(input[0] & (0x80<<WS_RSV2));
    frame->rsv3 = !!(input[0] & (0x80<<WS_RSV3));
    frame->masked = !!(input[1] & 0x80);
    frame->opcode = (char)(input[0] & 0x0F);

    frame->payload_len = (char)(input[1] ^ 0x80);
    if(frame->fin == 0) return;        
    log_string("op: %i", frame->opcode);

    frame->payload[0] = '\0';

    int mask_start = 2;

    if( frame->payload_len == 126 ) {
      mask_start = 4;
      frame->payload_len16 = ntohs(input[2] | (uint16_t)(input[3]<<8));
      log_string("%u", frame->payload_len16);
    }

    /* Untested */
    if( frame->payload_len == 127 ) {
      mask_start = 8;
      frame->payload_len64 = ntohs(input[2] | (uint16_t)(input[3]<<8) | (uint16_t)(input[3]<<16) | (uint16_t)(input[3]<<24));
    }
    
    int mask_end = mask_start + WS_MASK_SIZE;

    if(frame->masked == 1 && frame->payload_len > 0) {
      for (c = mask_start; input[c] != '\0' ; c++) {
	if(c >= mask_start  && c < mask_end) {
	  frame->mask[m] =  input[c];
	  m++;
	} else if(c >= mask_end) {
	  j = n % 4;
	  frame->payload[n] = input[c] ^ frame->mask[j];
	  n++;
	}

      }
    }

    frame->payload[n] = '\0';
    return frame;
}

websocket_txt_send(char *msg) {

}


void handleWebSocket(WEBSOCKET_DATA *sock) {
  BUFFER     *buf = newBuffer(MAX_BUFFER);

  char b64in[MAX_BUFFER]; memset(b64in, 0, MAX_BUFFER);
  char sha1[40]; memset(sha1, 0, 40);
  char dest[b64max(40)];
  char *cSave, *cTok, *ch, swap;

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
      sock->input_buf[0] = '\0';
      sock->input_length = 0;
    }
  } else {
    /* We are connected, check the input buffer for commands, */
    /* Frame is as follows:
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-------+-+-------------+-------------------------------+
     |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
     |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     | |1|2|3|       |K|             |                               |
     +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
     |     Extended payload length continued, if payload len == 127  |
     + - - - - - - - - - - - - - - - +-------------------------------+
     |                               |Masking-key, if MASK set to 1  |
     +-------------------------------+-------------------------------+
     | Masking-key (continued)       |          Payload Data         |
     +-------------------------------- - - - - - - - - - - - - - - - +
     :                     Payload Data continued ...                :
     + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
     |                     Payload Data continued ...                |
     +---------------------------------------------------------------+
      */

    /* Deframe the data */
    WEBSOCKET_FRAME *iframe = websocket_deframe(sock->input_buf);

    log_string("%s", iframe->payload);

    sock->input_buf[0] = '\0';
    sock->input_length = 0;

    websocket_delete_frame(iframe);
  }
  // clean up our mess

  deleteBuffer(buf);
}


void websockets_loop(WEBSOCKET_DATA  *conn) {
  struct timeval tv = { 0, 0 }; // we don't wait for any action.
  fd_set read_fd;
  int peek = 0;

  //int in_len = recv (conn->uid, &conn->input_buf, MAX_INPUT_LEN, NULL);
  // get our sets all done up
  ioctl(conn->uid, FIONREAD, &peek);
  if(peek > 0) {
    int in_len = read(conn->uid, conn->input_buf + conn->input_length,
		      MAX_INPUT_LEN - conn->input_length - 1);
    
    if(in_len > 0) {
      conn->input_length += in_len;
      conn->input_buf[conn->input_length] = '\0';
      handleWebSocket(conn);
    }
      else if(in_len <= 0) {
	closeWebSocket(conn);
	listRemove(ws_descs, conn);
	deleteWebSocket(conn);
	return;
      }
      
    
    
  }
  
}

void websockets_process(void *owner, void *data, char *arg) {
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

    }
  }

  LIST_ITERATOR *conn_i = newListIterator(ws_descs);
  ITERATE_LIST(conn, conn_i) {
      websockets_loop(conn);
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
  start_update(0xA, 0.1 SECOND, websockets_process, NULL, NULL, NULL);
}

void destroy_websockets() {
  //  hookRemove("receive_connection", (void*)doTest);
  WEBSOCKET_DATA  *conn = NULL;
  LIST_ITERATOR *conn_i = newListIterator(ws_descs);
  shutdown(ws_uid, SHUT_RDWR);

  ITERATE_LIST(conn, conn_i) {

    listRemove(ws_descs, conn);

    closeWebSocket(conn);

    deleteWebSocket(conn);

  } deleteListIterator(conn_i);
  interrupt_events_involving(0xA);
}

bool onLoad() {
  init_websockets();
  return TRUE;
}

bool onUnload() {
  destroy_websockets();
  return TRUE;
}
