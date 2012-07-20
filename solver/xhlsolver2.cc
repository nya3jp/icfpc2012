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


int conf_log = 0;
int conf_separate = 1;
int conf_ID = 1;

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
long long MOD = 1;
class State{
public:
  State(const Point &robot, int W, int H, int target, int target_max, const vector<Point> &rocks, const vector<bool> &earth, int limit)
  {
    long long state0 = robot.x + (robot.y * W) + ((target-1LL) * W * H);
//    state1 = ((robot.x ^ (robot.y << 16) ^ ((long long)target << 32)) << 16) ^ limit;
    state1 = 0; // limit;
    srand(0);
    for( int i = 0; i < (int)rocks.size(); i ++ ){
      srand(rand() ^ rocks[i].x);
      srand(rand() ^ rocks[i].y);
/*
      state1 *= 7541;
      state1 ^= rocks[i].x;
      state1 *= 1753;
      state1 ^= rocks[i].y;
*/
    }
    for( int i = 0; i < (int)earth.size(); i ++ ){
      srand(rand() ^ earth[i]);
/*
      state1 *= 3;
      if( earth[i] )
        state1 ^= 1;
*/
    }
    long long MOD2 = W * H * target_max;
    state1 = rand();
    if( conf_separate && MOD >= MOD2 )
      state1 = state1 / 14987 % (MOD / MOD2) * MOD2 + state0;
    else
      state1 = (state1 / 14987 + state0) % MOD;
  }
  bool operator<(const State &s) const {
    return state1 < s.state1;
  }
private:
  long long state1;
};
#endif

/*
int best_score = 0;
std::string best_history;
*/
set<State> visited;
State getState(const icfpc2012::Simulator &simulator, int target, int target_max, int limit)
{
  vector<Point> rocks;
  const vector<pair<Point, Point> > &rocks_raw = simulator.getRocks();
  for( int i = 0; i < (int)rocks_raw.size(); i ++ )
    rocks.push_back(rocks_raw[i].first);
//  sort(rocks.begin(),rocks.end());

  const vector<bool> &earths = simulator.getEarthState();
/*
  vector<bool> earths2;
  for( int i = 0; i < (int)earths.size(); i ++ )
    if(1)
    earths2.push_back(earths[i]);
*/

  return State(simulator.getRobot(), simulator.GetWidth(), simulator.GetHeight(), target, target_max, rocks, earths, limit );
}
const Movement ms[] = {LEFT, RIGHT, UP, DOWN, WAIT};
const char action_letter[] = "LRUDW";

volatile int to_exit = 0;

static void sig_int(int) { to_exit = 1; /*fprintf( stderr, "SIG_INT\n" );*/ }

pair<int,string> solve(icfpc2012::Simulator &simulator, const vector<Point> &lambdas, const vector<int> &perm, int pos, const string &history, int limit)
{
  if( to_exit )
    return pair<int,string>(0,"");
  if( limit < 0 )
    return pair<int,string>(0,"");
  while( pos+1 < (int)lambdas.size() && simulator.getField(lambdas[pos].x, lambdas[pos].y) != '\\' )
    pos ++;
  if( lambdas[pos] == simulator.getRobot() )
    pos ++;
  if( pos >= (int)lambdas.size() )
    return pair<int,string>(lambdas.size(), history);

//  if( lambdas[pos] == simulator.getRobot() )
//    return solve(simulator, lambdas, perm, pos+1, history);

//  printf( "%s\n%s\n", history.c_str(), simulator.DebugString().c_str() );

// score = max lambdas
  State state1 = getState(simulator, pos+1, (int)lambdas.size(), limit);
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
      pair<int,string> v = solve(t, lambdas, perm, pos, history + this_history, limit - 1);
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

template<class T>
void next_random_permutation( vector<T> &perm, int s, int e )
{
  for( int i = s; i+1 < e; i ++ ){
    int j = rand() % (e - i) + i;
    swap( perm[i], perm[j] );
  }
}

pair<int,string> solvePerm(icfpc2012::Simulator &simulator, int limit)
{
  vector<Point> lambdas = simulator.getLambdas();
  int N = lambdas.size();
  lambdas.push_back(simulator.getLift());
  pair<int,string> best;

  vector<int> perm(N+1, 0);
  for( int i = 0; i < N+1; i ++ )
    perm[i] = i;
  for( int c = 0; to_exit == 0; c ++ ){
     visited.clear();
     pair<int,string> v = solve(simulator, lambdas, perm, 0, "", limit);
     if( best.first < v.first ){
       best = v;
       if( best.first == N+1 ) break;
     }
     else if( best.first == v.first && best.second.size() > v.second.size() )
       best = v;
//     printf( "Perm #%d: %d (%s)\n", c, v.first, v.second.c_str() );
     ++ c;
     if( 0 ){
       if( !next_permutation( perm.begin(), perm.end() - 1 ) )
         break;
     }
     else{
       if( c >= 1000 )
         break;
       next_random_permutation( perm, 0, N );
     }
  }

  return best;
}

pair<int,string> solvePermID(icfpc2012::Simulator &simulator, int limit0)
{
  pair<int,string> nolim = solvePerm(simulator, limit0 - 1);
  if( nolim.first <= 0 ) return nolim;
  for( int L = 0, R = nolim.second.size(); conf_ID && L + 1 < R && to_exit == 0;  ){
    int M = (L + R) / 2;
    if( conf_log )
      fprintf( stderr, "%d", M );
    pair<int,string> v = solvePerm(simulator, M);
    if( v.first > 0 ){
      if( nolim.first < v.first || 
          (nolim.first == v.first && nolim.second.size() > v.second.size()) )
        nolim = v;
      R = min(M, (int)v.second.size()-1);
      if( conf_log )
        fprintf( stderr, " -> %d\n", R );
    }
    else{
      if( conf_log )
        fprintf( stderr, "\n" );
      L = M;
    }
  }
  return nolim;
}

int main(int argc, char* argv[]) {
/*
  ios_base::sync_with_stdio(false);
  if (argc != 2) {
    cerr << "Argc should be 2" << endl;
    exit(1);
  }
*/

  if( argc >= 4 ){
    conf_log      = atoi(argv[1]);
    conf_separate = atoi(argv[2]);
    conf_ID       = atoi(argv[3]);
  }


  if( signal(SIGINT, sig_int) == SIG_ERR){
    printf( "FAIL: signal\n" );
    exit(1);
  }

  icfpc2012::Simulator simulator;
/*
  {
    fstream in(argv[1]);
    in >> simulator;
  }
*/
  cin >> simulator;

  int N = simulator.getLambdas().size();
  MOD = (1 << 10) - 1;
  pair<int,string> best = solvePermID(simulator,1000);
  if( conf_log )
  fprintf( stderr, "%lld -> %d (%s %d)\n", MOD, best.first, best.second.c_str(), (int)best.second.size() );
  for( ; to_exit == 0 && MOD < (1LL<<30);  ){
    MOD = MOD * 2 + 1;
    pair<int,string> v = solvePermID(simulator, best.first == N+1 ? best.second.size() : 1000);
    if( conf_log )
    fprintf( stderr, "%lld -> %d (%s %d)\n", MOD, v.first, v.second.c_str(), (int)v.second.size() );
    if( best.first < v.first )
      best = v;
    else if( best.first == v.first && best.second.size() > v.second.size() )
      best = v;
  }
  cout << best.second << "A" << endl;
  cout.flush();

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
