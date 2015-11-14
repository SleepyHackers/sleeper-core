#include "mud.h"
#include "buffer.h"
#include "utils.h"
#include "list.h"
#include "room.h"
#include "socket.h"
#include "hooks.h"
#include "event.h"
#include "inform.h"


const char   MODULE_NAME[]     = "webui_mud";
const char   MODULE_DESC[]     = "WebUI to legacy mud translator";
const char   MODULE_DEPENDS[]  = "webui_proto";
const double MODULE_VERSION    = 1.0;

void webuimud_say(char *msg) {

}

void init_webui_mud() {
  webuiproto_add_cmd("MSG", webuimud_say);
}

void destroy_webui_mud() {
  webuiproto_remove_cmd();
}

bool onLoad() {
  init_webui_mud();
  return TRUE;
}

bool onUnload() {
  destroy_webui_mud();
  return TRUE;
}
