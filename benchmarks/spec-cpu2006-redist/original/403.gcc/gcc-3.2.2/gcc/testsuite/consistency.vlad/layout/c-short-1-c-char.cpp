#include <stdio.h>

class c{
public:
  short f;
};


static class sss: public c{
public:
  char m;
} sss;

#define _offsetof(st,f) ((char *)&((st *) 16)->f - (char *) 16)

int main (void) {
  printf ("++Class with char inhereting class with short:\n");
  printf ("size=%d,align=%d\n", sizeof (sss), __alignof__ (sss));
  printf ("offset-short=%d,offset-char=%d,\nalign-short=%d,align-char=%d\n",
          _offsetof (class sss, f), _offsetof (class sss, m),
          __alignof__ (sss.f), __alignof__ (sss.m));
  return 0;
}
