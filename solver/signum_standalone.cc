#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <set>
#include <map>
#include <vector>
#include <string.h>
#include <algorithm>
#include <sstream>
#include <signal.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>

#include "boulder.h"
#include "signum.h"
// #include "solver_main.h"

using namespace std;
using namespace icfpc2012;

static volatile bool g_interrupted = false;

static void OnSIGINT(int) {
  g_interrupted = true;
  signal(SIGINT, SIG_DFL);
}

bool Yield2() {
  return (g_interrupted);
}


int main(int argc, char *argv[]) {
  ios_base::sync_with_stdio(false);
  signal(SIGINT, OnSIGINT);

  string ans;
  getline(cin, ans);

  GameState initial_state;
  cin >> initial_state;
  GameStateStack initial_stack(initial_state);

  pair<int, string> v = shorten(initial_stack, ans, Yield2);
  if( v.first >= 0 )
    cout << v.second << endl;
  else
    cout << ans << endl;

  return 0;
}
