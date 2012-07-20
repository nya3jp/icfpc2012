#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <signal.h>

#include "simulator.h"

using namespace std;
using namespace icfpc2012;

#define XHL_TEMP

#ifndef XHL_TEMP
class State{
public:
  State(const Point &robot, int target, const vector<Point> &rocks, const vector<bool> &earth) :
    robot_(robot),
    target_(target),
    rocks_(rocks)
//    ,earth_(earth)
  {}
  bool operator<(const State &s) const {
    if( robot_  != s.robot_  ) return robot_  < s.robot_;
    if( target_ != s.target_ ) return target_ < s.target_;
    //if( rocks_  != s.rocks_  )
    return rocks_  < s.rocks_;
//    return earth_ < s.earth_;
  }
  void dump() const {
    printf( "%2d %2d %d ", robot_.x, robot_.y, target_ );
    for( int i = 0; i < (int)rocks_.size(); i ++ ){
      printf( "<%2d %2d> ", rocks_[i].x, rocks_[i].y );
    }
    printf ("\n" );
  }
  string DebugString() const {
    char buf[1024];
    int o = 0;
    o += sprintf( buf + o, "%2d %2d %d ", robot_.x, robot_.y, target_ );
    for( int i = 0; i < (int)rocks_.size(); i ++ ){
      o += sprintf( buf + o, "<%2d %2d> ", rocks_[i].x, rocks_[i].y );
    }
    o += sprintf (buf + o, "\n" );
    return buf;
  }
private:
  Point         robot_;
  int           target_;
  vector<Point> rocks_;
//  vector<bool>  earth_;
};
#else
class State{
public:
  State(const Point &robot, int target, const vector<Point> &rocks, const vector<bool> &earth)
  {
    state1 = robot.x;
    state2 = robot.y | ((long long)target << 32);
    for( int i = 0; i < (int)rocks.size(); i ++ ){
      state1 *= 7;
      state1 ^= rocks[i].x;
      state2 *= 7;
      state2 ^= rocks[i].y;
    }
    for( int i = 0; i < (int)earth.size(); i ++ ){
      state1 *= 2;
      if( earth[i] )
        state1 ^= 1;
    }
  }
  bool operator<(const State &s) const {
    if( state1 != s.state1 ) return state1 < s.state1;
    return state2 < s.state2;
  }
private:
  long long state1, state2;
};
#endif

/*
int best_score = 0;
std::string best_history;
*/
set<State> visited;
State getState(const icfpc2012::Simulator &simulator, int target)
{
  vector<Point> rocks;
  const vector<pair<Point, Point> > &rocks_raw = simulator.getRocks();
  for( int i = 0; i < (int)rocks_raw.size(); i ++ )
    rocks.push_back(rocks_raw[i].first);
//  sort(rocks.begin(),rocks.end());

  const vector<bool> &earths = simulator.getEarthState();
  vector<bool> earths2;
  for( int i = 0; i < (int)earths.size(); i ++ )
    if(0)
    earths2.push_back(earths[i]);

  return State(simulator.getRobot(), target, rocks, earths2 );
}
const Movement ms[] = {LEFT, RIGHT, UP, DOWN, WAIT};
const char action_letter[] = "LRUDW";

volatile int to_exit = 0;

static void sig_int(int) { to_exit = 1; /*fprintf( stderr, "SIG_INT\n" );*/ }

pair<int,string> solve(icfpc2012::Simulator &simulator, const vector<Point> &lambdas, const vector<int> &perm, int pos, const string &history)
{
  if( to_exit )
    return pair<int,string>(0,"");
  while( pos < (int)lambdas.size() && simulator.getField(lambdas[pos].x, lambdas[pos].y) != '\\' )
    pos ++;
  if( pos >= (int)lambdas.size() )
    return pair<int,string>(lambdas.size(), history);

//  if( lambdas[pos] == simulator.getRobot() )
//    return solve(simulator, lambdas, perm, pos+1, history);

//  printf( "%s\n%s\n", history.c_str(), simulator.DebugString().c_str() );

// score = max lambdas
  State state1 = getState(simulator, pos+1);
  if( visited.count(state1) )
    return pair<int,string>(0,"");
  visited.insert(state1);

/*
  if( (visited.size() & ((1<<16)-1)) == 0 )
    printf( "%d visited\n", visited.size() );
*/

  string max_history;
  int max_score = 0;
  for( int action = 0; to_exit == 0 && action < 5; action ++ ){
//    icfpc2012::Simulator &t2 = simulator;
//    icfpc2012::Simulator t_old = simulator;
    icfpc2012::Simulator &t = simulator;
    EndState state  = t.Run(ms[action]);
/*
    string this_history = "?\n" + t.DebugString() + "\n" + state1.DebugString() + "\n";
    this_history[0] =action_letter[action];
*/
    string this_history = "?";
    this_history[0] =action_letter[action];

    if (state == NOT_GAME_OVER) {
      pair<int,string> v = solve(t, lambdas, perm, pos, history + this_history);
      if( max_score < v.first ){
        max_score   = v.first;
        max_history = v.second;
      }
    }
    else if( state == WIN_END ){
      if( max_score < pos + 1 ){
        max_score   = pos + 1;
        max_history = (history + this_history);
      }
    }
    t.Undo();
/*
    if( t_old.DebugString() != simulator.DebugString() ){
      printf( "ERROR:\nEXPECTED:%s\nRECEIVED:%s\n",
        simulator.DebugString().c_str(),
        t_old.DebugString().c_str(),
        (history + action_letter[action]).c_str()
      );
    }
*/
  }
  return pair<int,string>(max_score, max_history);
}

pair<int,string> solvePerm(icfpc2012::Simulator &simulator)
{
  vector<Point> lambdas = simulator.getLambdas();
  int N = lambdas.size();
  lambdas.push_back(simulator.getLift());
  vector<int> perm(N+1, 0);
  for( int i = 0; i < N+1; i ++ ){
    perm[i] = i;
  }
  visited.clear();
  string max_history;
  int max_score = 0;
  int c = 0;
  do{
     pair<int,string> v = solve(simulator, lambdas, perm, 0, "");
     if( max_score < v.first ){
       max_score   = v.first;
       max_history = v.second;
       if( max_score == N+1 ) break;
     }
//     printf( "Perm #%d: %d (%s)\n", c, v.first, v.second.c_str() );
     ++ c;
  } while( to_exit == 0 && next_permutation( perm.begin(), perm.end() - 1 ) );
  
  return pair<int,string>(max_score, max_history);
}

int main(int argc, char* argv[]) {
/*
  ios_base::sync_with_stdio(false);
  if (argc != 2) {
    cerr << "Argc should be 2" << endl;
    exit(1);
  }
*/

  signal(SIGINT, sig_int);

  icfpc2012::Simulator simulator;
/*
  {
    fstream in(argv[1]);
    in >> simulator;
  }
*/
  cin >> simulator;

  pair<int,string> best = solvePerm(simulator);
  cout << best.second << "A" << endl;

/*
  for( set<State>::const_iterator it = visited.begin(), ite = visited.end(); it != ite; ++ it ){
    it->dump();
  }
*/

//  printf( "Base score = %d (%s)\n", best.first, best.second.c_str() );
/*
  while(1){
   
    pair<int,string> v = solvePerm(simulator);
    printf( "Best score = %d (%s)\n", v.first, v.second.c_str() );
  }
*/
  return 0;
}
