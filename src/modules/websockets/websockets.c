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

const char   MODULE_NAME[]     = "websockets";
const char   MODULE_DESC[]     = "Websocket handler";
const char   MODULE_DEPENDS[]  = "";
const double MODULE_VERSION    = 1.0;

int ws_uid;

void websocket_delete_frame(WEBSOCKET_FRAME *frame) {
  WEBSOCKET_FRAME  *part = NULL;
  free(frame->payload);
  free(frame);
}

WEBSOCKET_COMMAND *websocket_new_command(int type, char *msg) {
  WEBSOCKET_COMMAND *command = calloc(1, sizeof(WEBSOCKET_DATA));
  command->opcode = type;
  command->payload = strdup(msg);
}

void websocket_delete_command(WEBSOCKET_COMMAND *command) {
  free(command->payload);
  free(command);
}

WEBSOCKET_DATA *newWebSocket() {
  WEBSOCKET_DATA *wsock = calloc(1, sizeof(WEBSOCKET_DATA));
  wsock->die = 0;
  wsock->connected = 0;
  wsock->input   = newList();
  wsock->output   = newList();
  wsock->frame_frags   = newList();
  return wsock;
}

void deleteWebSocket(WEBSOCKET_DATA *wsock) {
  deleteListWith(wsock->frame_frags, websocket_delete_frame);
  deleteListWith(wsock->input, websocket_delete_command);
  deleteListWith(wsock->output, websocket_delete_command);
  free(wsock);
}

void closeWebSocket(WEBSOCKET_DATA *wsock) {
  if(wsock->connected == 1) {
    websocket_send_close(wsock, "");
  }
  
  close(wsock->uid);
}

