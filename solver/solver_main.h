#ifndef ICFPC2012_SOLVER_MAIN_H_
#define ICFPC2012_SOLVER_MAIN_H_

namespace icfpc2012 {
class GameStateStack;
}

// API to solvers.
void Yield();
void MaybeUpdateSolution(const icfpc2012::GameStateStack& solution);

// Users should implement this entry function.
void SolverMain(const icfpc2012::GameStateStack& initial_stack);

#endif  // ICFPC2012_SOLVER_MAIN_H_
