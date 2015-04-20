#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "mud.h"
#include "dice.h"
#include "character.h"
#include "utils.h"
#include "list.h"

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
    if (roll[die] > tn) {
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

COMMAND(sroll) {
  const char* SYNTAX = "Syntax is: sroll <dice> [tn] [sides]\r\n";

  int* roll;
  char* msg;
  void *int_args = NULL;
  int dice=0; int tn=0; int sides=6; int successes;
  bool multiple = FALSE;

  if (!parse_args(ch, FALSE, cmd, arg, "int.multiple", &int_args, &multiple)) {
    send_to_char(ch, SYNTAX);
    return;
  }

  if (multiple == FALSE) {
    dice = (int)int_args;                    //Pointer to int cast intentional
    roll = rollDice(dice, sides, srDie);
    msg = dumpRoll(roll, dice);
    send_to_char(ch, "You roll %d 6-sided Shadowrun dice :: %s\r\n", dice, msg);
    free(msg);
  }
  else {
    dice = (int)listPop((LIST*)int_args);    //Pointer to int cast intentional
    if (!(isListEmpty((LIST*)int_args))) {
      tn = (int)listPop((LIST*)int_args);    //Pointer to int cast intentional
    }
    if (!(isListEmpty((LIST*)int_args))) {
      sides =(int)listPop((LIST*)int_args);  //Pointer to int cast intentional
    }
    roll = rollDice(dice, sides, srDie);
    msg = dumpRollTN(roll, dice, tn);
    successes = testRoll(roll, dice, tn);
    send_to_char(ch, "You roll %d %d-sided Shadowrun dice at TN %d :: %s :: %d successes.\r\n", dice, sides, tn, msg, successes);
    deleteList(int_args);
    free(msg);
  }
  free(roll);
}

void init_dice() {
  add_cmd("sroll", NULL, sroll, "player", FALSE);
}
