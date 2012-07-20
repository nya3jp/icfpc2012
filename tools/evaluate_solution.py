#!/usr/bin/python

import optparse
import os
import signal
import subprocess
import sys
import threading


def main():
  p = optparse.OptionParser()
  p.add_option('-t', '--time-limit', dest='time_limit', metavar='<secs>', default=150, type='int')
  p.add_option('-g', '--grace-period', dest='grace_period', metavar='<secs>', default=10, type='int')
  p.add_option('-p', '--program', dest='program', metavar='<filename>')
  p.add_option('-d', '--debug', dest='debug', action='store_true')
  p.add_option('-a', '--addargs', dest='addargs', metavar='<args>', default='')
  p.add_option('-H', '--high-score-file', dest='high_score_file', metavar='<file>', default='')
  opts, args = p.parse_args(sys.argv[1:])

  if not opts.program:
    print 'Please specify your solver with -p'
    sys.exit(1)

  highscores = {}
  if opts.high_score_file:
    with open(opts.high_score_file) as f:
      for line in f:
        name, score = line.strip().split(': ')
        highscores[name] = int(score)

  total_score = 0

  for map_file in args:
    sys.stdout.write('%s: ' % map_file)
    sys.stdout.flush()

    with open(map_file) as f:
      map_data = f.read()

    map_name = os.path.splitext(os.path.basename(map_file))[0]
    highscore = highscores.get(map_name, 0)

    with open(os.devnull, 'w') as null:
      args = [opts.program]
      args.extend(opts.addargs.replace('%mapname%', map_name).split())
      proc = subprocess.Popen(
          args,
          stdin=subprocess.PIPE,
          stdout=subprocess.PIPE,
          stderr=(None if opts.debug else null))

    timers = []
    def Interrupt():
      try:
        os.kill(proc.pid, signal.SIGINT)
      except:
        pass
    t = threading.Timer(opts.time_limit, Interrupt)
    t.start()
    timers.append(t)
    def Kill():
      try:
        os.kill(proc.pid, signal.SIGKILL)
      except:
        pass
    t = threading.Timer(opts.time_limit + opts.grace_period, Kill)
    t.start()
    timers.append(t)

    try:
      move, _ = proc.communicate(map_data)
      move = move.strip()
    except KeyboardInterrupt:
      Kill()
      print '[Received SIGINT]'
      break
    finally:
      for t in timers:
        t.cancel()
      del timers

    proc = subprocess.Popen([os.path.join(os.path.dirname(__file__), os.pardir, 'solver', 'evaluator')],
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    score, _ = proc.communicate('%s\n%s' % (move, map_data))
    score = int(score.strip())

    print score, '/', highscore if highscore else '???'
    total_score += score

  print 'TOTAL: %d' % total_score


if __name__ == '__main__':
  main()
