#include <python2.7/Python.h>        // to add Python hooks

#include "../mud.h"
#include "../utils.h"
#include "../hashtable.h"
#include "../room.h"
#include "../exit.h"
#include "../character.h"
#include "../list.h"
#include "../hooks.h"

#include "party.h"
#include "follow.h"

/* Init us in the mainloop */
void init_party(void) {
  init_follow();
}
