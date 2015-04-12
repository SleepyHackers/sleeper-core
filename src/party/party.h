#ifndef _PARTY_H
#define _PARTY_H

void init_party(void);

typedef struct {
  CHAR_DATA *leader;
  LIST *followers;
} LEADER_DATA;

typedef struct {
  CHAR_DATA *follower;
  CHAR_DATA *leader;
} FOLLOWER_DATA;

#endif

