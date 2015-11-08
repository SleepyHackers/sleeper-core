#ifndef WEBSOCKETS_H
#define WEBSOCKETS_H

#define WEB_PORT 2067

void init_websockets(void);

void finalize_webserver(void);

void websocket_broadcast_txt(char *msg);

#define GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

/* Offsets */
#define WS_FIN     0
#define WS_RSV1    1
#define WS_RSV2    2
#define WS_RSV3    3
#define WS_OPCODE  4
#define WS_MASK    8
#define WS_LEN     9
#define WS_EXLEN   16
#define WS_MASK    16

/* OP CODES */
#define WSF_CONT   0
#define WSF_TXT    1
#define WSF_BIN    2
#define WSF_CLOSE  8
#define WSF_PING   9
#define WSF_PONG   10

#define WS_MASK_SIZE 4

#define WSF_MAX_LEN8 125
#define WSF_LEN16 126
#define WSF_MAX_LEN16 127

#define WSF_MAX_LEN32 127


//*the datstructure for a websocket descriptor */
typedef struct websocket_data {
  int uid;
  struct sockaddr_in stAddr;
  char input_buf[MAX_INPUT_LEN];
  int input_length;
  int connected;
  int die;
  CHAR_DATA *ch;
  LIST *input;
  LIST *output;
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
  int size;
} WEBSOCKET_FRAME;

typedef struct websocket_command {
  int opcode;
  char *payload;
} WEBSOCKET_COMMAND;

LIST *ws_descs = NULL;
#endif
