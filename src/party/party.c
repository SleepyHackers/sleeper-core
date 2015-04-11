#include <python2.7/Python.h>        // to add Python hooks

#include "../mud.h"
#include "../character.h"
#include "../list.h"

#include "party.h"

LIST *leaders = NULL;


/* Creates a new leader, with a first follower */
LEADER_DATA *newLeader(CHAR_DATA *cdLeader, CHAR_DATA *cdFollower) {
  LEADER_DATA *ldNew = malloc(sizeof(LEADER_DATA));
  
  /* Set the leader, create a new list of followers, add the follower */
  ldNew->leader = cdLeader;
  ldNew->followers = newList();
  listPut(ldNew->followers, cdFollower);

  return ldNew; /* Return the new struct */
}

void deleteLeader(LEADER_DATA *ldLeader) {
  /* cleanup */
  if(ldLeader->followers) {
    deleteList(ldLeader->followers);
  }

  free(ldLeader);
}

/* Hook to run on "enter", allows following of NPCs and PCs */
void doFollow(char *arg) {
  char buffy[MAX_BUFFER];
  char buffy1[MAX_BUFFER];

  arg = two_args(arg, buffy, buffy1);

  LIST_ITERATOR *i = newListIterator(leaders);
  LEADER_DATA *ldTmp = NULL;
  CHAR_DATA *cdTmp = NULL;
 
  /* Find our target Char */

  ITERATE_LIST(ldTmp, i) {
    send_to_char(ldTmp->leader, "%i\r\n", charGetUID(ldTmp->leader));
    if(ldTmp->leader == cdTmp) {
      send_to_char(ldTmp->leader, "Blah, follows you\r\n");
    }
  } deleteListIterator(i);
}

COMMAND(cmd_follow) {
  /* Our cstring buffers */
  char buffy[MAX_BUFFER];
  char buffy1[MAX_BUFFER];

  CHAR_DATA *cdTmp = NULL; /* Temp pointer for leader */

  arg = one_arg(arg, buffy);  /* Get the first arg */  

  /* Find the char, if in the room and can see */
  /*
    leader = CHAR_DATA *find_char(ch, LIST *list, int num, const char *name,
				 const char *prototype, bool must_see);
  */

   cdTmp = find_char(ch, roomGetCharacters(charGetRoom(ch)), 1, buffy, NULL, TRUE);

   if(cdTmp == NULL) {  /* Did we fail to find the char? */
     send_to_char(ch, "There is no %s to follow.\r\n", buffy);

   } else if(cdTmp == ch) { /* Cannot follow yourself */
     send_to_char(ch, "You cannot follow yourself!\r\n");

   } else { /* All clear, follow the leader now if possible */

    LIST_ITERATOR *i = newListIterator(leaders);
    LEADER_DATA *ldTmp = NULL;
    bool bFound = FALSE;
    
    ITERATE_LIST(ldTmp, i) {
      if(ldTmp->leader == cdTmp) {
	listPut(ldTmp->followers, ch);
	bFound = TRUE;
	send_to_char(ch, "Now following %s.\r\n", charGetName(cdTmp));
	break;
      }
    } deleteListIterator(i);
   
    if(bFound == FALSE) {
      listPut(leaders, newLeader(cdTmp, ch));
      send_to_char(ch, "Now following %s.\r\n", charGetName(cdTmp));
    }
    
  }

}

void init_party(void) {
  add_cmd("follow", NULL, cmd_follow, "player", FALSE); /* Add the follow command */
  hookAdd("enter", doFollow);                           /* Add the follow hook to "enter" */
  leaders = newList();                                  /* Init our list of leaders */
}
