#include "mud.h"
#include "buffer.h"
#include "utils.h"
#include "list.h"
#include "room.h"
#include "socket.h"
#include "hooks.h"
#include "event.h"
#include "inform.h"
#include "../modules/websockets/websockets.h"

const char   MODULE_NAME[]     = "webui_protocol";
const char   MODULE_DESC[]     = "WebUI protocol handler";
const char   MODULE_DEPENDS[]  = "websockets";
const double MODULE_VERSION    = 1.0;


void webuiproto_add_cmd(char *cmd, void *func) {

}

void webuiproto_remove_cmd(char *cmd) {
  
}

void webuiproto_parse() {
  WEBSOCKET_COMMAND *cmd;

  char *msg;
  WEBSOCKET_DATA  *conn = NULL;
  LIST_ITERATOR *conn_i = newListIterator(ws_descs);
  
  ITERATE_LIST(conn, conn_i) {
    LIST_ITERATOR *cmd_i = newListIterator(conn->input);
    ITERATE_LIST(cmd, cmd_i) {
      switch(cmd->opcode) {
      case WSF_TXT:
	
	msg = (char*)malloc(MAX_BUFFER);
	
	sprintf(msg, "&lt;annon%i&gt; %s", conn->uid, cmd->payload);
	websocket_broadcast_txt(msg);
	free(msg);
	listRemove(conn->input, cmd);
	break;
	
      }
    } deleteListIterator(cmd_i);
  } deleteListIterator(conn_i);
}

void init_webui_proto() {
  start_update(0xB, 0.1 SECOND, webuiproto_parse, NULL, NULL, NULL);
}

void destroy_webui_proto() {
  interrupt_events_involving(0xB);
}

bool onLoad() {
  init_webui_proto();
  return TRUE;
}

bool onUnload() {
  destroy_webui_proto();
  return TRUE;
}
