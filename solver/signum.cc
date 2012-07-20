#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <set>
#include <map>
#include <vector>
#include <string.h>
#include <algorithm>
#include <sstream>
#include <signal.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>

#include "boulder.h"
// #include "solver_main.h"

using namespace std;
using namespace icfpc2012;

bool similar(const GameState &s1, const GameState &s2 )
{
  return s1.field().robot().position() == s2.field().robot().position();
}

pair<int, string> shorten(const icfpc2012::GameStateStack& simulator, string ans, bool (*Yield)())
{
  int valid_count = 0;
  int best_score;
  {
    GameState st = simulator.current_state();
    for( int i = 0; i < (int)ans.size(); i ++ ){
      if(Yield()){ return pair<int, string>(-1,""); }
      Movement move = MovementFromChar(ans[i]);
      st = Simulate(st, move, true);
    }
    best_score = st.score();
  }
  
// Simulate(const GameStateStack& current_stack, Movement move);

//  cerr << "Best = " << best_score << endl;
  //while(1) // for( int len = len_min; len < (int)ans.size(); len ++ )
  //for( int len = len_min; len < (int)ans.size(); len ++ )
  for( int len = 32; len >= 2; -- len )
  {
    //int len = 16;
    GameState st = simulator.current_state();
    vector<GameState> history(len,st);
    valid_count = 0;
    for( int i = 0; i < (int)ans.size(); i ++ ){
      if(Yield()){ goto END; }
      // int j0 = (len == len_min ? 2 : len);
      //int j0 = 2;
      int j0 = len;
      // for( int j = j0; j <= len; j ++ ){
      for( int j = len; j >= j0; j -- ){
        if( valid_count >= j && similar(st, history[(i-j+len) % len]) ){
          GameState st2 = history[(i-j+len) % len];
          int k;
          for( k = i; k < (int)ans.size(); k ++ ){
            if( Yield()) goto END;
            Movement move = MovementFromChar(ans[k]);
            st2 = Simulate(st2, move, true);
            if( st2.end_state() != NOT_GAME_OVER) break;
          }
          if( st2.score() > best_score ){
  //          cerr << i-j << " - " << i << " = " << st2.score() << " vs " << best_score << " @ " << k << "/" << ans.size() << endl;
  //          cerr << "Best -> " << st2.score() << " @ " << j << endl;
            for( int k = 0; k < j; k ++ )
              ans[i-1-k] = '*';
            valid_count = 0;
            best_score = st2.score();
            st = history[(i-j+len) % len];
            break;
          }
        }
      }
      history[i % len] = st;
      valid_count ++;
      Movement move = MovementFromChar(ans[i]);
      st = Simulate(st, move, true);
    }
    ans.erase(remove(ans.begin(), ans.end(), '*'), ans.end());
  }
END:;
  ans.erase(remove(ans.begin(), ans.end(), '*'), ans.end());
  return pair<int,string>(best_score, ans);
}
