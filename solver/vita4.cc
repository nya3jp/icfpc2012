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

#include "boulder.h"
#include "solver_main.h"

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
vector<Point> earthPositions;

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
  State(const icfpc2012::GameStateStack &simulator, int pos, int limit)
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
    const icfpc2012::Field &field = simulator.current_state().field();
    const Point &robot = field.robot().position();
    long long state0 = robot.x + robot.y * field.width();
    add(pos);
    add(simulator.current_state().lambda_collected());
    add(simulator.current_state().water_count());
    add(simulator.current_state().GetWaterLevel());
    {
      const icfpc2012::RockList &rocks = field.rock_list();
      for( int i = 0; i < (int)rocks.size(); i ++ ){
        add(rocks[i].position().x);
        add(rocks[i].position().y);
      }
    }

    {
      const icfpc2012::RockList &rocks = field.higher_order_rock_list();
      for( int i = 0; i < (int)rocks.size(); i ++ ){
        if( field.GetCell(rocks[i].position()).type() == HIGHER_ORDER_ROCK ){
          add(rocks[i].position().x);
          add(rocks[i].position().y);
        }
      }
    }

    {
      const icfpc2012::WadlerList &wadlers = field.wadler_list();
      for( int i = 0; i < (int)wadlers.size(); i ++ ){
        add(wadlers[i].position().x);
        add(wadlers[i].position().y);
      }
    }

    for( int i = 0; i < (int)earthPositions.size(); i ++ ){
      add( field.GetCell(earthPositions[i]).type() );
    }

    const LambdaList &lambdas = field.lambda_list();
    for( int i = 0; i < (int)lambdas.size(); i ++ ){
      if( field.GetCell(lambdas[i].position()).type() == LAMBDA ){
        add(lambdas[i].position().x);
        add(lambdas[i].position().y);
      }
    }

    long long MOD2 = field.width() * field.height();
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

State getState(const icfpc2012::GameStateStack &simulator, int pos, int limit)
{
  return State(
    simulator,
    pos,
    limit
  );
}
const Movement ms[] = {LEFT, RIGHT, UP, DOWN, WAIT};
const char action_letter[] = "LRUDW";

typedef pair<int,int> solve_t;

solve_t solve(const icfpc2012::GameStateStack &simulator, const vector<Point> &lambdas, int pos, int len, int limit)
{
  const icfpc2012::Field &field = simulator.current_state().field();
  while( pos+1 < (int)lambdas.size() && field.GetCell(lambdas[pos]).type() != LAMBDA )
    pos ++;
  Yield();
  {
    icfpc2012::GameStateStack t = Simulate(simulator, ABORT);
    MaybeUpdateSolution(t);
  }
  if( len >= limit )
    return solve_t(0,-65536);

// score = max lambdas
  State state1 = getState(simulator, pos, len);

  if( visited[state1.getValue()] )
    return solve_t(0,-65536);
  visited[state1.getValue()] = NUM_MOVEMENTS + 2;

  solve_t best(0,-65536);
  int best_action = NUM_MOVEMENTS + 1;
  for( int action = 0; action < NUM_MOVEMENTS; action ++ ){
    Yield();
    icfpc2012::GameStateStack t = Simulate(simulator, MOVEMENTS[action]);
    if (t.current_state().end_state() == NOT_GAME_OVER) {
      solve_t v = solve(t, lambdas, pos, len+1, limit );
      if( best < v )
        best = v, best_action = action;
    }
    else if( t.current_state().end_state() == WIN_END ){
      MaybeUpdateSolution(t);
      solve_t v(1000000000, -(len+1));
      if( best < v )
        best = v, best_action = action + 16;
    }
  }
    visited[state1.getValue()] = best_action + 1;
    return best;
}

/*
string getAnsString(icfpc2012::GameStateStack &simulator, const vector<Point> &lambdas, int pos, int len, int limit)
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
    icfpc2012::GameStateStack &t = simulator;
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
  if( !aborted )
    MaybeUpdateSolution(simulator);
  for( int i = 0; i < count; i ++ )
    simulator.Undo();
  if( aborted )
    return "";
  else
    return ss.str();
}
*/
solve_t solveID(const icfpc2012::GameStateStack &simulator, const vector<Point> &lambdas, int limit0)
{
  memset(visited, 0x00, sizeof(visited) );
  solve_t nolim = solve(simulator, lambdas, 0, 0, limit0 - 1);
//  string ans = getAnsString(simulator, lambdas, 0, 0, limit0 - 1);

//  fprintf( stderr, "%d visited\n", (int)visited.size() );
  if( nolim.first <= 0 ) return nolim;
  for( int L = 0, R = -nolim.second-1; conf_ID && L + 1 < R;  ){
     Yield();
    int M = (L + R) / 2;
    if( conf_log )
      fprintf( stderr, "%d %d %d", L, R, M );
    memset(visited, 0x00, sizeof(visited) );
    solve_t v = solve(simulator, lambdas, 0, 0, M);
    if( v.first > 0 ){
      if( nolim < v ){
/*
        ans = getAnsString(simulator, lambdas, 0, 0, M);
        if( ans != "" )
*/
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
  return nolim;
}

solve_t solvePermID(const icfpc2012::GameStateStack &simulator, int limit)
{
  const LambdaList &lambdas_raw = simulator.current_state().field().lambda_list();
  vector<Point> lambdas(lambdas_raw.size());
  for( int i = 0; i < (int)lambdas_raw.size(); i ++ ){
    lambdas[i] = lambdas_raw[i].position();
  }

  solve_t best(0,-65536);

  for( int c = 0; c < 10; c ++ ){
     Yield();
     random_shuffle(lambdas.begin(),lambdas.end());
     solve_t v = solveID(simulator, lambdas, limit);
     if( best < v ){
       best = v;
       if( best.first == 1000000000 ) break;
     }
  }

  return best;
}

void SolverMain(const icfpc2012::GameStateStack& simulator) {
  for( int i = 0; i < 256; i ++ )
    S[i] = i;
  random_shuffle(S, S+256);

  solve_t best(0,-65536);

  {
    const icfpc2012::Field &field = simulator.current_state().field();
    earthPositions.clear();
    for( int y = 0; y < field.height(); y ++ ){
      for( int x = 0; x < field.width(); x ++ ){
        switch( field.GetCell(x,y).type() ){
          case CLOSED_LIFT:
          case EARTH:
          case TRAMPOLINE_A:
          case TRAMPOLINE_B:
          case TRAMPOLINE_C:
          case TRAMPOLINE_D:
          case TRAMPOLINE_E:
          case TRAMPOLINE_F:
          case TRAMPOLINE_G:
          case TRAMPOLINE_H:
          case TRAMPOLINE_I:
          case TARGET_1:
          case TARGET_2:
          case TARGET_3:
          case TARGET_4:
          case TARGET_5:
          case TARGET_6:
          case TARGET_7:
          case TARGET_8:
          case TARGET_9:
          case RAZOR:
            earthPositions.push_back(Point(x,y));
            break;
        }
      }
    }
  }
  for( MOD = (1 << 20) - 1; MOD < STATE_MAX; MOD = MOD * 2 + 1 ){
    Yield();
    solve_t v = solvePermID(simulator, best.first == 1000000000 ? -best.second : 1000000);
    if( conf_log )
      fprintf( stderr, "%lld -> %d (??? %d)\n", MOD, v.first, -v.second ); // FIXME
    if( best < v )
      best = v;
  }
  return;
}
