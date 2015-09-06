#define _GNU_SOURCE
#include <dlfcn.h>
#include <pthread.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "modules.h"
#include "mud.h"
#include "list.h"
#include "character.h"
#include "storage.h"

//Struct used to store information about a loaded module in the module_list
struct mudModule {
  char* name;
  char* path;
  char* desc;
  char* depends;
  void* lib;
  double version;
};

LIST* module_list;   //List of modules currently loaded into the core
LIST* autoload_list; //List of modules to load on boot
char* module_dir;    //Location of compiled modules
char* autoload_file; //Location of autoload list

//Return an empty, initialized mudModule structure
static struct mudModule* newMudModule() {
  struct mudModule* new = malloc(sizeof(*new));
  new->name = malloc(16);
  new->path = malloc(1024);
  new->desc = malloc(1024);
  new->depends = malloc(1024);
  new->version = 0.0;
  new->lib = NULL;
  return new;
}

//Free a mudModule and it's allocated members
__attribute__ ((nonnull(1)))
__attribute__ ((nothrow))
static void freeMudModule(struct mudModule* m) {
  free(m->name);
  free(m->path);
  free(m->desc);
  free(m->depends);
  dlclose(m->lib);
  free(m);
}

//Get a void function pointer symbol from a library 
__attribute__ ((flatten))
__attribute__ ((nonnull(1,2)))
static inline void (*fetchCall(void* lib, char* name)) { 
  return (void (*)(void *))dlsym(lib, name); 
}

//Get a bool function pointer symbol from a library
__attribute__ ((flatten))
__attribute__ ((nonnull(1,2)))
static inline void (*fetchCallBool(void* lib, char* name)) {
  return (bool (*)(bool *))dlsym(lib, name);
}

static char* modulePath(char* name) {
  char* path = malloc(1024);
  snprintf(path, 1024, "%s/%s.so", module_dir, name);
  if (access(path, F_OK) != -1) {
    return path;
  }
  else {
    return NULL;
  }
}

static struct mudModule* buildMudModule(char* name) {
  struct mudModule* new = newMudModule();
  char* path = modulePath(name);
  if (!path) {
    log_string("Error: Can't find path for module '%s'", name);
   return NULL;
  }
  void* lib = dlopen(path, RTLD_NOW|RTLD_GLOBAL);
  if (!lib) { 
    log_string("Error: while trying to open module '%s': %s", path, dlerror());
    return NULL; 
  }
  char* desc     = (char*)dlsym(lib, "MODULE_DESC");
  char* depends  = (char*)dlsym(lib, "MODULE_DEPENDS");
  double version = *(double*)dlsym(lib, "MODULE_VERSION");

  snprintf(new->name,    16,   "%s", name);
  snprintf(new->path,    1024, "%s", path);
  snprintf(new->desc,    1024, "%s", desc);
  snprintf(new->depends, 1024, "%s", depends);
  new->version = version;
  new->lib = lib;
 
  free(path);
  return new;
}

//Check if module name is in list; return module struct if found, NULL otherwise
struct mudModule* getMudModule(char* name, LIST* list) {
  struct mudModule* m;
  LIST_ITERATOR* i = newListIterator(list);
  ITERATE_LIST(m, i) {
    if (!(strncmp(name, m->name, 1024))) {
      return m;
    }
  }
  log_string("Couldn't find module '%s' in list", name);
  return NULL;
}

//Check if module d of a minimum version is loaded - True if so, false otherwise
__attribute__ ((nonnull(1)))
static bool resolveDepend(char* d, double version) {
  struct mudModule* m = getMudModule(d, module_list);
  if (!m) { return FALSE; }
  if (m->version >= version) { return TRUE; }
  else {
    log_string("Module '%s' v%.1f doesn't meet version requirement for dependency: %s",
               m->name, m->version, d);
  }
  return FALSE;
}

//Grab module from a "module-version" string
__attribute__ ((nonnull(1)))
static char* cutModule(char* d) {
  char* dep = strndup(d, 16);
  char* mod = malloc(16); memset(mod, '\0', 16);
  int l = 0;
  while (l < 17 && *dep != '-') {
    *mod = *dep;
    l++; mod++; dep++;
  }
  return (mod-l);
}

//Grab version from a "module-version" string
__attribute__ ((nonnull(1)))
static double cutVersion(char* d) {
  char* v = strndup(d, 1024);
  while (v && (*v != '-')) { v++; }
  v++;
  return (double)strtof(v, NULL);
}

//Return false if any deps can't be satisfied.
__attribute__ ((nonnull(1)))
static bool checkDepends(struct mudModule* m) {
  char *mod;
  double version;
  char *d       = strndup(m->depends, 1024);
  char *p       = strtok(d, " ");
  while (p) {
    mod     = cutModule(p);
    version = cutVersion(p);
    if (!(resolveDepend(mod,version))) {
      log_string("Error: failed to resolve dependency '%s' v%.1f for module '%s'",
                 p, version, m->name);
      return FALSE;
    }
    p = strtok(NULL, " -");
  }
  return TRUE;
}

