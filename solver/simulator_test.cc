#include <iostream>

#include "simulator.h"

using namespace std;
using namespace icfpc2012;

int main(int argc, char** argv) {
  string move;
  getline(cin, move);

  Simulator simulator;
  cin >> simulator;

  for (int i = 0; i < static_cast<int>(move.length()); ++i) {
    Movement movement;
    switch (move[i]) {
      case 'L':
        movement = LEFT;
        break;
      case 'R':
        movement = RIGHT;
        break;
      case 'U':
        movement = UP;
        break;
      case 'D':
        movement = DOWN;
        break;
      case 'W':
        movement = WAIT;
        break;
      case 'A':
        movement = ABORT;
        break;
      default:
        cout << "Unknown command: " << move[i];
        return 1;
    }
    EndState state = simulator.Run(movement);
    if (state != NOT_GAME_OVER) {
      break;
    }
  }
  cout << simulator.GetScore() << endl;
  cout << simulator.DebugString();

  return 0;
}
