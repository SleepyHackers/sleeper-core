#include "mud.h"
#include "character.h"
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

void init_webui_proto() {
  log_string("hello world");
}

void destroy_webui_proto() {

}

bool onLoad() {
  init_webui_proto();
  return TRUE;
}

bool onUnload() {
  destroy_webui_proto();
  return TRUE;
}
