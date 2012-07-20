#!/usr/bin/python


class Cell(object):
  def __init__(self, ch):
    self._ch = ch

  def __str__(self):
    return self._ch


ROBOT, WALL, ROCK, LAMBDA, CLOSED_LIFT, OPEN_LIFT, EARTH, EMPTY = CELL_TYPES = map(Cell, r'R#*\LO. ')
CELL_MAP = dict([(str(c), c) for c in CELL_TYPES])

WIN, LOSE, ABORT = 'WIN', 'LOSE', 'ABORT'


class ImmutableOverflowView(object):
  """Wraps a list allowing out-of-bound access."""

  def __init__(self, original_list, default):
    self._original_list = original_list
    self._default = default

  def __getitem__(self, index):
    if 0 <= index and index < len(self._original_list):
      return self._original_list[index]
    return self._default


class Field(object):
  """Immutable matrix representing a field state."""

  def __init__(self, matrix):
    self._matrix = matrix
    self._width = len(matrix)
    self._height = len(matrix[0])
    self._Validate()

  @classmethod
  def FromString(cls, field_string):
    lines = field_string.splitlines()
    height = len(lines)
    width = max([len(line) for line in lines])
    matrix = [[EMPTY for _ in xrange(height)] for _ in xrange(width)]
    for y, line in enumerate(reversed(lines)):
      for x, ch in enumerate(line):
        matrix[x][y] = CELL_MAP[ch]
    return Field(matrix)

  def ToMutableMatrix(self):
    matrix = [[self[x][y] for y in xrange(self.height)] for x in xrange(self.width)]
    return matrix

  def CountCell(self, want_cell):
    cnt = 0
    for x in xrange(self.width):
      for y in xrange(self.height):
        if self[x][y] is want_cell:
          cnt += 1
    return cnt

  def LocateRobot(self):
    for x in xrange(self.width):
      for y in xrange(self.height):
        if self[x][y] is ROBOT:
          return (x, y)
    raise Exception('ROBOT not found')

  def __getitem__(self, x):
    if 0 <= x and x < self.width:
      return ImmutableOverflowView(self._matrix[x], WALL)
    return ImmutableOverflowView([], WALL)

  def __str__(self):
    return '\n'.join([''.join([str(self[x][y]) for x in range(self.width)])
                      for y in reversed(range(self.height))])

  def _Validate(self):
    robots = self.CountCell(ROBOT)
    assert robots == 1, '# of robots should be exactly 1, but actually %d' % robots

  width = property(lambda self: self._width)
  height = property(lambda self: self._height)


class GameState(object):
  """Immutable object that represents a game state."""

  def __init__(self, step, field, end_reason, lambda_collected):
    self._step = step
    self._field = field
    self._end_reason = end_reason
    self._lambda_collected = lambda_collected

  @classmethod
  def FromString(cls, field_string):
    return cls(0, Field.FromString(field_string), None, 0)

  def GetScore(self):
    score = self.lambda_collected * 25
    if self.end_reason == WIN:
      score += self.lambda_collected * 50
    elif self.end_reason == ABORT:
      score += self.lambda_collected * 25 + 1
    return score - self.step

  step = property(lambda self: self._step)
  field = property(lambda self: self._field)
  end_reason = property(lambda self: self._end_reason)
  lambda_collected = property(lambda self: self._lambda_collected)


