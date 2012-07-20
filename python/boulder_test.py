#!/usr/bin/python

import os
import sys

import boulder

def RunBoulderTest(test_file):
  with open(test_file) as f:
    lines = f.read().splitlines()
  move = lines[0]
  field_string = '\n'.join(lines[1:])
  initial_state = boulder.GameState.FromString(field_string)
  master = boulder.GameMaster(initial_state)
  for ch in move:
    master.Drive(ch)
  return '%d\n%s\n' % (master.current_state.GetScore(), master.current_state.field)


def main():
  generate_mode = '--generate_golden' in sys.argv
  TESTS_DIR = os.path.join(os.path.dirname(__file__), os.pardir, 'tests', 'testdata')
  success = True
  for name in os.listdir(TESTS_DIR):
    if not name.endswith('.test'):
      continue
    prefix = os.path.splitext(name)[0]
    test_file = os.path.join(TESTS_DIR, name)
    golden_file = os.path.join(TESTS_DIR, '%s.golden' % prefix)
    test_data = RunBoulderTest(test_file)
    if generate_mode:
      with open(golden_file, 'w') as f:
        f.write(test_data)
      print 'Written %s.golden' % prefix
    else:
      with open(golden_file) as f:
        golden_data = f.read()
      if test_data != golden_data:
        print '!!NG!!\t%s' % prefix
        success = False
      else:
        print 'OK\t%s' % prefix
  if not success:
    sys.exit(1)
  sys.exit(0)


if __name__ == '__main__':
  main()
