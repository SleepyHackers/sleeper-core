#ifndef MODULES_H_
#define MODULES_H_
#include "mud.h"

#define AUTOLOAD_FILE "../lib/modules.init" //Autoload list database file

void init_modules();
bool moduleLoaded();
bool patchCore(char* sym, void* fp);

#endif