class Simulator(object):
  """Simulates a single step of move.

  All methods take a state and returns a tuple of (new state, extra messages).
  """

  def Abort(self, state):
    assert not state.end_reason, 'Game already finished'
    messages = []
    state = GameState(state.step + 1, state.field, ABORT, state.lambda_collected)
    return (state, messages)

  def Wait(self, state):
    assert not state.end_reason, 'Game already finished'
    messages = []
    state = GameState(state.step + 1, state.field, state.end_reason, state.lambda_collected)
    state = self._UpdateMap(state, messages)
    return (state, messages)

  def Move(self, state, dx, dy):
    assert not state.end_reason, 'Game already finished'
    messages = []
    state = GameState(state.step + 1, state.field, state.end_reason, state.lambda_collected)
    state = self._Move(state, dx, dy, messages)
    if not state.end_reason:
      state = self._UpdateMap(state, messages)
    return (state, messages)

  def _Move(self, state, dx, dy, messages):
    assert not state.end_reason
    before = state.field
    after = before.ToMutableMatrix()
    end_reason = None
    lambda_collected = state.lambda_collected
    sx, sy = before.LocateRobot()
    tx, ty = sx + dx, sy + dy
    fx, fy = sx + dx*2, sy + dy*2
    side_move = (dy == 0 and dx in (-1, 1))
    next_cell = before[tx][ty]
    far_cell = before[fx][fy]
    if next_cell in (EMPTY, EARTH, LAMBDA, OPEN_LIFT):
      if next_cell is LAMBDA:
        lambda_collected += 1
      if next_cell is OPEN_LIFT:
        end_reason = WIN
      after[sx][sy] = EMPTY
      after[tx][ty] = ROBOT
    elif side_move and next_cell is ROCK and far_cell is EMPTY:
      after[sx][sy] = EMPTY
      after[tx][ty] = ROBOT
      after[fx][fy] = ROCK
    else:
      messages.append('You can\'t move that way.')
    return GameState(state.step, Field(after), end_reason, lambda_collected)

  def _UpdateMap(self, state, messages):
    assert not state.end_reason
    before = state.field
    after = before.ToMutableMatrix()
    end_reason = None
    # Head position
    hx, hy = before.LocateRobot()
    hy += 1
    # No lambda left in the field?
    no_lambda = (before.CountCell(LAMBDA) == 0)
    # Iterate through all cells
    for y in xrange(before.height):
      for x in xrange(before.width):
        if before[x][y] is ROCK and before[x][y-1] is EMPTY:
          after[x][y] = EMPTY
          after[x][y-1] = ROCK
          if (x, y-1) == (hx, hy):
            end_reason = LOSE
        elif (before[x][y] is ROCK and before[x][y-1] is ROCK and
              before[x+1][y] is EMPTY and before[x+1][y-1] is EMPTY):
          after[x][y] = EMPTY
          after[x+1][y-1] = ROCK
          if (x+1, y-1) == (hx, hy):
            end_reason = LOSE
        elif (before[x][y] is ROCK and before[x][y-1] is ROCK and
              before[x-1][y] is EMPTY and before[x-1][y-1] is EMPTY):
          after[x][y] = EMPTY
          after[x-1][y-1] = ROCK
          if (x-1, y-1) == (hx, hy):
            end_reason = LOSE
        elif (before[x][y] is ROCK and before[x][y-1] is LAMBDA and
              before[x+1][y] is EMPTY and before[x+1][y-1] is EMPTY):
          after[x][y] = EMPTY
          after[x+1][y-1] = ROCK
          if (x+1, y-1) == (hx, hy):
            end_reason = LOSE
        elif before[x][y] is CLOSED_LIFT and no_lambda:
          after[x][y] = OPEN_LIFT
        else:
          after[x][y] = before[x][y]
    return GameState(state.step, Field(after), end_reason, state.lambda_collected)


class GameMaster(object):
  """An utility class to keep track of current and all previous states."""

  def __init__(self, initial_state):
    self._state_log = [(None, initial_state)]
    self._simulator = Simulator()

  def GetStateLog(self):
    return list(self._state_log)

  def GetMoveLog(self):
    return ''.join([ch for (ch, _) in self._state_log[1:]])

  def Drive(self, ch):
    state = self.current_state
    if state.end_reason:
      return ['Game finished: %s' % state.end_reason]
    if ch == 'L':
      new_state, messages = self._simulator.Move(state, -1, 0)
    elif ch == 'D':
      new_state, messages = self._simulator.Move(state, 0, -1)
    elif ch == 'U':
      new_state, messages = self._simulator.Move(state, 0, +1)
    elif ch == 'R':
      new_state, messages = self._simulator.Move(state, +1, 0)
    elif ch == 'W':
      new_state, messages = self._simulator.Wait(state)
    elif ch == 'A':
      new_state, messages = self._simulator.Abort(state)
    elif ch == 'Z':
      return self.Revert()
    else:
      raise Exception('Unknown control char: %s' % ch)
    if new_state.end_reason:
      messages.append('Game finished: %s' % new_state.end_reason)
    self._state_log.append((ch, new_state))
    return messages

  def Revert(self):
    if len(self._state_log) <= 1:
      return ['No more step to revert']
    self._state_log.pop()
    return ['Reverted one step']

  current_state = property(lambda self: self._state_log[-1][1])
  field_width = property(lambda self: self.current_state.field.width)
  field_height = property(lambda self: self.current_state.field.height)


