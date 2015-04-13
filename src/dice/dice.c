#include <stdlib.h>
#include "mud.h"
#include "utils.h"
#include "randistrs.h"
#include "dice.h"

int srdie() { 
  int n = rand_number(1, 6);

  if (n == 6) { return (n+(srdie())); }
  else        { return n; }
}

int* srdice(int dice) {
  int* roll = malloc((sizeof(int) * dice));
  int die=0;

  for (;;) {
    if (die > dice) {
      break;
    }
    roll[die] = srdie();
    die++;
  }

  return roll;
}
