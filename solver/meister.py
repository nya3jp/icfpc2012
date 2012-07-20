#!/usr/bin/python
#
#      #####    ##                                                       
#   ######  /  #### /                                                    
#  /#   /  /   ####/                                    #                
# /    /  /    # #                                     ##                
#     /  /     #                                       ##                
#    ## ##     #      /###   ##   ####      /###     ######## /##        
#    ## ##     #     / ###  / ##    ###  / / ###  / ######## / ###       
#    ## ########    /   ###/  ##     ###/ /   ###/     ##   /   ###      
#    ## ##     #   ##    ##   ##      ## ##    ##      ##  ##    ###     
#    ## ##     ##  ##    ##   ##      ## ##    ##      ##  ########      
#    #  ##     ##  ##    ##   ##      ## ##    ##      ##  #######       
#       /       ## ##    ##   ##      ## ##    ##      ##  ##            
#   /##/        ## ##    /#   ##      ## ##    /#      ##  ####    /     
#  /  #####      ## ####/ ##   #########  ####/ ##     ##   ######/      
# /     ##           ###   ##    #### ###  ###   ##     ##   #####       
# #                                    ###                               
#  ##                           #####   ###                              
#                             /#######  /#                               
#                            /      ###/                                 
#                                                                        
#       ##### /    ##                                                    
#    ######  /  #####                                              #     
#   /#   /  /     #####                                           ###    
#  /    /  ##     # ##                                             #     
#      /  ###     #                                                      
#     ##   ##     #    /###     /###      /###   ### /### /###   ###     
#     ##   ##     #   / ###  / /  ###  / / ###  / ##/ ###/ /##  / ###    
#     ##   ##     #  /   ###/ /    ###/ /   ###/   ##  ###/ ###/   ##    
#     ##   ##     # ##    ## ##     ## ##    ##    ##   ##   ##    ##    
#     ##   ##     # ##    ## ##     ## ##    ##    ##   ##   ##    ##    
#      ##  ##     # ##    ## ##     ## ##    ##    ##   ##   ##    ##    
#       ## #      # ##    ## ##     ## ##    ##    ##   ##   ##    ##    
#        ###      # ##    /# ##     ## ##    /#    ##   ##   ##    ##    
#         #########  ####/ ## ########  ####/ ##   ###  ###  ###   ### / 
#           #### ###  ###   ##  ### ###  ###   ##   ###  ###  ###   ##/  
#                 ###                ###                                 
#     ########     ###         ####   ###                                
#   /############  /#        /######  /#                                 
#  /           ###/         /     ###/                                   


import optparse
import os
import signal
import subprocess
import sys
import threading
import time


# Run in this order.
WOLKENRITTER = [
  ('yuuno', 2),
  ('zafira', 5),
  ('vita', 10),
  ('shamal', 20),
  ('reinforce', 10000000000),
  ]

GRACE_TIME = 3

HOME = os.path.dirname(__file__) or '.'
TMPFILE = os.path.join(HOME, 'output.tmp')


