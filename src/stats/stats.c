#include <string.h>
#include <stdlib.h> //For malloc
//CFLAGS in module.mk allows us a nicer syntax here
#include "mud.h" //Have to include this for pretty much anything
#include "handler.h"
#include "storage.h"
#include "auxiliary.h"
#include "world.h"
#include "character.h"

//Structs for character data

//When you malloc this, you get a pointer to a bigass memory block that fits the size
//of all these ints, they arent individually allocated - in fact you dont have anything
//allocated in there, no strings or anything
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
  double essence;
  double bio_index;
}; //Semicolon at end of struct definitions
       
       
struct char_stats *newCharStats () { //Struct keyword
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
  character->essence               = 6.0;
  character->bio_index             = 9.0;
  
  //Return pointer to new character
  return character;
}

//ok, wait, goddamn em
void deleteCharStats(struct char_stats *character) {
  free(character);
} 

struct char_stats* copyCharStats(struct char_stats *character) {
  struct char_stats* new = newCharStats();
  memcpy(new, character, sizeof(*new));
  return new;
}

void copyCharStatsTo(struct char_stats *character, struct char_stats *target) {
  memcpy(target, character, sizeof(*target));
}

/*
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
  double essence;
  double bio_index;
*/



STORAGE_SET* storeCharStats(struct char_stats *character) {
  STORAGE_SET* set = new_storage_set();
  
  store_int(set, "strength", character->strength);
  store_int(set, "body", character->body);
  store_int(set, "quickness", character->quickness);
  store_int(set, "charisma", character->charisma);
  store_int(set, "willpower", character->willpower);
  store_int(set, "intelligence", character->intelligence);
  store_long(set, "money", character->money);
  store_long(set, "bank_balance", character->bank_balance);
  store_int(set, "physical_condition", character->physical_condition);
  store_int(set, "mental_condition", character->mental_condition);
  store_double(set, "essence", character->essence);
  store_double(set, "bio_index", character->bio_index);

  return set;
}

struct char_stats* readCharStats(STORAGE_SET *set) {
  struct char_stats* character = newCharStats();
  
  //dont have to dereference the character, character->strength is equivalent
  //to (*character).strength is there an intdup? dont need it, just gotta call the
  //you have to copy strings because they are pointers to memory
  character->strength = read_int(set, "strength");
  character->body = read_int(set, "body");
  character->quickness = read_int(set, "quickness");
  character->charisma = read_int(set, "charisma");
  character->willpower = read_int(set, "willpower");
  character->intelligence = read_int(set, "intelligence");
  character->money = read_long(set, "money");
  character->bank_balance = read_long(set, "bank_balance");
  character->physical_condition = read_long(set, "physical_condition");
  character->mental_condition = read_long(set, "mental_condition");
  character->essence = read_double(set, "essence");
  character->bio_index = read_double(set, "bio_index");

  return character;
}

    /*
well, what do you think?
looks...perfect 
     */  


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
       
double getEssence (struct char_stats *character) {
  return character->essence;
}

double getBioIndex (struct char_stats *character) {
  return character->bio_index;
}

void init_stats() {
  AUXILIARY_FUNCS* stats_aux = newAuxiliaryFuncs(AUXILIARY_TYPE_CHAR, 
						 newCharStats,
						 deleteCharStats,
						 copyCharStatsTo,
						 copyCharStats,
						 storeCharStats,
						 readCharStats);
  auxiliariesInstall("Character Stats", stats_aux);
}
