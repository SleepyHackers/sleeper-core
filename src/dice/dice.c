#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "mud.h"
#include "dice.h"
#include "character.h"
#include "utils.h"
#include "list.h"
#include "room.h"

int genericDie(int sides) {
  return rand_number(1, sides);
}

int srDie(int sides) { 
  int n = rand_number(1, sides);

  if (n == sides) { return (n+(srDie(sides))); }
  else        { return n; }
}

int* rollDice(int dice, int sides, int (*call)()) {
  int* roll = malloc((sizeof(int) * dice));
  memset(roll, 0, sizeof(roll));
  int die;

  for (die=0; die < dice; die++) {
    roll[die] = call(sides);
  }

  return roll;
}

int testRoll(int* roll, int dice, int tn) {
  int die;
  int successes=0;

  for (die=0; die < dice; die++) {
    if (roll[die] >= tn) {
      successes++;
    }
  }

  return successes;
}

char* dumpRoll(int* roll, int dice) {
  int die;

  char* msg = malloc((sizeof(char) * 128));
  char* buf = malloc((sizeof(char) * 4));
  memset(msg, '\0', sizeof(*msg));
  memset(buf, '\0', sizeof(*buf));
 
  for (die=0; die < dice; die++) {
    snprintf(buf, 4, " %d", roll[die]);
    strncat(msg, buf, 128);
  }

  free(buf);
  return msg;
}

char* dumpRollTN(int* roll, int dice, int tn) {
  int die;

  char* msg = malloc((sizeof(char) * 128));
  char* buf = malloc((sizeof(char) * 8));
  memset(msg, '\0', sizeof(*msg));
  memset(buf, '\0', sizeof(*buf));

  for (die=0; die < dice; die++) {
    if (roll[die] >= tn) {
      snprintf(buf, 8, "{W%d{n ", roll[die]);
    }
    else {
      snprintf(buf, 8, "%d ", roll[die]);
    }
    strncat(msg, buf, 128);
  }

  free(buf);
  return msg;
}

void do_roll(CHAR_DATA* ch, const char* cmd, char* arg, const char* dietype, int (*diefunc)()) {
  const char* SYNTAX = "Syntax is: (s)roll <dice> [tn] [sides]\r\n";

  int* roll;
  char* msg;
  CHAR_DATA* c;
  LIST_ITERATOR* chars;
  char* out = malloc((sizeof(char) * MAX_BUFFER));
  int dice=0; int tn=0; int sides=0; int successes;

  if (!parse_args(ch, FALSE, cmd, arg, "int | int int | int int int", &dice, &tn, &sides)) {
    send_to_char(ch, SYNTAX);
    return;
  }

  if (!(sides)) {
    sides = 6;
  }

  if (!(tn)) {
    roll = rollDice(dice, sides, diefunc);
    msg = dumpRoll(roll, dice);
    snprintf(out, MAX_BUFFER, "%s rolls %d %d-sided %s dice :: %s\r\n", charGetName(ch), dice, sides, dietype, msg);
    free(msg);
  }

  else {
    roll = rollDice(dice, sides, diefunc);
    msg = dumpRollTN(roll, dice, tn);
    successes = testRoll(roll, dice, tn);
    snprintf(out, MAX_BUFFER, "%s rolls %d %d-sided %s dice at TN %d :: %s :: %d successes.\r\n", charGetName(ch), dice, sides, dietype, tn, msg, successes);
    free(msg);
  }

  chars = newListIterator(roomGetCharacters(charGetRoom(ch)));
  ITERATE_LIST(c, chars) {
    send_to_char(c, out);
  } deleteListIterator(chars);
  free(out);
  free(roll);
}

COMMAND(sroll) { do_roll(ch, cmd, arg, "Shadowrun", srDie); }
COMMAND(roll)  { do_roll(ch, cmd, arg, "generic", genericDie); }

void init_dice() {
  add_cmd("sroll", NULL, sroll, "player", FALSE);
  add_cmd("roll", NULL, roll, "player", FALSE);
}
