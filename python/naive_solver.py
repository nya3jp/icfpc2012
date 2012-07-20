#!/usr/bin/python

import heapq
import sys

import boulder


class Solution(object):
  def __init__(self, state_log):
    self._state_log = state_log
    self._simulator = boulder.Simulator()

  @classmethod
  def FromInitialState(cls, initial_state):
    return cls([(None, initial_state)])

  def GetMoveLog(self):
    return ''.join([ch for (ch, _) in self._state_log[1:]])

  def IsFinished(self):
    return self.current_state.end_reason

  def Drive(self, ch):
    assert not self.IsFinished()
    state = self.current_state
    if ch == 'L':
      new_state, _ = self._simulator.Move(state, -1, 0)
    elif ch == 'D':
      new_state, _ = self._simulator.Move(state, 0, -1)
    elif ch == 'U':
      new_state, _ = self._simulator.Move(state, 0, +1)
    elif ch == 'R':
      new_state, _ = self._simulator.Move(state, +1, 0)
    elif ch == 'W':
      new_state, _ = self._simulator.Wait(state)
    elif ch == 'A':
      new_state, _ = self._simulator.Abort(state)
    else:
      raise Exception('Unknown control char: %s' % ch)
    new_state_log = list(self._state_log)
    new_state_log.append((ch, new_state))
    return Solution(new_state_log)

  def __cmp__(self, other):
    self_score = self.current_state.GetScore()
    other_score = other.current_state.GetScore()
    return other_score - self_score

  current_state = property(lambda self: self._state_log[-1][1])
  field_width = property(lambda self: self.current_state.field.width)
  field_height = property(lambda self: self.current_state.field.height)


class NaiveSolver(object):
  def __init__(self, initial_state):
    initial_solution = Solution.FromInitialState(initial_state)
    self._best_solution = initial_solution
    self._queue = []
    self._cache = {}
    self._MaybeVisitSolution(initial_solution)

  def Run(self):
    try:
      while self._queue:
        solution = heapq.heappop(self._queue)
        for ch in 'UDLRWA':
          next_solution = solution.Drive(ch)
          self._MaybeVisitSolution(next_solution)
    except KeyboardInterrupt:
      pass
    print self._best_solution.GetMoveLog()
    print >>sys.stderr, self._best_solution.current_state.GetScore()

  def _MaybeVisitSolution(self, solution):
    if (solution.current_state.GetScore() >
        self._best_solution.current_state.GetScore()):
      self._best_solution = solution
    if not solution.IsFinished():
      cache_key = self._GetCacheKey(solution)
      if cache_key not in self._cache:
        self._cache[cache_key] = solution
        heapq.heappush(self._queue, solution)

  def _GetCacheKey(self, solution):
    field = solution.current_state.field
    rock_set = set()
    lambda_set = set()
    for x in xrange(field.width):
      for y in xrange(field.height):
        if field[x][y] is boulder.ROCK:
          rock_set.add((x, y))
        if field[x][y] is boulder.LAMBDA:
          lambda_set.add((x, y))
    robot = field.LocateRobot()
    tuple_key = (frozenset(rock_set), frozenset(lambda_set), robot)
    return hash(tuple_key)


def main():
  field_string = sys.stdin.read()
  initial_state = boulder.GameState.FromString(field_string)
  solver = NaiveSolver(initial_state)
  solver.Run()


if __name__ == '__main__':
  main()
