#include <stdlib.h> //For malloc
//CFLAGS in module.mk allows us a nicer syntax here
#include "mud.h" //Have to include this for pretty much anything
#include "storage.h"
#include "auxiliary.h"
#include "world.h"


//Structs for character data

struct char_stats {
  int strength;
  int body;
  int quickness;
  int charisma;
  int willpower;
  int intelligence;
  long money;
  long bank_balance;
  int physical_condition;
  int mental_condition;
  float essence;
  float bio_index;
}; //Semicolon at end of struct definitions
       
       
struct char_stats *newStats () { //Struct keyword
  //Calloc is almost never necessary, a plain malloc will suffice
  struct char_stats *character = malloc(sizeof(*character)); //Must use struct keyword

  //Pointer access operator is ->
  character->strength              = 1;
  character->body                  = 1;
  character->quickness             = 1;
  character->charisma              = 1;
  character->willpower             = 1;
  character->intelligence          = 1;
  character->money                 = 0;
  character->bank_balance          = 0;
  character->physical_condition    = 10;
  character->mental_condition      = 10;
  character->essence               = 6;
  character->bio_index             = 9;
  
  //Return pointer to new character
  return character;
}

//to retrieve values

//Gotta tell the compiler it's a struct here, unless you use a typedef somewhere.
int charGetStrength (struct char_stats *character) {
  return character->strength;
}

//charGetBody function name already taken
int charGetBod (struct char_stats *character) {
  return character->body;
}

int charGetQuickness (struct char_stats *character) {
  return character->quickness;
}

int charGetCharisma (struct char_stats *character) {
  return character->charisma;
}

int charGetWillpower (struct char_stats *character) {
  return character->willpower;
}

int charGetIntelligence (struct char_stats *character) {
  return character->intelligence;
}

long charGetBankBal (struct char_stats *character) {
  return character->bank_balance;
}       

long charGetMoney (struct char_stats *character) {
  return character->money;
}
       
float getEssence (struct char_stats *character) {
  return character->essence;
}

float getBioIndex (struct char_stats *character) {
  return character->bio_index;
}

