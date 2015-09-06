#include <string.h>
#include <stdlib.h> 
//CFLAGS in module.mk allows us a nicer syntax here
#include "mud.h" 
#include "handler.h"
#include "storage.h"
#include "auxiliary.h"
#include "world.h"
#include "character.h"

//Module information
const char   MODULE_NAME[]    = "stats";
const char   MODULE_DESC[]    = "Installs auxiliary statistic, attribute, nuyen, and karma data on characters";
const char   MODULE_DEPENDS[] = "";
const double MODULE_VERSION   = 1.0;

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
  double essence;
  double bio_index;
  int magic;
  long karma;
  long tke;
  int augStrength;
  int augBody;
  int augQuickness;
  int augCharisma;
  int augWillpower;
  int augIntelligence;
};
       
/*Return a pointer to a new char_stats struct on the heap,
  initialized with default values  */
struct char_stats *newCharStats () { 
  struct char_stats *character = malloc(sizeof(*character)); 

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
  character->magic                 = 6;
  character->karma                 = 0;
  character->tke                   = 0;
  character->augStrength           = 0;
  character->augBody               = 0;
  character->augQuickness          = 0;
  character->augCharisma           = 0;
  character->augWillpower          = 0;
  character->augIntelligence       = 0;

  return character;
}

//Free a previously allocated char_stats struct
void deleteCharStats(struct char_stats *character) {
  free(character);
} 

/*Copy the contents of a char_stats struct to a new location in memory,
  and return a pointer*/
struct char_stats* copyCharStats(struct char_stats *character) {
  struct char_stats* new = newCharStats();
  memcpy(new, character, sizeof(*new));
  return new;
}

//Copy the contents of one char_stats_struct to another
void copyCharStatsTo(struct char_stats *character, struct char_stats *target) {
  memcpy(target, character, sizeof(*target));
}

//Return a char_stats struct represented as a storage set
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
  store_int(set, "magic", character->magic);
  store_long(set, "karma", character->karma);
  store_long(set, "tke", character->tke);
  store_int(set, "augStrength", character->augStrength);
  store_int(set, "augBody", character->augBody);
  store_int(set, "augQuickness", character->augQuickness);
  store_int(set, "augCharisma", character->augCharisma);
  store_int(set, "augWillpower", character->augWillpower);
  store_int(set, "augIntelligence", character->augIntelligence);

  return set;
}

//Return a char_stats struct from a storage set
struct char_stats* readCharStats(STORAGE_SET *set) {
  struct char_stats* character = newCharStats();
  
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
  character->magic = read_int(set, "magic");
  character->karma = read_long(set, "karma");
  character->tke = read_long(set, "tke");
  character->augStrength = read_int(set, "augStrength");
  character->augBody = read_int(set, "augBody");
  character->augQuickness = read_int(set, "augQuickness");
  character->augCharisma = read_int(set, "augCharisma");
  character->augWillpower = read_int(set, "augWillpower");
  character->augIntelligence = read_int(set, "augIntelligence");

  return character;
}

/* Get values for char_stats */

int charGetStrength (struct char_stats *character) {
  return character->strength;
}

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
       
double charGetEssence (struct char_stats *character) {
  return character->essence;
}

double charGetBioIndex (struct char_stats *character) {
  return character->bio_index;
}

int charGetmagic (struct char_stats *character) {
  return character->magic;
}

long charGetKarma (struct char_stats *character) {
  return character->karma;
}

long charGetTKE (struct char_stats *character) {
  return character->tke;
}

int charGetAugStrength (struct char_stats *character) {
  return character->augStrength;
}

int charGetAugBody (struct char_stats *character) {
  return character->augBody;
}

int charGetAugQuickness (struct char_stats *character) {
  return character->augQuickness;
}

int charGetAugCharisma (struct char_stats *character) {
  return character->augCharisma;
}

int charGetAugWillpower (struct char_stats *character) {
  return character->augWillpower;
}

int charGetAugIntelligence (struct char_stats *character) {
  return character->augIntelligence;
}

//Initialize the stats module, installing needed auxiliary data into the core
void init_stats() {
  auxiliariesInstall("character_stats", newAuxiliaryFuncs(AUXILIARY_TYPE_CHAR, 
                                                          newCharStats,
                                                          deleteCharStats,
                                                          copyCharStatsTo,
                                                          copyCharStats,
                                                          storeCharStats,
                                                          readCharStats));
}

void destroy_stats() {
  auxiliariesUninstall("character_stats");
}

bool onLoad() {
  init_stats();
  return TRUE;
}

bool onUnload() {
  destroy_stats();
  return TRUE;
}
