#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "mud.h"
#include "dice.h"
#include "character.h"
#include "utils.h"
#include "list.h"
#include "room.h"

//Return random number between 1 and sides
int genericDie(int sides) {
  return rand_number(1, sides);
}

//Return random number between 1 and sides, applying the Shadowrun "six rule" to the # of sides
int srDie(int sides) { 
  int n = rand_number(1, sides);

  if (n == sides) { return (n+(srDie(sides))); }
  else        { return n; }
}

//Make dice roll of specified parameters as array of ints on the heap. Roll must be freed after use.
int* rollDice(int dice, int sides, int (*call)()) {
  int* roll = malloc((sizeof(int) * dice));
  memset(roll, 0, sizeof(roll));
  int die;

  for (die=0; die < dice; die++) {
    roll[die] = call(sides);
  }

  return roll;
}

//Test a roll for successes with a TN
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

//Format a roll into a string buffer. Must be freed after use.
char* dumpRoll(int* roll, int dice) {
  int die;

  char* msg = malloc((sizeof(char) * 1024));
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

//Format a roll into a string buffer, with TN testing. Must be freed after use.
char* dumpRollTN(int* roll, int dice, int tn) {
  int die;

  char* msg = malloc((sizeof(char) * 1024));
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

//Meat of the roll commands. Use user input to construct, test, and format a roll.
void do_roll(CHAR_DATA* ch, const char* cmd, char* arg, const char* dietype, int (*diefunc)()) {
  const char* SYNTAX = "Syntax is: (s)roll <dice> [tn] [sides]\r\n";

  int* roll;
  char* msg;
  CHAR_DATA* c;
  LIST_ITERATOR* chars;
  char* out = malloc((sizeof(char) * MAX_BUFFER));
  int dice=0; int tn=0; int sides=0; int successes;

  //Both TN and sides are optional
  if (!parse_args(ch, FALSE, cmd, arg, "int | int int | int int int", &dice, &tn, &sides)) {
    send_to_char(ch, SYNTAX);
    return;
  }

  //Don't overflow
  if (dice > MAX_DICE) {
    send_to_char(ch, "Sorry, max dice in one roll is %d.\r\n", MAX_DICE);
    return;
  }

  //Default sides value if not specified
  if (!(sides)) {
    sides = 6;
  }

  //Roll dice
  roll = rollDice(dice, sides, diefunc);

  //If no TN supplied (or TN 0), format roll simply
  if (!(tn)) {
    msg = dumpRoll(roll, dice);
    snprintf(out, MAX_BUFFER, "%s rolls %d %d-sided %s dice :: %s\r\n", charGetName(ch), dice, sides, dietype, msg);
    free(msg);
  }

  //Otherwise, use dumpRollTN and testRoll to get formatted, tested output
  else {
    msg = dumpRollTN(roll, dice, tn);
    successes = testRoll(roll, dice, tn);
    snprintf(out, MAX_BUFFER, "%s rolls %d %d-sided %s dice at TN %d :: %s :: %d successes.\r\n", charGetName(ch), dice, sides, dietype, tn, msg, successes);
    free(msg);
  }

  //Send message to all characters in the room
  chars = newListIterator(roomGetCharacters(charGetRoom(ch)));
  ITERATE_LIST(c, chars) {
    send_to_char(c, out);
  } deleteListIterator(chars);

  free(out);
  free(roll);
}

//Commands
COMMAND(sroll) { do_roll(ch, cmd, arg, "Shadowrun", srDie); }
COMMAND(roll)  { do_roll(ch, cmd, arg, "generic", genericDie); }

//Initialize module
void init_dice() {
  add_cmd("sroll", NULL, sroll, "player", FALSE);
  add_cmd("roll", NULL, roll, "player", FALSE);
}
