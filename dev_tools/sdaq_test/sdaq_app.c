#include "sdaq_drv.h"

#define MAX(a, b) ((a)>(b)? (a) : (b))
int main() {
  int d = 1, c = 2, e = MAX(d,c);
  sdaq_can_identifier id={0};
  printf("e = %d\n", e);
  printf("sizeof(id)=%ld\n",sizeof(id));
  return 0;
}
