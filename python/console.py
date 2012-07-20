#!/usr/bin/python

import curses
import sys

import boulder


class InteractiveGameRunner(object):
  """Runs a game interactively."""

  KEY_MAP = {
    ord('h'): 'L', ord('j'): 'D', ord('k'): 'U', ord('l'): 'R',
    ord('.'): 'W', ord('q'): 'A',
    curses.KEY_LEFT: 'L', curses.KEY_DOWN: 'D',
    curses.KEY_UP: 'U', curses.KEY_RIGHT: 'R',
    }

  def __init__(self, initial_state):
    self._master = boulder.GameMaster(initial_state)
    self._messages = []

  def Run(self):
    curses.wrapper(self._InternalRun)
    print self._master.current_state.end_reason
    print self._master.GetMoveLog()
    print self._master.current_state.GetScore()

  def _InternalRun(self, screen):
    curses.use_default_colors()
    self._screen = screen
    self._running = True
    while True:
      self._screen.erase()
      self._Draw()
      if not self._running:
        break
      key = self._screen.getch()
      self._OnKey(key)

  def _OnKey(self, key):
    ch = self.KEY_MAP.get(key)
    if ch and not (ch == 'A' and self._master.current_state.end_reason):
      messages = self._master.Drive(ch)
      self._messages.extend(messages)
    elif key == ord('u'):
      messages = self._master.Revert()
      self._messages.extend(messages)
    if ch == 'A':
      self._running = False

  def _Draw(self):
    field_string = str(self._master.current_state.field)
    for i, line in enumerate(field_string.splitlines()):
      self._DrawString(i, 0, line)
    self._DrawString(self._master.field_height+1, 0,
                     'Step %d' % self._master.current_state.step)
    for i, message in enumerate(self._messages):
      self._DrawString(self._master.field_height+2+i, 0, message)
    self._messages = []
    self._screen.refresh()

  def _DrawString(self, y, x, s):
    try:
      self._screen.addstr(y, x, s)
    except curses.error:
      pass


class BatchGameRunner(object):
  """Runs a game in batch."""

  def __init__(self, initial_state):
    self._master = boulder.GameMaster(initial_state)

  def Run(self, input_stream):
    for ch in input_stream.read().strip():
      if self._master.current_state.end_reason:
        break
      self._master.Drive(ch)
    print self._master.current_state.GetScore()


def main():
  filename = sys.argv[1]
  field_string = open(filename).read()
  initial_state = boulder.GameState.FromString(field_string)
  if sys.stdin.isatty():
    InteractiveGameRunner(initial_state).Run()
  else:
    BatchGameRunner(initial_state).Run(sys.stdin)


if __name__ == '__main__':
  main()
