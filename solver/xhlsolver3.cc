#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <map>
#include <vector>
#include <string.h>
#include <algorithm>
#include <signal.h>

#include "simulator.h"

using namespace std;
using namespace icfpc2012;


int conf_log = 0;
int conf_separate = 1;
int conf_ID = 1;

long long MOD = 1;
class State{
public:
  void add(int v){
    srand(rand() ^ v);
  }
  State(const icfpc2012::Simulator &simulator, int pos, int limit)
  {
    const Point &robot = simulator.getRobot();
    long long state0 = robot.x + robot.y * simulator.GetWidth();
    srand(pos);
    const vector< pair<Point,Point> > &rocks = simulator.getRocks();
    for( int i = 0; i < (int)rocks.size(); i ++ ){
      add(rocks[i].first.x);
      add(rocks[i].first.y);
    }
    const vector<bool> &earthState = simulator.getEarthState();
    for( int i = 0; i < (int)earthState.size(); i ++ ){
      add(earthState[i]);
    }
    const vector<Point> &lambdas = simulator.getLambdas();
    for( int i = 0; i < (int)lambdas.size(); i ++ ){
      if( simulator.getField(lambdas[i].x, lambdas[i].y) == '\\' ){
        add(lambdas[i].x);
        add(lambdas[i].y);
      }
    }

    long long MOD2 = simulator.GetWidth() * simulator.GetHeight();
    state1 = rand();
    if( conf_separate && MOD >= MOD2 )
      state1 = state1 / 14987 % (MOD / MOD2) * MOD2 + state0;
    else
      state1 = (state1 / 14987 + state0) % MOD;
  }
  bool operator<(const State &s) const {
    return state1 < s.state1;
  }
  int getValue() const { return (int)state1; }
private:
  long long state1;
};

/*
int best_score = 0;
std::string best_history;
*/

#define STATE_MAX (1<<24)
int visited[STATE_MAX];

State getState(const icfpc2012::Simulator &simulator, int pos, int limit)
{

  return State(
    simulator,
    pos,
    limit
  );
}
const Movement ms[] = {LEFT, RIGHT, UP, DOWN, WAIT};
const char action_letter[] = "LRUDW";

volatile int to_exit = 0;

static void sig_int(int) { to_exit = 1; /*fprintf( stderr, "SIG_INT\n" );*/ }

pair<int,string> solve(icfpc2012::Simulator &simulator, const vector<Point> &lambdas, int pos, const string &history, int limit)
{
  while( pos+1 < (int)lambdas.size() && simulator.getField(lambdas[pos].x, lambdas[pos].y) != '\\' )
    pos ++;
  if( to_exit || (int)history.size() >= limit )
    return pair<int,string>(0,"");

// score = max lambdas
  State state1 = getState(simulator, pos, history.size());

  if( visited[state1.getValue()] )
    return pair<int,string>(0,"");
  visited[state1.getValue()] = 1;

  pair<int,string> best(0,"");
  int best_action = 7;
  for( int action = 0; to_exit == 0 && action < 5; action ++ ){
    icfpc2012::Simulator &t = simulator;
    EndState state  = t.Run(ms[action]);
    string this_history = "?";
    this_history[0] =action_letter[action];

    if (state == NOT_GAME_OVER) {
      pair<int,string> v = solve(t, lambdas, pos, history + this_history, limit );
      if( best.first < v.first || (best.first == v.first && best.second.size() > v.second.size()) )
        best = v, best_action = action;
    }
    else if( state == WIN_END ){
      pair<int,string> v(1000000000, history + this_history);
      if( best.first < v.first || (best.first == v.first && best.second.size() > v.second.size()) )
        best = v, best_action = action;
    }
    t.Undo();
  }
/*
  if( best.first == 0 )
    return solve(simulator, pos + 1, history, limit);
  else
*/
    visited[state1.getValue()] = best_action + 1;
    return best;
}

pair<int,string> solveID(icfpc2012::Simulator &simulator, const vector<Point> &lambdas, int limit0)
{
  memset(visited, 0x00, sizeof(visited) );
  pair<int,string> nolim = solve(simulator, lambdas, 0, "", limit0 - 1);
//  fprintf( stderr, "%d visited\n", (int)visited.size() );
  if( nolim.first <= 0 ) return nolim;
  for( int L = 0, R = nolim.second.size(); conf_ID && L + 1 < R && to_exit == 0;  ){
    int M = (L + R) / 2;
    if( conf_log )
      fprintf( stderr, "%d", M );
    memset(visited, 0x00, sizeof(visited) );
    pair<int,string> v = solve(simulator, lambdas, 0, "", M);
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

pair<int,string> solvePermID(icfpc2012::Simulator &simulator, int limit)
{
  vector<Point> lambdas = simulator.getLambdas();
  int N = lambdas.size();

  pair<int,string> best;

  for( int c = 0; to_exit == 0 && c < 10; c ++ ){
     random_shuffle(lambdas.begin(),lambdas.end());
     pair<int,string> v = solveID(simulator, lambdas, limit);
     if( best.first < v.first || 
         (best.first == v.first && best.second.size() > v.second.size()) ){
       best = v;
       if( best.first == N+1 ) break;
     }
  }

  return best;
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
  cin >> simulator;

  int N = simulator.getLambdas().size();
  MOD = (1 << 20) - 1;
  pair<int,string> best = solvePermID(simulator,1000);
  if( conf_log )
  fprintf( stderr, "%lld -> %d (%s %d)\n", MOD, best.first, best.second.c_str(), (int)best.second.size() );
  for( ; to_exit == 0 && MOD < STATE_MAX;  ){
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
