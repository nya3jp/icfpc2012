#include <iostream>

#include "boulder.h"

using namespace std;
using namespace icfpc2012;

int main(int argc, char** argv) {
  ios_base::sync_with_stdio(false);

  string move;
  getline(cin, move);
  icfpc2012::GameState state;
  cin >> state;

  for (int i = 0; i < static_cast<int>(move.size()); ++i) {
    state = Simulate(state, MovementFromChar(move[i]));
  }

  cout << state.score() << endl;

  return 0;
}
