#include <string.h>
#include <iostream>
#include <queue>
#include <set>

#include "boulder.h"
#include "solver_main.h"

using namespace std;
using namespace icfpc2012;

namespace std {
bool operator<(const GameStateStack& a, const GameStateStack& b) {
  return (a.current_state().score() > b.current_state().score());
}
}

void SolverMain(const GameStateStack& initial_stack) {
  GameStateStack current_stack(initial_stack);
  while (current_stack.current_state().end_state() == NOT_GAME_OVER) {
    Yield();

    priority_queue<GameStateStack> q;
    set<Point> seen;
    q.push(current_stack);
    seen.insert(current_stack.current_state().field().robot().position());

    bool improved = false;
    while (!q.empty()) {
      Yield();
      GameStateStack stack = q.top();
      q.pop();
      if (stack.current_state().lambda_collected() >
          current_stack.current_state().lambda_collected()) {
        current_stack = stack;
        improved = true;
        break;
      }
      for (int index_move = 0; index_move < NUM_MOVEMENTS; ++index_move) {
        Movement move = MOVEMENTS[index_move];
        GameStateStack next_stack = Simulate(stack, move);
        MaybeUpdateSolution(next_stack);
        if (next_stack.current_state().end_state() == NOT_GAME_OVER &&
            seen.insert(next_stack.current_state().field().robot().position())
                .second) {
          q.push(next_stack);
        }
      }
    }

    if (!improved) {
      break;
    }
  }
}