class MeisterHayate(object):
  def __init__(self, debug=False, fixed_time_limit=0, detail_output_dir=None,
               input_name=None):
    self._debug = debug
    self._fixed_time_limit = fixed_time_limit
    self._detail_output_dir = detail_output_dir
    self._input_name = input_name

  def DriveIgnition(self, map_data):
    self._map_data = map_data
    self._interrupted = False
    candidates = [(0, 'A', 'nop')]
    # Try really hard to output something!
    try:
      if self._debug:
        sys.stderr.write('(')
      for name, time_limit in WOLKENRITTER:
        # Already received SIGINT?
        if self._interrupted:
          break
        if self._fixed_time_limit > 0:
          time_limit = self._fixed_time_limit
        if self._debug:
          sys.stderr.write('%s' % name)
        magic = self._RunSolution(name, time_limit)
        if magic is not None:
          if not self._interrupted:
            magic = self._RunCompressor(magic)
          score = self._RunEvaluator(magic)
          if score is not None:
            candidates.append((score, magic, name))
          if self._debug:
            sys.stderr.write(':%d, ' % score)
        else:
          if self._debug:
            sys.stderr.write(':<crash>, ')
        self._MaybeSaveDetail(name, magic, score)
    except:
      if self._debug:
        raise
    candidates.sort(key=lambda c: -c[0])
    if self._debug:
      sys.stderr.write('best:"%s") ' % candidates[0][2])
    print candidates[0][1]

  def _RunSolution(self, name, time_limit):
    try:
      with open(os.devnull, 'w') as null:
        with open(TMPFILE, 'w') as out:
          proc = subprocess.Popen(
              [os.path.join(HOME, name)],
              stdin=subprocess.PIPE, stdout=out, stderr=null)
    except OSError:
      return ''

    threads = [self._MakeSignaler(proc.pid, signal.SIGINT, time_limit),
               self._MakeSignaler(proc.pid, signal.SIGKILL, time_limit + GRACE_TIME),
               #self._MakeOOMKiller(proc.pid),
               ]

    try:
      proc.communicate(self._map_data)
    except KeyboardInterrupt:
      self._interrupted = True
      if self._debug:
        sys.stderr.write('!')
      for t in threads:
        try:
          t.cancel()
        except:
          pass
      threads = [self._MakeSignaler(proc.pid, signal.SIGINT, 0),
                 self._MakeSignaler(proc.pid, signal.SIGKILL, GRACE_TIME),
                 #self._MakeOOMKiller(proc.pid),
                 ]
      proc.wait()
    finally:
      for t in threads:
        try:
          t.cancel()
        except:
          pass

    try:
      with open(TMPFILE) as f:
        return f.read().strip()
    except:
      return ''

  def _RunEvaluator(self, magic):
    with open(os.devnull, 'w') as null:
      with open(TMPFILE, 'w') as out:
        proc = subprocess.Popen(
            [os.path.join(HOME, 'evaluator')],
            stdin=subprocess.PIPE, stdout=out, stderr=null)
    try:
      proc.communicate('%s\n%s' % (magic, self._map_data))
    except KeyboardInterrupt:
      self._interrupted = True
      if self._debug:
        sys.stderr.write('!')
      proc.wait()

    try:
      with open(TMPFILE) as f:
        return int(f.read().strip())
    except:
      return None

  def _RunCompressor(self, magic):
    try:
      with open(os.devnull, 'w') as null:
        with open(TMPFILE, 'w') as out:
          proc = subprocess.Popen(
              [os.path.join(HOME, 'signum')],
              stdin=subprocess.PIPE, stdout=out, stderr=null)
    except OSError:
      if self._debug:
        raise
      return magic

    threads = [self._MakeSignaler(proc.pid, signal.SIGINT, 3),
               self._MakeSignaler(proc.pid, signal.SIGKILL, 5),
               #self._MakeOOMKiller(proc.pid),
               ]

    try:
      proc.communicate('%s\n%s' % (magic, self._map_data))
    except KeyboardInterrupt:
      self._interrupted = True
      if self._debug:
        sys.stderr.write('!')
      # Received SIGINT during compression...
      return magic
    finally:
      try:
        proc.kill()
      except:
        pass
      try:
        proc.wait()
      except:
        pass
      for t in threads:
        try:
          t.cancel()
        except:
          pass

    try:
      with open(TMPFILE) as f:
        return f.read().strip() or magic
    except:
      return magic

  def _MakeSignaler(self, pid, sig, time_limit):
    t = threading.Timer(time_limit, lambda: Kill(pid, sig))
    t.start()
    return t

  def _MakeOOMKiller(self, pid):
    t = OOMKiller(pid, debug=self._debug)
    t.start()
    return t

  def _MaybeSaveDetail(self, name, magic, score):
    if self._detail_output_dir and self._input_name:
      detail_path = os.path.join(
          self._detail_output_dir, '%s.%s.txt' % (self._input_name, name))
      try:
        with open(detail_path, 'w') as f:
          print >>f, score
          print >>f, magic
      except:
        if self._debug:
          raise


class OOMKiller(threading.Thread):
  def __init__(self, pid, debug=False):
    super(OOMKiller, self).__init__()
    self._pid = pid
    self._debug = debug
    self._running = False

  def run(self):
    self._running = True
    self._sigint_sent = False
    self._sigkill_sent = False
    try:
      while self._running:
        time.sleep(1)
        meminfo = {}
        with open('/proc/meminfo') as f:
          for line in f:
            cols = line.split()
            if cols[0].endswith(':'):
              try:
                meminfo[cols[0][:-1]] = int(cols[1])
              except ValueError:
                pass
        if not ('MemTotal' in meminfo and
                'MemFree' in meminfo and
                'Buffers' in meminfo and
                'Cached' in meminfo):
          # Not supported
          if self._debug:
            sys.stderr.write('[Malformed /proc/meminfo]')
          break
        mem_usage = (
          100.0
          * (meminfo['MemTotal'] -
             (meminfo['MemFree'] + meminfo['Buffers'] + meminfo['Cached']))
          / meminfo['MemTotal'])
        if mem_usage >= 90 and not self._sigint_sent:
          if self._debug:
            sys.stderr.write('[SIGINT sent]')
          Kill(self._pid, signal.SIGINT)
          self._sigint_sent = True
        if mem_usage >= 95 and not self._sigkill_sent:
          if self._debug:
            sys.stderr.write('[SIGKILL sent]')
          Kill(self._pid, signal.SIGKILL)
          self._sigkill_sent = True
    except:
      if self._debug:
        raise

  def cancel(self):
    self._running = False


def Kill(pid, sig):
  try:
    os.kill(pid, sig)
  except:
    pass


def main():
  p = optparse.OptionParser()
  p.add_option('-d', '--debug', action='store_true', dest='debug', default=False)
  p.add_option('-f', '--fixed-time-limit', dest='fixed_time_limit', type='int', default=0)
  p.add_option('--detail-output-dir', dest='detail_output_dir')
  p.add_option('--input-name', dest='input_name')
  opts, args = p.parse_args(sys.argv[1:])
  map_data = sys.stdin.read()
  hayate = MeisterHayate(
      debug=opts.debug,
      fixed_time_limit=opts.fixed_time_limit,
      detail_output_dir=opts.detail_output_dir,
      input_name=opts.input_name)
  hayate.DriveIgnition(map_data)


if __name__ == '__main__':
  main()
