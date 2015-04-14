#include <string.h>
#include <stdlib.h>
#include "mud.h"
#include "utils.h"
#include "randistrs.h"
#include "dice.h"

int srDie() { 
  int n = rand_number(1, 6);

  if (n == 6) { return (n+(srDie())); }
  else        { return n; }
}

int* srDice(int dice) {
  int* roll = malloc((sizeof(int) * dice));
  memset(roll, 0, sizeof(roll));
  int die;

  for (die=0; die <= dice; die++) {
    roll[die] = srDie();
  }

  return roll;
}

char* formatRoll(int* roll, int dice, int tn) {
  int die;
  char* fdice[dice];
  char* buf = malloc(( (sizeof(char) * 4) * dice ));

  for (die=0; die <= dice; die++) {
    if (roll[die] >= tn) {
      fdice[die] = malloc((sizeof(char) * 4));
      sprintf(buf, "\033[1m%d\033[0m ", roll[die]);
    }
    else {
      fdice[die] = malloc(sizeof(char));
      sprintf(buf, "%d ", roll[die]);
    }
    strcat(buf, fdice[die]);
    free(fdice[die]);
  }

  return buf;
}

COMMAND(roll) {
  //Need to get two int args here as # of dice and TN
}

