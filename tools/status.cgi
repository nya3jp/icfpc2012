#!/usr/bin/python

import json
import re
import StringIO
import urllib
import zipfile

def main():
  u = urllib.urlopen("http://reinforce.p.nya3.jp:8080/api/json")
  data = json.loads(u.read())
  u.close()
  color='unknown'
  for job in data['jobs']:
    if job['name'] == 'LyricalTest':
      color = job['color']
      break
  if color in ('blue', 'blue_anime'):
    color = 'blue'
    msg = 'Passing'
  else:
    color = 'red'
    msg = 'FAILING'

  print "Content-Type: text/html"
  print
  print "<!doctype html>"
  print "<title>Tome of the night SKI</title>"
  print "<meta http-equiv=refresh content=60>"
  print "<body style='margin: 0; padding: 0;'>"

  def ScoreBoard(rev, st):
    u = urllib.urlopen('http://reinforce.p.nya3.jp:8080/job/LyricalEvaluation/%s/artifact/*zip*/archive.zip' % rev)
    z = zipfile.ZipFile(StringIO.StringIO(u.read()))
    u.close()
  
    maps = set()
    solutions = set()
    for name in z.namelist():
      m = re.search(r'^archive/maps/([^.]*)\.([^.]*)\.txt$', name)
      if m:
        maps.add(m.group(1))
        solutions.add(m.group(2))
    maps = sorted(list(maps))
    solutions = sorted(list(solutions))
  
    highscores = {}
    with z.open('archive/maps/HIGHSCORES') as f:
      for line in f:
        name, score = line.strip().split(': ')
        highscores[name] = int(score)
  
    maps.sort(key=lambda m: (m not in highscores, m))
  
    print '<div style="position:absolute;%s;top:0;background:white;padding:20px">' % st
    print '<table style="font-size: 10px;text-align:center">'
    print '<tr><td></td>'
    for s in solutions:
      print '<td><img src="/%s.gif" style="width:60px;height:60px"></td>' % s
    print '<td><img src="/nanoha.gif" style="width:60px;height:60px"></td>'
    print '</tr>'
    totals=[0 for s in solutions]
    for m in maps:
      if m in highscores:
        print '<tr><td><b>%s</b></td>' % m
      else:
        print '<tr><td>(%s)</td>' % m
      scores = []
      gscores = []
      for i, s in enumerate(solutions):
        with z.open('archive/maps/%s.%s.txt' % (m, s)) as f:
          score = int(f.read().split()[0])
          scores.append(score)
          if s != 'yuuno':
            gscores.append(score)
          if m in highscores:
            totals[i] += score
      best = max(gscores)
      for s, score in zip(solutions, scores):
        if score == best:
          print '<td style="background: yellow"><b>%d</b></td>' % score
        else:
          print '<td>%d</td>' % score
      if m in highscores:
        print '<td><b>%d</b></td>' % highscores[m]
      else:
        print '<td>-</td>'
      print '</tr>'
    print '<tr><td>Total</td>'
    for score in totals:
      print '<td><b>%d</b></td>' % score
    print '<td><b>%d</b></td>' % sum(highscores.values())
    print '</tr>'
    print '</table>'
    print '</div>'

  ScoreBoard('lastSuccessfulBuild', 'left:0')
  ScoreBoard('12', 'right:0')

  print "<div style='text-align: center; position: absolute; left: 500px; top: 20px; z-index:-1;'><span style='font-size: 40px; line-height: 2.0'>%s</span><br><img src='/%s.jpg'></div>" % (msg, color)


if __name__ == '__main__':
  main()
