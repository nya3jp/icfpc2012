#include "boulder.h"

#include <stdio.h>
#include "assert.h"

using icfpc2012::CowArray;

void TestCowArray() {
  for (int l = 0; l < 10; ++l) {
    CowArray<int> v(l, 28);
    for (int i = 0; i < l; ++i) {
      assert(v.Get(i) == 28);
    }

    for (int i = 0; i < l; ++i) {
      v.GetMutable(i) = i;
    }
    for (int i = 0; i < l; ++i) {
      assert(v.Get(i) == i);
    }

    for (int i = 0; i < l; ++i) {
      CowArray<int> u(v);
      u.GetMutable(i) = 28;
      for (int j = 0; j < l; ++j) {
        assert(v.Get(j) == j);
      }
      for (int j = 0; j < l; ++j) {
        assert(u.Get(j) == (j == i ? 28 : j));
      }
    }
  }
}

int main(int argc, char** argv) {
  TestCowArray();
  puts("OK!");
  return 0;
}
