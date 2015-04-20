#ifndef DICE_H_
#define DICE_H_

#define DICE_TYPES "generic, sr"
#define DICE_GENERIC 0
#define DICE_SR 1

#define srDice(d) rollDice(d, 6, DICE_SR)
#define genericDice(d,s) rollDice(d, s, DICE_GENERIC)

void init_dice();

#endif
