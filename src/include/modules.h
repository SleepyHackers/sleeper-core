#ifndef MODULES_H_
#define MODULES_H_
#include "mud.h"

void init_modules();
bool moduleLoaded();
bool patchCore(char* sym, void* fp);

#endif
