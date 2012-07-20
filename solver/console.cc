#include <cstdlib>
#include <fstream>
#include <iostream>

#include "simulator.h"

using namespace std;
using namespace icfpc2012;

int main(int argc, char* argv[]) {
  ios_base::sync_with_stdio(false);

  if (argc != 3) {
    cerr << "Argc should be 3" << endl;
    exit(1);
  }

  icfpc2012::Simulator simulator;
  {
    fstream in(argv[1]);
    in >> simulator;
  }

  for (int i = 0; argv[2][i]; ++i) {
    EndState state;
    if (argv[2][i] == 'Z') {
      simulator.Undo();
      state = NOT_GAME_OVER;
    } else {
      Movement movement;
      switch (argv[2][i]) {
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
	  cerr << "Unknown command: " << argv[2][i];
	  exit(1);
      }
      state = simulator.Run(movement);
    }
    cout << simulator.GetHistoryLength() << endl;
    cerr << simulator.DebugColoredString() << endl;
    switch (state) {
      case NOT_GAME_OVER: cout << "NOT_GAME_OVER" << endl; break;
      case WIN_END: cout << "WIN_END" << endl; break;
      case LOOSE_END: cout << "LOOSE_END" << endl; break;
      case ABORT_END: cout << "ABORT_END" << endl; break;
      default:
	break;
    }
    cout << "initial_water: " << simulator.initial_water() << endl;
    cout << "flooding: " << simulator.flooding() << endl;
    cout << "waterproof: " << simulator.waterproof() << endl;
    cout << "current_water_count: " << simulator.current_water_count() << endl;
    cout << "score: " << simulator.GetScore() << endl;
    if (state != NOT_GAME_OVER) {
      break;
    }
  }

  return 0;
}
