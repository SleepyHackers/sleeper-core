#include <python2.7/Python.h>        // to add Python hooks

#include "../mud.h"
#include "../utils.h"
#include "../hashtable.h"
#include "../room.h"
#include "../exit.h"
#include "../character.h"
#include "../list.h"

#include "party.h"

LIST *leaders = NULL, *followers;


typedef struct {
  CHAR_DATA *leader;
  LIST *followers;
} LEADER_DATA;

typedef struct {
  CHAR_DATA *follower;
  CHAR_DATA *leader;
} FOLLOWER_DATA;


/* leader comparator for list* functions */
int leaderFind(const CHAR_DATA *cmpto, const LEADER_DATA *cur) {
  return cur->leader != cmpto;
}

/* follower comparator for list* functions */
int followerListFind(const CHAR_DATA *cmpto, const LEADER_DATA *cur) {
  return cur != cmpto;
}

int followerFind(const CHAR_DATA *cmpto, const FOLLOWER_DATA *cur) {
  return cur->follower != cmpto;
}

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

/* Creates a new leader, with a first follower */
FOLLOWER_DATA *newFollower(CHAR_DATA *cdLeader, CHAR_DATA *cdFollower) {
  FOLLOWER_DATA *fdNew = malloc(sizeof(FOLLOWER_DATA));
  
  /* Set the leader, create a new list of followers, add the follower */
  fdNew->leader = cdLeader;
  fdNew->follower = cdFollower;

  return fdNew; /* Return the new struct */
}

void deleteFollower(FOLLOWER_DATA *ldFollower) {
  /* cleanup */
  free(ldFollower);
}


/* Hook to run on "enter", allows following of NPCs and PCs */
void doFollow(char *info) {
  char buffy[MAX_BUFFER];
  char buffy1[MAX_BUFFER];

  LEADER_DATA *ldTmp = NULL;
  CHAR_DATA *cdTmp = NULL, *cdTarget = NULL;
  ROOM_DATA *rdTmp = NULL, *rdCur = NULL;

  hookParseInfo(info, &cdTarget, &rdTmp);
 
  /* Find our target Char */
  ldTmp = listGetWith(leaders, cdTarget, leaderFind);

  if(ldTmp != NULL) {
    const char *cDir = NULL;
    EXIT_DATA *edTmp = NULL;
    rdCur = charGetRoom(cdTarget);

    HASH_ITERATOR *hiI = newHashIterator(rdCur->exits);
    ITERATE_HASH(cDir, edTmp, hiI) {
      if(exitGetRoom(edTmp) == rdTmp) {
	send_to_char(cdTarget, "match found\r\n");
      }
    } deleteHashIterator(hiI);

  }
}


/* Adds char to the follower/leader lists */
void startFollow(CHAR_DATA *ch, char *arg) {
  CHAR_DATA *cdTmp = NULL, *cdTmp1 = NULL;
  FOLLOWER_DATA *fdTmp = NULL;
  LEADER_DATA *ldTmp = NULL;
 

  /* Find the char, if in the room and can see */
  /*
    leader = CHAR_DATA *find_char(ch, LIST *list, int num, const char *name,
				 const char *prototype, bool must_see);
  */

   cdTmp = find_char(ch, roomGetCharacters(charGetRoom(ch)), 1, arg, NULL, TRUE);

   /* Test to make sure char is not following */
   fdTmp = listGetWith(followers, ch, followerFind);
   if(fdTmp == NULL) {

     if(cdTmp == NULL) {  /* Did we fail to find the char? */
       send_to_char(ch, "There is no %s to follow.\r\n", arg);
       
     } else if(cdTmp == ch) { /* Cannot follow yourself */
       send_to_char(ch, "You cannot follow yourself!\r\n");
       
     } else { /* All clear, follow the leader now if possible */
       
       ldTmp = listGetWith(leaders, cdTmp, leaderFind);
       
       if(ldTmp != NULL) {
	 cdTmp1 = listGetWith(ldTmp->followers, ch, followerListFind);
	 
	 if(cdTmp1 == NULL) {
	   listPut(ldTmp->followers, ch);
	   listPut(followers, newFollower(cdTmp, ch));

	   send_to_char(ch, "Now following %s.\r\n", charGetName(cdTmp));
	 } else {
	   send_to_char(ch, "You are already following %s.\r\n", charGetName(cdTmp));	
	 }
       } else {
	 listPut(leaders, newLeader(cdTmp, ch));
	 listPut(followers, newFollower(cdTmp, ch));
	 send_to_char(ch, "Now following %s.\r\n", charGetName(cdTmp));
       }
     }
   } else {
     send_to_char(ch, "Already following someone.\r\n", charGetName(fdTmp->leader));
   }
}

/* Removes char from the follower/leader lists */
void stopFollow(CHAR_DATA *ch) {
  /* Find our leader */
  CHAR_DATA *cdTmp = NULL, *cdTmp1 = NULL;
  FOLLOWER_DATA *fdTmp = NULL;
  LEADER_DATA *ldTmp = NULL;

   /* Test to make sure char is not following */
   fdTmp = listGetWith(followers, ch, followerFind);
  
   if(fdTmp != NULL) {
     send_to_char(ch, "You no longer follow %s.\r\n", charGetName(fdTmp->leader));
     /* Delete from leaders followers */
     ldTmp = listGetWith(leaders, fdTmp->leader, leaderFind);
     listRemoveWith(ldTmp->followers, ch, followerListFind);

     /* Last follower? Delete leader */
     if(isListEmpty(ldTmp->followers)) {
       listRemoveWith(leaders, fdTmp->leader, leaderFind);
       deleteLeader(ldTmp);
     }

     /* Delete follower data */
     listRemoveWith(followers, ch, followerListFind);
     deleteFollower(fdTmp);


   } else {
     send_to_char(ch, "follow {target}\r\n");
   }
  
}

/* (un)Follows a target player. Takes one or no args. 
   arg1 - The target
   If no args are given, stops following players

   TODO: Add a pointers to allow quick unfollows
*/
COMMAND(cmd_follow) {
  /* Our cstring buffers */
  char buffy[MAX_BUFFER];
  char buffy1[MAX_BUFFER];

  /* Test for no args */
  if(compares(arg, "")) {
    stopFollow(ch);
  } else {
    arg = one_arg(arg, buffy);  /* Get the first arg */ 
    startFollow(ch, buffy);
  }

}

/* Init us in the mainloop */
void init_party(void) {
  add_cmd("follow", NULL, cmd_follow, "player", FALSE); /* Add the follow command */
  hookAdd("enter", doFollow);                           /* Add the follow hook to "enter" */
  leaders = newList();                                  /* Init our list of leaders */
  followers = newList();                                /* Ini tour list of followers */
}