void websocket_destroy(WEBSOCKET_DATA *conn) {
  /*  deleteChar(conn->ch); */
  listRemove(ws_descs, conn);
  log_string("WebSocket: Closking link, %i", conn->uid);
  closeWebSocket(conn);
  deleteWebSocket(conn);
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

WEBSOCKET_FRAME *websocket_new_frame() {
  WEBSOCKET_FRAME *frame = calloc(1, sizeof(WEBSOCKET_FRAME));;
  frame->payload = (char*)malloc(MAX_INPUT_LEN);
  frame->payload[0] = '\0';
  frame->payload_len = 0;
  frame->fin = 0;
  frame->rsv1 = 0;
  frame->rsv2 = 0;
  frame->rsv3 = 0;
  frame->masked = 0;
  frame->opcode = 1;
  frame->size = 0;
  return frame;
}

int websocket_send_close(WEBSOCKET_DATA *sock, char *msg) {
  WEBSOCKET_FRAME *oframe = websocket_new_frame();
  oframe->opcode = WSF_CLOSE;
  websocket_send_frame(oframe, sock);
  oframe->payload = strdup(msg);
  oframe->payload_len = strlen(oframe->payload);
  websocket_delete_frame(oframe);
}


int websocket_ping(WEBSOCKET_DATA *conn, char *msg) {
  WEBSOCKET_FRAME *oframe = websocket_new_frame();
  oframe->opcode = WSF_PING;
  oframe->payload = strdup(msg);
  oframe->payload_len = strlen(oframe->payload);

  websocket_send_frame(oframe, conn);
  websocket_delete_frame(oframe);
  
}

int websocket_pong(WEBSOCKET_DATA *conn, char *msg) {
  WEBSOCKET_FRAME *oframe = websocket_new_frame();
  
  oframe->payload = strdup(msg);
  oframe->opcode = WSF_PONG;
  oframe->payload_len = strlen(oframe->payload);
  
  websocket_send_frame(oframe, conn);
  websocket_delete_frame(oframe);
}

char *websocket_build_frame(WEBSOCKET_FRAME *frame) {
  char *binary = (char*)malloc(MAX_BUFFER);
  int payload_offset = 2;

  frame->fin = 1;
		   
    
  memset(binary, 0, MAX_BUFFER);
  if(frame->fin == 1) { binary[0] = binary[0] | 0x80; }
  if(frame->rsv1 == 1) { binary[0] = binary[0] | 0x80>>WS_RSV1; }
  if(frame->rsv2 == 1) { binary[0] = binary[0] | 0x80>>WS_RSV2; }
  if(frame->rsv3 == 1) { binary[0] = binary[0] | 0x80>>WS_RSV3; }

  binary[0] = binary[0] | frame->opcode;

  /* Set Mask bit to false and size */
  if(frame->payload_len <= WSF_MAX_LEN8) {
    binary[1] = frame->payload_len;
  } else {
    binary[1] = WSF_LEN16;
   binary[2] = 0xFF & frame->payload_len;
    binary[3] = 0xFF & frame->payload_len>>8;
    payload_offset = 4;
  }
  
  int i;
  for(i = 0; i < MAX_BUFFER && i <= frame->payload_len; ++i) {
    binary[payload_offset+i] = (int)frame->payload[i];
  }
  frame->size = payload_offset + i - 1;

  return binary;
}

void websocket_debug_raw_frame(char *output, int len) {
  int i;
  int c = 0;
  for(c = 0; c <= MAX_BUFFER && c <= len; ++c) {
    for(i = 0; i <= 7; ++i) {
      log_string("%2i: %u", c, !!(output[c] & (0x80>>i)));
    }
  }
}

int websocket_send_frame(WEBSOCKET_FRAME *frame, WEBSOCKET_DATA *sock) {
  char *output = websocket_build_frame(frame);
  //  websocket_debug_raw_frame(output, frame->size);

  send(sock->uid, output, frame->size, 0);
  free(output);
  return 0;
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
    WEBSOCKET_FRAME *frame = websocket_new_frame();

    frame->fin = !!(input[0] & 0x80);
    frame->rsv1 = !!(input[0] & (0x80<<WS_RSV1));
    frame->rsv2 = !!(input[0] & (0x80<<WS_RSV2));
    frame->rsv3 = !!(input[0] & (0x80<<WS_RSV3));
    frame->masked = !!(input[1] & 0x80);
    frame->opcode = (char)(input[0] & 0x0F);

    frame->payload_len = (char)(input[1] ^ 0x80);

    frame->payload[0] = '\0';

    int mask_start = 2;

    if( frame->payload_len == 126 ) {
      mask_start = 4;
      frame->payload_len16 = ntohs(input[2] | (uint16_t)(input[3]<<8));
      log_string("%u", frame->payload_len16);
      frame->payload_len = frame->payload_len16;
    }

    /* Untested */
    if( frame->payload_len == 127 ) {
      mask_start = 8;
      frame->payload_len64 = ntohs(input[2] | (uint16_t)(input[3]<<8) | (uint16_t)(input[3]<<16) | (uint16_t)(input[3]<<24));
      frame->payload_len = frame->payload_len64;
    }
    
    int mask_end = mask_start + WS_MASK_SIZE;

    if(frame->masked == 1 && frame->payload_len > 0) {
      for (c = mask_start; c < MAX_INPUT_LEN && c <( frame->payload_len + WS_MASK_SIZE + mask_start); c++) {
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

void websocket_send_txt(WEBSOCKET_DATA *sock, char *msg) {
  WEBSOCKET_FRAME *oframe = websocket_new_frame();
  
  oframe->payload = strdup(msg);
  oframe->opcode = WSF_TXT;
  oframe->payload_len = strlen(oframe->payload);
  
  websocket_send_frame(oframe, sock);
  websocket_delete_frame(oframe);
}

void websocket_send_bin(WEBSOCKET_DATA *sock, char *msg) {
  WEBSOCKET_FRAME *oframe = websocket_new_frame();

  oframe->payload = strdup(msg);
  oframe->opcode = WSF_BIN;
  oframe->payload_len = strlen(oframe->payload);

  websocket_send_frame(oframe, sock);
  websocket_delete_frame(oframe);
}

void websocket_handle_input(WEBSOCKET_DATA *conn) {
  WEBSOCKET_COMMAND *cmd;
  LIST_ITERATOR *cmd_i = newListIterator(conn->input);
  int close = 0;
  
  ITERATE_LIST(cmd, cmd_i) {
    switch(cmd->opcode) {
    case WSF_CLOSE:
      listPush(conn->output, websocket_new_command(WSF_CLOSE, cmd->payload));
      close = 1;
      listRemove(conn->input, cmd);
      break;
    case WSF_PING:
      listQueue(conn->output, websocket_new_command(WSF_PONG, cmd->payload));
      listRemove(conn->input, cmd);
      break;
    }
  } deleteListIterator(cmd_i);
}

void websocket_broadcast_txt(char *msg) {
  LIST_ITERATOR *conn_i = newListIterator(ws_descs);
  WEBSOCKET_DATA *test;
  ITERATE_LIST(test, conn_i) {
    listQueue(test->output, websocket_new_command(WSF_TXT, msg));
  } deleteListIterator(conn_i);

}

void websocket_handle_output(WEBSOCKET_DATA *conn) {
  WEBSOCKET_COMMAND *cmd;
  LIST_ITERATOR *cmd_i = newListIterator(conn->output);
  int close = 0;
  
  ITERATE_LIST(cmd, cmd_i) {
    switch(cmd->opcode) {
    case WSF_CLOSE:
      websocket_send_close(conn, cmd->payload);
      close = 1;
      break;
    case WSF_PONG:
      //      websocket_send_pong(conn, cmd->payload);
      break;
    case WSF_TXT:
      websocket_send_txt(conn, cmd->payload);
      break;
    case WSF_BIN:
      websocket_send_bin(conn, cmd->payload);
      break;
    }

    listRemove(conn->output, cmd);

  } deleteListIterator(cmd_i);

  if(close == 1) {
    websocket_destroy(conn);
  }


    
    /*  if(cmd->opcode = WSF_TXT) {
    websocket_send_txt(conn, cmd->payload);
    }*/
}

void handleWebSocket(WEBSOCKET_DATA *sock) {

  char b64in[MAX_BUFFER]; memset(b64in, 0, MAX_BUFFER);
  char sha1[40]; memset(sha1, 0, 40);
  char dest[b64max(40)];
  char *cSave, *cTok, *ch, swap;

  /* Are we connected yet? If not attempt a handshake */
  if(sock->connected != 1) {
      BUFFER     *buf = newBuffer(MAX_BUFFER);

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
	  /*	  CHAR_DATA *ch = newChar();
	  charSetName(ch, "Guest");

	  	  
	  // give the character a unique id
	  int next_char_uid = mudsettingGetInt("puid") + 1;
	  mudsettingSetInt("puid", next_char_uid);
	  charSetUID(ch, next_char_uid);

	  char_exist(ch);
	  sock->ch = ch;*/
	  break;
	}
	ch = strtok_r(NULL, "\n", &cSave);
      }

      if (sock->connected != 1) {
	deleteBuffer(buf);
	websocket_destroy(sock);
	return;
      }

      bprintf(buf, "HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\nUpgrade: websocket\r\n\r\n", dest);
      
      // send out the buf contents
      send(sock->uid, bufferString(buf), strlen(bufferString(buf)), 0);
      sock->input_buf[0] = '\0';
      sock->input_length = 0;
      deleteBuffer(buf);
    }
  } else {
    /* We are connected, check the input buffer for commands, */

    /* Deframe the data */
    WEBSOCKET_FRAME *iframe = websocket_deframe(sock->input_buf);

    /* Invalid data, bailing! */
    if(iframe == NULL) {
      log_string("%s", "Failed to decoe frame");
      return;
    }

    if(iframe->fin == 0) {
      listQueue(sock->frame_frags, iframe);
      return;
    } else if (iframe->fin == 1 && !isListEmpty(sock->frame_frags)) {

    }

    listQueue(sock->input, websocket_new_command(iframe->opcode, iframe->payload));

    sock->input_buf[0] = '\0';
    sock->input_length = 0;

    websocket_delete_frame(iframe);
  }
  // clean up our mess

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
    } else if(in_len <= 0) {
      websocket_destroy(conn);
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
      websocket_handle_input(conn);
      websocket_handle_output(conn);
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
    websocket_destroy(conn);
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
