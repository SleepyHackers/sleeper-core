#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <page.h>
#include "mud.h"

//Module information
const char   MODULE_NAME[]    = "dyncall";
const char   MODULE_DESC[]    = "Provides support for accessing arbitrary C symbols dynamically";
const char   MODULE_DEPENDS[] = "";
const double MODULE_VERSION   = 1.0;

/*
  //char* funcargs=malloc((32*127)); memset(funcargs, '\0', (32*127)); //Accounting for spaces
  a = strtok(funcargs, " ");
  while (a) {
    //funcargs[p] = a;
    send_to_char(ch, "%s - %x\r\n", a, dlsym(dl, a));
    a = strtok(NULL, " ");
    p++;
    }*/

static int* dumpSection(struct char_data* ch, int* sym) {
  send_to_char(ch, "{W[%-6x] [%-6x] [%-6x] [%-6x] [%-6x] [%-6x] [%-6x] [%-6x]{n\r\n%-8x %-8x %-8x %-8x %-8x %-8x %-8x %-8x\r\n",
               sym, (sym+1), (sym+2), (sym+3), (sym+4), (sym+5), (sym+6), (sym+7),
               *(int*)sym, *(int*)(sym+1), *(int*)(sym+2), *(int*)(sym+3),
               *(int*)(sym+4), *(int*)(sym+5), *(int*)(sym+6), *(int*)(sym+7));
  sym+=7;
  return sym;
}

COMMAND(doGetSym) {
  char* a;
  void* sym;
  int   p;
  void* dl = dlopen(NULL, RTLD_NOW);
  if (!dl) {
    send_to_char(ch, "Error: dyncall couldn't dlopen()\r\n");
    return;
  }

  //ISO/IEC 9899 sec. 5.2.4.1
  char* symname=malloc(31); memset(symname, '\0', 31);
  one_arg(arg, symname);
  
  sym = (void*)dlsym(dl, symname);
  if (!sym) {
    send_to_char(ch, "Couldn't find symbol\r\n");
    return;
  }
  
  send_to_char(ch, "'%s' @ %x \r\n", symname, sym);
  for (p=0; p<8; p++) {
    sym = dumpSection(ch, sym);
  }

  free(symname);
  dlclose(dl);
}

COMMAND(doMap) {
  const char* SYNTAX = "map <address> <bytes>\r\n";
  char* addr=malloc(8); memset(addr, '\0', 8);
  char* size=malloc(8); memset(size, '\0', 8);
  void* p;
  int   sz;
  int   i;
  two_args(arg, addr, size);

  if ((!sz) || (!addr)) {
    send_to_char(SYNTAX);
    return;
  }
  
  sscanf(addr, "%p", (void **)&p);
  sz = strtol(size, NULL, 10);
  /*
  p = mmap(p, sz, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, 0, 0);
  if (p == MAP_FAILED) {
    send_to_char("Map failed\r\n");
    return;
  }*/
  for (i=0; i<sz; i++) {
    p |= (1 << PAGE_MASK);
    p++;
  }
  send_to_char("Map successful\r\n");
}

static void init_dyncall() {
  add_cmd("getsym", NULL, doGetSym, "player", FALSE);
  add_cmd("mmap",   NULL, doMap,    "player", FALSE);
}

static void destroy_dyncall() {
  remove_cmd("getsym");
  remove_cmd("mmap");
}

bool onLoad() {
  init_dyncall();
  return TRUE;
}

bool onUnload() {
  destroy_dyncall();
  return TRUE;
}
