nclude "../storage.h"
#include "../auxiliary.h"
#include "../world.h"


//Structs for character data

struct char_stats {
	 int strength;
    int body
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
																													              
}
       
       
char_stats *newStats () {
	 char_stats *character = calloc(1, sizeof(char_stats));

    character-->strength  = 1;
    character-->body      = 1;
    character-->quickness = 1;
    character-->charisma  = 1;
    character-->willpower = 1;
    character-->intelligence  = 1;
    character-->money         = 0;
    character-->bank_balance  = 0;
    character-->physical_condition    = 10;
    character-->mental_condition      = 10;
    character-->essence               = 6;
    character-->bio_index             = 9;
}

//to retrieve values

int charGetStrength (char_stats *character) {
	    return character->strength;
}

int charGetBody (char_stats *character) {
	    return character->body;
}

int charGetQuickness (char_stats *character) {
	    return character->quickness;
}

int charGetCharisma (char_stats *character) {
	    return character->charisma;
}

int charGetWillpower (char_stats *character) {
	    return character->willpower;
}

int charGetIntelligence (char_stats *character) {
	    return character->intelligence;
}

long charGetMoney (char_stats *character) {
	    return character->money;
}
    
long charGetBankBal (char_stats *character) {
	    return character->bank_balance;
}       

int charGetMoney (char_stats *character) {
	    return character->physical_condition;
}

int charGetMoney (char_stats *character) {
	    return character->mental_condition;
}
       
float getEssence (char_stats *character) {
	      character->essence;
}

float getBioIndex (char_stats *character) {
	      character->bio_index;
}

