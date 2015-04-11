#include <python2.7/Python.h>        // to add Python hooks

#include "../mud.h"
#include "../character.h"

#include "party.h"

CHAR_DATA *leader = NULL;

/* Our cstring buffers */
char buffy[MAX_BUFFER];
char buffy1[MAX_BUFFER];

COMMAND(cmd_follow) {
  CHAR_DATA *cdTmp = NULL; /* Temp pointer for leader */


  

  arg = one_arg(arg, buffy);  /* Get the first arg */  


  /* Find the char, if in the room and can see */
  //leader = CHAR_DATA *find_char(ch, LIST *list, int num, const char *name,
  //				const char *prototype, bool must_see);
  cdTmp = find_char(ch, roomGetCharacters(charGetRoom(ch)), 1, buffy, NULL, TRUE);


  if(cdTmp == NULL) {  /* Did we fail to find the char? */
    send_to_char(ch, "There is no %s to follow.\r\n", buffy);

  } else if(cdTmp == ch) { /* Cannot follow yourself */
    send_to_char(ch, "You cannot follow yourself!\r\n");

  } else { /* All clear, follow the leader now if possible */
    send_to_char(ch, "Now following %s.\r\n", charGetName(cdTmp));
  }

}

void init_party(void) {
  add_cmd("follow", NULL, cmd_follow, "player", FALSE);

}
