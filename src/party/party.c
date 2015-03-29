#include <python2.7/Python.h>        // to add Python hooks

#include "../mud.h"
#include "../character.h"

#include "party.h"

CHAR_DATA *leader = NULL;

COMMAND(cmd_follow) {
  leader = CHAR_DATA *find_char(ch, LIST *list, int num, const char *name,
				const char *prototype, bool must_see);
  send_to_char(ch, "Hello, world!\r\n");

}

void init_party(void) {
  add_cmd("follow", NULL, cmd_follow, "player", FALSE);

}
