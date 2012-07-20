#!/usr/bin/env python

import argparse
import sys
import random

parser = argparse.ArgumentParser(description = "map random generator")

parser.add_argument('width', metavar='W', type=int,
                    help='width')
parser.add_argument('height', metavar='H', type=int,
                    help='height')
parser.add_argument('--outer-wall', dest='outer_wall', action='store_true',
                    default=False,
                    help='whether the map is walled off')
parser.add_argument('--outer-lift', dest='outer_lift', action='store_true',
                    default=False,
                    help='whether the lifter is set to the outer layer')
parser.add_argument('--earth', dest='earth', action='store', type=float,
                    default=0.8,
                    help='Ratio of earth')
parser.add_argument('--lambda', dest='lam', action='store', type=float,
                    default=0.1,
                    help='Ratio of lambdas')
parser.add_argument('--rock', dest='rock', action='store', type=float,
                    default=0.1,
                    help='Ratio of rocks')
parser.add_argument('--wall', dest='wall', action='store', type=float,
                    default=0.05,
                    help='Ratio of extra (inner) walls')
parser.add_argument('--flood', dest='flood', action='store_true',
                    default=False,
                    help='Enable flooding')

args = parser.parse_args()

WALL='#'
EARTH='.'
EMPTY=' '
ROCK='*'
LIFTER='L'
LAMBDA='\\'
ROBOT='R'

map = [[EMPTY] * args.height for _ in range(args.width)]

def rand():
    x = random.randint(0, args.width - 1)
    y = random.randint(0, args.height - 1)
    return(x, y)

ntotal = args.height * args.width

for i in range(int(ntotal * args.earth)):
    (x, y) = rand()
    map[x][y] = EARTH
for i in range(int(ntotal * args.wall)):
    (x, y) = rand()
    map[x][y] = WALL
for i in range(int(ntotal * args.rock)):
    (x, y) = rand()
    map[x][y] = ROCK
for i in range(int(ntotal * args.lam)):
    (x, y) = rand()
    map[x][y] = LAMBDA

if args.outer_wall:
    for i in range(0, args.width):
        map[i][0] = WALL
        map[i][args.height - 1] = WALL
    for i in range(0, args.height):
        map[0][i] = WALL
        map[args.width - 1][i] = WALL

if args.outer_lift:
    which = random.randint(0, 1)
    widthcut = args.width - 2
    heightcut = args.height - 2
    where = random.randint(0, widthcut + heightcut - 1)
    if which == 0:
        if where < widthcut:
            map[where + 1][0] = LIFTER
        else:
            map[0][where - widthcut + 1] = LIFTER
    else:
        if where < widthcut:
            map[where + 1][args.width - 1] = LIFTER
        else:
            map[args.height - 1][where - widthcut + 1] = LIFTER
else:
    (x, y) = rand()
    map[x][y] = LIFTER

(x, y) = rand()
map[x][y] = ROBOT

for y in range(args.height):
    for x in range(args.width):
        sys.stdout.write(map[x][y])
    sys.stdout.write('\n')









