#include <fstream>
#include <iostream>

#include "boulder.h"

using namespace std;
using namespace icfpc2012;

int main(int argc, char** argv) {
  ios_base::sync_with_stdio(false);

  if (argc != 3) {
    cerr << "Argc should be 3" << endl;
    exit(1);
  }

  icfpc2012::GameState initial_state;
  {
    fstream in(argv[1]);
    in >> initial_state;
  }

  vector<GameState> state_log;
  state_log.push_back(initial_state);
  for (int i = 0; argv[2][i]; ++i) {
    char ch = argv[2][i];
    if (ch == 'Z') {
      if (state_log.size() >= 2) {
        state_log.pop_back();
      }
    } else {
      Movement move = MovementFromChar(ch);
      state_log.push_back(Simulate(state_log.back(), move));
    }
    if (state_log.back().end_state() != NOT_GAME_OVER) {
      break;
    }
  }

  const GameState& final_state = state_log.back();
  cout << final_state.DebugString() << endl;

  return 0;
}