//Check if a module is depended on by any loaded modules
__attribute__ ((nonnull(1)))
static bool checkForDepend(char* name) {
  return FALSE;
  // TODO //
}

//Load a module by path: create a new mudModule struct for it,
//call dlOpen, fire the modules constructor, and place the struct
//in the module_list
__attribute__ ((nonnull(1)))
static bool loadModule(char* name) {
  struct mudModule* new = buildMudModule(name);
  if (!new) { return FALSE; }

  if (!(checkDepends(new))) {
    log_string("Dependency error, can't load.");
    return FALSE;
  }

  bool (*moduleLoadFunc)() = fetchCallBool(new->lib, "onLoad");
  if (!moduleLoadFunc) {
    log_string("Warning: module '%s' has no onLoad constructor", name);
  }
  else if (!(moduleLoadFunc())) {
    log_string("Error: couldn't run constructor for module '%s', not loading", name);
    freeMudModule(new);
    return FALSE;
  }

  listQueue(module_list, (void*)new);

  log_string("Module '%s' loaded successfully", name);
  return TRUE;
}

//Unload a module by path: Iterate through the module list
//until the modules name is found, remove it from the list,
//call the modules deconstructor, close the library, and
//free the struct.
__attribute__ ((nonnull(1)))
static bool unloadModule(char* name) {
  bool (*moduleUnloadFunc)();
  struct mudModule* m = getMudModule(name, module_list);
  struct mudModule* rd;
  if (!m) { 
    log_string("Couldn't find module '%s' to unload", name);
    return FALSE; 
  }

  rd = checkForDepend(name);
  if (rd) {
    log_string("Error: Module '%s' is relied on by module '%s', not unloading.\r\n",
               name, rd->name);
    return FALSE;
  }
  
  moduleUnloadFunc = fetchCallBool(m->lib, "onUnload");
  if (!moduleUnloadFunc) {
    log_string("Warning: module '%s' has no deconstructor", name);
  }
  else if (!(moduleUnloadFunc())) {
    log_string("Error: Deconstructor for module '%s' failed, not unloading", name);
    return FALSE;
  }
  if (!(listRemove(module_list, m))) {
    log_string("Couldn't remove module '%s' from module_list", name);
    return FALSE;
  }
  freeMudModule(m);
  log_string("Module '%s' unloaded successfully", name);
  return TRUE;
}

//Reload a module
__attribute__ ((nonnull(1)))
static bool reloadModule(char* name) {
  if (!(unloadModule(name))) { return FALSE; }
  if (!(loadModule(name))) { return FALSE; }
  return TRUE;
}

//Output a list of modules to a character
__attribute__ ((nonnull(1)))
static void listModules(struct char_data* ch, LIST* list, char* msg) {
  struct mudModule* m;
  LIST_ITERATOR* i = newListIterator(list);
  
  send_to_char(ch, "%s:\r\n", msg);
  ITERATE_LIST(m, i) {
    if (m) {
      send_to_char(ch, "%s - %s - %.1f - %s\r\n", 
                   m->name, m->desc, m->version, m->depends);
    }
  } deleteListIterator(i);
}

//List all available modules in the module directory
__attribute__ ((nonnull(1)))
static void listAllModules(struct char_data* ch) {
  struct dirent* dir;
  void* lib;
  char* path = malloc(1024); memset(path, '\0', 1024);
  DIR* d = opendir(module_dir);
  if (!d) {
    send_to_char(ch, "Couldn't access the module directory '%s'\r\n", module_dir);
    return;
  }

  send_to_char(ch, "Modules available to load:\r\n");
  while ((dir = readdir(d)) != NULL) {
    if (dir->d_type == DT_REG) {
      snprintf(path, 1024, "%s/%s", module_dir, dir->d_name);
      lib = dlopen(path, RTLD_LAZY);
      if (lib) {
        send_to_char(ch, "%s - %s - %.1f - %s\r\n",
                     (char*)dlsym(lib, "MODULE_NAME"),
                     (char*)dlsym(lib, "MODULE_DESC"),
                     *(double*)dlsym(lib, "MODULE_VERSION"),
                     (char*)dlsym(lib, "MODULE_DEPENDS"));
        dlclose(lib);
      }
      memset(path, '\0', 1024);
    }
  }
  free(path);
}

//Read a mudModule from a storage set
__attribute__ ((nonnull(1)))
static struct mudModule* mudModuleRead(STORAGE_SET* set) {
  struct mudModule* m = newMudModule();

  m->name    = strndup(read_string(set, "name"),    16);
  m->path    = strndup(read_string(set, "path"),    1024);
  m->desc    = strndup(read_string(set, "desc"),    1024);
  m->depends = strndup(read_string(set, "depends"), 1024);
  m->version = read_double(set, "version");

  return m;
}

//Write a mudModule to a storage set
__attribute__ ((nonnull(1)))
static STORAGE_SET* mudModuleStore(struct mudModule* m) {
  STORAGE_SET* set = new_storage_set();

  store_string(set, "name",    m->name);
  store_string(set, "path",    m->path);
  store_string(set, "desc",    m->desc);
  store_string(set, "depends", m->depends);
  store_double(set, "version", m->version);

  return set;
}

