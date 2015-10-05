#include <python2.7/Python.h>        // to add Python hooks

#include "mud.h"
#include "utils.h"
#include "hashtable.h"
#include "room.h"
#include "exit.h"
#include "character.h"
#include "list.h"
#include "hooks.h"

#include "party.h"
#include "follow.h"

const char   MODULE_NAME[]     = "party";
const char   MODULE_DESC[]     = "Party, group, and follow system";
const char   MODULE_DEPENDS[]  = "dice-1.0";
const double MODULE_VERSION    = 1.0;


/* Init us in the mainloop */
void init_party(void) {
  init_follow();
}

void destroy_party() {
  destroy_follow();
}

//Module constructor and deconstructor
void onLoad() {
  init_party();
}

void onUnload() {
  destroy_party();
}
