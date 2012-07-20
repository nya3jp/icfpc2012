#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <map>
#include <vector>
#include <string.h>
#include <algorithm>
#include <sstream>
#include <signal.h>

#include "simulator.h"

using namespace std;
using namespace icfpc2012;


int conf_log = 0;
int conf_separate = 1;
int conf_ID = 1;

//#define NEW_HASH

int S[256];
#ifdef NEW_HASH
int hash(const unsigned char *in, int len)
{
  unsigned char X[48];
  memset( X, 0x00, 16);
  for( int i = 0; i < len; i += 16 ){
    for( int j = 0; j < 16; j ++ ){
      X[16+j] = in[i*16+j];
      X[32+j] = X[16+j] ^ X[j];
    }
    int t = 0;
    for( int j = 0; j < 18; j ++ ){
      for( int k = 0; k < 48; k ++ ){
        X[k] = t = X[k] ^ S[t];
      }
      t = (t + j) & 255;
    }
  }
  return
    ((int)X[0]) | 
    ((int)X[1]) << 8 | 
    ((int)X[2]) << 16 | 
    ((int)X[3]) << 24;
}

#define BUF_MAX 256
#endif

long long MOD = 1;
class State{
public:
#ifdef NEW_HASH
  int hashBufPos;
  unsigned char hashBuf[BUF_MAX];
  void add(int v){ hashBuf[(hashBufPos++)&(BUF_MAX-1)] ^= v; }
  int getHash() const { return hash(hashBuf, BUF_MAX); }
#else
  void add(int v){ srand(rand() ^ v); }
  int getHash() const { return rand(); }
#endif
  State(const icfpc2012::Simulator &simulator, int pos, int limit)
#ifdef NEW_HASH
   : hashBufPos(0)
#endif
  {
    (void)limit; // to avoid unused parameter warning
#ifdef NEW_HASH
    memset( hashBuf, 0x00, sizeof(hashBuf) );
#else
    srand(0);
#endif
    const Point &robot = simulator.getRobot();
    long long state0 = robot.x + robot.y * simulator.GetWidth();
    add(pos);
    const vector< pair<Point,Point> > &rocks = simulator.getRocks();
    for( int i = 0; i < (int)rocks.size(); i ++ ){
//      add(rocks[i].first.x >> 8);
      add(rocks[i].first.x);
//      add(rocks[i].first.y >> 8);
      add(rocks[i].first.y);
    }
    const vector<bool> &earthState = simulator.getEarthState();
    for( int i = 0; i < (int)earthState.size(); i ++ ){
      add(earthState[i]);
    }
    const vector<Point> &lambdas = simulator.getLambdas();
    for( int i = 0; i < (int)lambdas.size(); i ++ ){
      if( simulator.getField(lambdas[i].x, lambdas[i].y) == '\\' ){
//        add(lambdas[i].x >> 8);
        add(lambdas[i].x);
//        add(lambdas[i].y >> 8);
        add(lambdas[i].y);
      }
    }

    long long MOD2 = simulator.GetWidth() * simulator.GetHeight();
    state1 = (unsigned int)getHash();
    if( conf_separate && MOD >= MOD2 )
      state1 = state1 % (MOD / MOD2) * MOD2 + state0;
    else
      state1 = (state1 + state0) % MOD;
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
const int action_max = 5;

volatile int to_exit = 0;

static void sig_int(int) { to_exit = 1; /*fprintf( stderr, "SIG_INT\n" );*/ }

typedef pair<int,int> solve_t;

solve_t solve(icfpc2012::Simulator &simulator, const vector<Point> &lambdas, int pos, int len, int limit)
{
  while( pos+1 < (int)lambdas.size() && simulator.getField(lambdas[pos].x, lambdas[pos].y) != '\\' )
    pos ++;
  if( to_exit || len >= limit )
    return solve_t(0,-65536);

// score = max lambdas
  State state1 = getState(simulator, pos, len);

  if( visited[state1.getValue()] )
    return solve_t(0,-65536);
  visited[state1.getValue()] = action_max + 2;

  solve_t best(0,-65536);
  int best_action = action_max + 1;
  for( int action = 0; to_exit == 0 && action < action_max; action ++ ){
    icfpc2012::Simulator &t = simulator;
    EndState state  = t.Run(ms[action]);
/*
    string this_history = "?";
    this_history[0] =action_letter[action];
*/

    if (state == NOT_GAME_OVER) {
      solve_t v = solve(t, lambdas, pos, len+1, limit );
      if( best < v )
        best = v, best_action = action;
    }
    else if( state == WIN_END ){
      solve_t v(1000000000, -(len+1));
      if( best < v )
        best = v, best_action = action + 16;
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

string getAnsString(icfpc2012::Simulator &simulator, const vector<Point> &lambdas, int pos, int len, int limit)
{
  stringstream ss;

  int count = 0;
  int aborted = 0;
  while(1){
    while( pos+1 < (int)lambdas.size() && simulator.getField(lambdas[pos].x, lambdas[pos].y) != '\\' )
      pos ++;
    if( to_exit || len >= limit ){
      aborted = 1;
      break;
    }

    State state1 = getState(simulator, pos, len);
    int v = visited[state1.getValue()];
    if( v == 0 ){
      aborted = 1;
      break;
    }
    int action = v - 1;
    icfpc2012::Simulator &t = simulator;
    if( action % 16 < 0 || action_max <= action % 16 ){
      aborted = 1;
      break;
    }
    EndState state = t.Run(ms[action % 16]);
    count ++;
    ss << (char)action_letter[action % 16];
    if( action >= 16 ) break;
    if (state != NOT_GAME_OVER) {
      aborted = 1;
      break;
    }
  }
  for( int i = 0; i < count; i ++ )
    simulator.Undo();
  if( aborted )
    return "";
  else
    return ss.str();
}

pair<solve_t, string> solveID(icfpc2012::Simulator &simulator, const vector<Point> &lambdas, int limit0)
{
  memset(visited, 0x00, sizeof(visited) );
  solve_t nolim = solve(simulator, lambdas, 0, 0, limit0 - 1);
  string ans = getAnsString(simulator, lambdas, 0, 0, limit0 - 1);

//  fprintf( stderr, "%d visited\n", (int)visited.size() );
  if( nolim.first <= 0 ) return pair<solve_t, string>(nolim, ans);
  for( int L = 0, R = -nolim.second-1; conf_ID && L + 1 < R && to_exit == 0;  ){
    int M = (L + R) / 2;
    if( conf_log )
      fprintf( stderr, "%d %d %d", L, R, M );
    memset(visited, 0x00, sizeof(visited) );
    solve_t v = solve(simulator, lambdas, 0, 0, M);
    if( v.first > 0 ){
      if( nolim < v ){
        ans = getAnsString(simulator, lambdas, 0, 0, M);
        if( ans != "" )
          nolim = v;
      }
      R = min(M, -v.second-1);
      if( conf_log )
        fprintf( stderr, " -> %d\n", R );
    }
    else{
      if( conf_log )
        fprintf( stderr, "\n" );
      L = M;
    }
  }
  return pair<solve_t, string>(nolim, ans);
}

pair<solve_t, string> solvePermID(icfpc2012::Simulator &simulator, int limit)
{
  vector<Point> lambdas = simulator.getLambdas();

  pair<solve_t, string> best(solve_t(0,-65536), "");

  for( int c = 0; to_exit == 0 && c < 10; c ++ ){
     random_shuffle(lambdas.begin(),lambdas.end());
     pair<solve_t, string> v = solveID(simulator, lambdas, limit);
     if( best.first < v.first && v.second != "" ){
       best = v;
       if( best.first.first == 1000000000 ) break;
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

  for( int i = 0; i < 256; i ++ )
    S[i] = i;
  random_shuffle(S, S+256);


  if( signal(SIGINT, sig_int) == SIG_ERR){
    printf( "FAIL: signal\n" );
    exit(1);
  }

  icfpc2012::Simulator simulator;
  cin >> simulator;

  pair<solve_t, string> best(solve_t(0,-65536),"");
  for( MOD = (1 << 20) - 1; to_exit == 0 && MOD < STATE_MAX; MOD = MOD * 2 + 1 ){
    pair<solve_t, string> v = solvePermID(simulator, best.first.first == 1000000000 ? -best.first.second : simulator.GetWidth()*simulator.GetHeight());
    if( conf_log )
      fprintf( stderr, "%lld -> %d (%s %d)\n", MOD, v.first.first, v.second.c_str(), -v.first.second ); // FIXME
    if( best < v && v.second != "" )
      best = v;
  }
  cout << best.second << "A" << endl; // FIXME
  cout.flush();

/*
  for( set<State>::const_iterator it = visited.begin(), ite = visited.end(); it != ite; ++ it ){
    it->dump();
  }
*/

//  printf( "Base score = %d (%s)\n", best.first, best.second.c_str() );
/*
  while(1){
   
    solve_t v = solvePerm(simulator);
    printf( "Best score = %d (%s)\n", v.first, v.second.c_str() );
  }
*/
  return 0;
}