//Save the autoload list to the modules.init file on disk
static void saveAutoloadList() {
  STORAGE_SET* set = new_storage_set();

  store_list(set, "autoload_list", gen_store_list(autoload_list, mudModuleStore));
  storage_write(set, autoload_file);
  storage_close(set);
}

//Load the modules.init file from disk
static void loadAutoloadList() {
  STORAGE_SET* set = storage_read(autoload_file);
  if (!set) { return; }
  autoload_list = gen_read_list(read_list(set, "autoload_list"), mudModuleRead);
  storage_close(set);
}

//Load all modules in the autoload list
static void runAutoloadList() {
  struct mudModule* m;
  LIST_ITERATOR* i = newListIterator(autoload_list);
  ITERATE_LIST(m, i) {
    loadModule(m->name);
  } deleteListIterator(i);
}

//Add a module to the autoload list by it's path
__attribute__ ((nonnull(1)))
static void addAutoloadModule(char* name) {
  struct mudModule* c;
  struct mudModule* m = buildMudModule(name);
  LIST_ITERATOR* i = newListIterator(autoload_list);
  if (!m) { return; }

  ITERATE_LIST(c, i) {
    if (!(strncmp(c->name, name, 1024))) {
      log_string("Module '%s' already in autoload list, not adding", name);
      return;
    }
  } deleteListIterator(i);

  listQueue(autoload_list, m);
  saveAutoloadList();
  log_string("Module '%s' added to autoload list", name);
}

//Remove a module from the autoload list by it's name
__attribute__ ((nonnull(1)))
static void removeAutoloadModule(char* name) {
  struct mudModule* c;
  struct mudModule* m = getMudModule(name, autoload_list);
  if (!m) { return; }
  LIST_ITERATOR* i = newListIterator(autoload_list);

  ITERATE_LIST(c, i) {
    if (!(strncmp(c->name, name, 16))) {
      if (!(listRemove(autoload_list, c))) {
        log_string("Couldn't remove module '%s' from autoload list", name);
        return;
      }
    }
  } deleteListIterator(i);

  saveAutoloadList();
  log_string("Module '%s' removed from autoload list", name);
}

//autoload portion of module command [sorry unix philosophers]
__attribute__ ((nonnull(1,2,3)))
static void doAutoload(struct char_data* ch, char* ald, char *mod) {
  if ((!strlen(ald)) || (!(strncmp(ald, "list", 4)))) {
    listModules(ch, autoload_list, "Modules loaded on boot");
    return;
  }
  else if (!(strncmp(ald, "add", 3))) {
    addAutoloadModule(mod);
  }
  else if (!(strncmp(ald, "remove", 6))) {
    removeAutoloadModule(mod);
  }
}

bool moduleLoaded(char* name) {
  struct mudModule* m = getMudModule(name, module_list);
  if (m) { return TRUE;  }
  return FALSE;
}

bool patchCore(char* sym, void* fp) {
  void* addr = dlsym(NULL, sym);
  if (!addr) {
    return FALSE;
  }

  memcpy(addr, (void**)fp, sizeof(fp));
  return TRUE;
}

//module command
COMMAND(doModule) {
  const char* SYNTAX = "Syntax is: module <list|listall|load|unload|reload> [path]\r\n"
                       "           module autoload [list|add|remove] [path]";
  char *one   = malloc(1024); memset(one,   '\0', 1024);
  char *two   = malloc(1024); memset(two,   '\0', 1024);
  char *three = malloc(1024); memset(three, '\0', 1024);
  three_args(arg, one, two, three);

  if (!(strncmp(one, "listall", 7))) {
    listAllModules(ch);
  }
  else if (!(strncmp(one, "list", 4))) {
    listModules(ch, module_list, "Modules loaded in core");
  }
  else if (!(strncmp(one, "load", 4))) {
    loadModule(two);
  }
  else if (!(strncmp(one, "unload", 6))) {
    unloadModule(two);
  }
  else if (!(strncmp(one, "reload", 6))) {
    reloadModule(two);
  }
  else if (!(strncmp(one, "autoload", 8))) {
    doAutoload(ch, two, three);
  }
  else {
    send_to_char(ch, "%s", SYNTAX);
  }

  free(one);
  free(two);
  free(three);
}

//Initialize module system
void init_modules() {
  module_dir    = malloc(1024);
  autoload_file = malloc(1024);
  
  const char* md = mudsettingGetString("module_dir");
  const char* af = mudsettingGetString("autoload_file");
  
  if (!md) {
    log_string("Error: 'module_dir' is not configured, can't boot module system.");
    return;
  }
  if (!af) {
    log_string("Error: 'autoload_file' is not configured, can't boot module system.");
    return;
  }

  strncpy(module_dir,    md, 1024);
  strncpy(autoload_file, af, 1024);
  
  module_list   = newList();
  autoload_list = newList();
  loadAutoloadList();
  runAutoloadList();
  
  add_cmd("module", NULL, doModule, "admin", FALSE);
}
