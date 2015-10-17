#include<http_parser.h>
#include <string.h>
#include <stdlib.h>
#include "mud.h"
#include "character.h"
#include "utils.h"
#include "list.h"
#include "room.h"
#include "socket.h"
#include "hooks.h"



const char   MODULE_NAME[]     = "websockets";
const char   MODULE_DESC[]     = "Websocket handler";
const char   MODULE_DEPENDS[]  = "";
const double MODULE_VERSION    = 1.0;


void doTest(char *info) {
  SOCKET_DATA *sock = NULL;
  hookParseInfo(info, &socket);
  //  char *cmd = listPop(sock->input);
  int uid = socketGetUID(sock);
  log_string("%d", uid);
}

void init_websockets() {
  hookAdd("receive_connection", (void*)doTest);
}

void destroy_websockets() {
  hookRemove("receive_connection", (void*)doTest);
}

bool onLoad() {
  init_websockets();
  return TRUE;
}

bool onUnload() {
  destroy_websockets();
  return TRUE;
}
