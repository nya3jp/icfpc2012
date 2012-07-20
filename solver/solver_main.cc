#include "solver_main.h"

#include <signal.h>
#include <stdlib.h>
#include <iostream>

#include "boulder.h"

using namespace std;
using namespace icfpc2012;

static volatile bool g_interrupted = false;
static GameStateStack best_solution;

static void OnSIGINT(int) {
  g_interrupted = true;
  signal(SIGINT, SIG_DFL);
}

static void PrintBestSolutionAndExit() {
  cout << best_solution.GetCommand() << endl;
  exit(0);
}

void Yield() {
  if (g_interrupted) {
    PrintBestSolutionAndExit();
  }
}

void MaybeUpdateSolution(const GameStateStack& solution) {
  if (solution.current_state().score() > best_solution.current_state().score()) {
    best_solution = solution;
  }
}

int main() {
  ios_base::sync_with_stdio(false);
  signal(SIGINT, OnSIGINT);

  GameState initial_state;
  cin >> initial_state;
  GameStateStack initial_stack(initial_state);
  best_solution = initial_stack;
  SolverMain(initial_stack);
  Yield();
  PrintBestSolutionAndExit();

  return 0;
}
