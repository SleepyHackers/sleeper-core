#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cencode.h"
#include "cdecode.h"

#define b64max(i) ((4*(i/3))+4)

char* strip(char* input, size_t sz) {
  char* clean = malloc(sz);
  memset(clean, 0, sz);
  int i = 0;
  while (*input) {
    if (*input == '\n') { input++; }
    if (!(strncmp(input, "�", 3))) { input += 3; }
    *clean = *input;
    clean++; input++; i++;
  }
  
  input -= i;
  return (clean-i);
}

//Libb64 interface functions
void b64encode(char* input, char* output, size_t sz) {
  base64_encodestate s;
  char* clean;
  char* c = output;
  int cnt = 0;

  base64_init_encodestate(&s);
  cnt = base64_encode_block(input, strlen(input), c, &s);
  c += cnt;
  cnt = base64_encode_blockend(c, &s);
  c += cnt;
  *c = '\0';
  clean = strip(output, b64max(sz));
  snprintf(output, sz, "%s", clean);

  #if DO_FREE
  free(clean);
  #endif
}
void b64decode(char* input, char* output, size_t sz) {
  char* c = output;
  int cnt = 0;
  base64_decodestate s;
  base64_init_decodestate(&s);
  cnt = base64_decode_block(input, sz, c, &s);
  c += cnt;
  *c = 0;
}
//End libb64 interface functions
