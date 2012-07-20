// TODO:
// local path improver
// local path search
// prevent too long steps
// fall back to random walk
// add subgoals
// genetic crossing

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <numeric>
#include <algorithm>
#include <functional>
#include <set>
#include <map>
#include <queue>
#include <string>
#include <signal.h>

#include "boulder.h"
#include "signum.h"

using namespace std;
using namespace icfpc2012;

// [0,nlambda): lambda
// [nlambda,nlambda+1): lifter
// [nlambda+1,): subgoal_begin
vector<Point> checkpoints;
size_t nlambda;
size_t subgoal_begin;

volatile int to_exit = 0;

bool yielder()
{
  return to_exit > 0;
}

static void sig_int(int)
{
  to_exit = 1;
  signal(SIGINT, SIG_DFL);
}

const double prob_wait = 0.1;
const double prob_shave = 0.5;

const int MAX_TRIAL = 100;
vector<vector<vector<int> > > potential;
bool use_potential = true;

vector<vector<bool> > wall;

void init_potential(const GameState &game)
{
  const int large = 999999;
  int w, h;
  w = game.field().width();
  h = game.field().height();

  potential.resize(checkpoints.size() + 1);
  for(size_t i = 0; i < potential.size(); ++i){
    vector<vector<int> > &potm = potential[i];
    potm = vector<vector<int> >(w + 2, vector<int>(h + 2, large)); // sentinel

    // make Potential table (O(WH))
  
    queue<pair<Point, int> > q;
    Point target = checkpoints[i];
    q.push(make_pair(target, 0));

    // perform BFS to calculate "potential" to the target
    while(!q.empty()){
      Point c = q.front().first;
      int pot = q.front().second;
      q.pop();

      if(c.x < 0 || c.x >= w) continue;
      if(c.y < 0 || c.y >= h) continue;
      Cell fld = game.field().GetCell(c);
      if(fld.type() == WALL)
        continue;
      if(potm[c.x + 1][c.y + 1] != large) continue;

      potm[c.x + 1][c.y + 1] = pot;
      
      q.push(make_pair(Point(c.x - 1, c.y    ), pot + 1));
      q.push(make_pair(Point(c.x + 1, c.y    ), pot + 1));
      q.push(make_pair(Point(c.x    , c.y - 1), pot + 1));
      q.push(make_pair(Point(c.x    , c.y + 1), pot + 1));
    }
  }
}



static char char_of_movement(Movement e)
{
  switch(e) {
  case LEFT:
    return 'L';
  case RIGHT:
    return 'R';
  case UP:
    return 'U';
  case DOWN:
    return 'D';
  case WAIT:
    return 'W';
  case SHAVE:
    return 'S';
  case ABORT:
    return 'A';
  }
  return '\0';
}

string convert_movements(const vector<int>& v)
{
  string ret;
  for(size_t j = 0; j < v.size(); ++j)
    ret += char_of_movement((Movement)v[j]);
  return ret;
}

struct gene 
{
  vector<int> visit_order;
  vector<vector<int> > movements;
  vector<bool> check_success;

  void dump() const {
    // movements
    for(size_t i = 0; i < visit_order.size(); ++i){
      cerr << "(" << checkpoints[i].x << "," << checkpoints[i].y << ") ";
      const vector<int> &sect = movements[i];
      cerr << convert_movements(sect) << endl;
    }
    cerr << convert_movements(movements.back()) << endl;
    cerr << "Score: " << score << endl;
  }
  
  string output_sequence() const {
    string ret;
    for(size_t i = 0; i < movements.size(); ++i)
      ret += convert_movements(movements[i]);
    return ret + 'A';
  }

  void update_score(GameState &sim) {
    score = sim.score();
    // get connectivity
    // Following code gives penalty to the unreachable lifter,
    // but disabled because of adversaliry effect
    if(0){ 
      Point cur = sim.field().robot().position();
      Point lift = sim.field().lift().position();
      if(cur == lift){
        genescore = score;
      }else{
        bool reachable = false;
        queue<Point> q;
        q.push(cur);
        int w, h;
        w = sim.field().width();
        h = sim.field().height();
        vector<vector<bool> > visited(w, vector<bool>(h, false));
      
        // perform BFS to check connectivity to the lift
        // gives small penalty to unreachable lifter
        while(!q.empty()){
          Point c = q.front();
          q.pop();
        
          if(c.x < 0 || c.x >= w) continue;
          if(c.y < 0 || c.y >= h) continue;
          if(visited[c.x][c.y]) continue;
          visited[c.x][c.y] = true;
        
          CellType fld = sim.field().GetCell(c).type();
          if(fld == EARTH || fld == EMPTY || fld == ROBOT){
            q.push(Point(c.x - 1, c.y    ));
            q.push(Point(c.x + 1, c.y    ));
            q.push(Point(c.x    , c.y - 1));
            q.push(Point(c.x    , c.y + 1));
            continue;
          }
          if(fld == CLOSED_LIFT || fld == OPEN_LIFT){
            reachable = true;
            break;
          }
          // TODO: TRAMPOLINE
        }
        //cerr << "Reachable: " << (reachable?"true":"false") << endl;
        if(!reachable){
          genescore = score - 10  * sim.lambda_collected();
        }else{
          // reachable
          genescore = score;
        }
      }
    }else{
      genescore = score;
    }
  }

  int score;
  int genescore;
};

/* 
   Try to run simulation. 
   returns game state
 */
int try_run(GameState &sim, const vector<int>& movement)
{
  GameState original = sim;
  for(size_t i = 0; i < movement.size(); ++i){
    sim = Simulate(sim, (Movement)movement[i], true);
    if(sim.end_state() == LOSE_END){
      // ＞ 突然の死 ＜
      sim = original;
      return -1;
    } 
    if(sim.end_state() == WIN_END){
      return 1;
    }
    // fixme TODO: ABORT_END
  }
  return 0;
}

/*
  Generate one possible (not optimal) route to next specified point (by lambda index).
  FIXME TODO: should LEFT/RIGHT/UP/DOWN be reordered this function does not work
  returns 1 if successfully reached and WIN.
  returns 0 if successfully reached. Sim is now in new state.
  returns -1 if aborted, and sim is rolled back to the previous state
 */
int
generate_route(GameState& simstate, int checkpoint, vector<int>& movement)
{
  GameState sim = simstate;
  Point to = checkpoints[checkpoint];
  movement.clear();
  int trial = 0;
  /*
  cerr << "rerouting from " << sim.getRobot().x << "," << sim.getRobot().y 
       << " to " << to.x << "," << to.y << endl;
  */

  while(true){
    // FIXME TODO: suggest according to BFS path
    // LRUDW
    const int nhands = 6;
    double weight[nhands] = {1.0, 1.0, 1.0, 1.0, 0.0, prob_wait};
    
    Point cur = sim.field().robot().position();
    // get relative
    int dx = to.x - cur.x;
    int dy = to.y - cur.y;

    if(dx == 0 && dy == 0){
      //cerr << "Found route with " << movement.size() << " steps and " << trial << "trials" << endl;
      // successfully reached
      simstate = sim;
      return 0;
    }
    if(use_potential){
      vector<vector<int> > &chkpotential = potential[checkpoint];
      int p = chkpotential[cur.x + 1][cur.y + 1];
      double wt_decrease = 2.0;
      if(chkpotential[cur.x    ][cur.y + 1] < p) weight[0] += wt_decrease;
      if(chkpotential[cur.x + 2][cur.y + 1] < p) weight[1] += wt_decrease;
      if(chkpotential[cur.x + 1][cur.y    ] < p) weight[3] += wt_decrease;
      if(chkpotential[cur.x + 1][cur.y + 2] < p) weight[2] += wt_decrease;
    }else{
      if(dx < 0) {
        weight[0] = (double)-dx;
      }else{
        weight[1] = (double)dx;
      }
      if(dy < 0) {
        weight[3] = (double)-dy;
      }else{
        weight[2] = (double)dy;
      }
    }
    if(wall[cur.x    ][cur.y + 1]) weight[0] = 0.;
    if(wall[cur.x + 2][cur.y + 1]) weight[1] = 0.;
    if(wall[cur.x + 1][cur.y    ]) weight[3] = 0.;
    if(wall[cur.x + 1][cur.y + 2]) weight[2] = 0.;

    int wadlers = 0;
    for(int i = -1; i <= 1; ++i)
      for(int j = -1; j <= 1; ++j)
        wadlers += sim.field().GetCell(cur.x + i, cur.y + j).type() == WADLER;
    if(wadlers > 0 && sim.num_razors() > 0)
      weight[4] = prob_shave;

    double totalweight = accumulate(weight, weight + nhands, 0.0);
    double r = rand() / (RAND_MAX + 1.0) * totalweight;
    int curhand = 0;
    for(int i = 0; i < nhands; ++i){
      if(r < weight[i]) {
        curhand = i + LEFT;
        break;
      }
      r -= weight[i];
    }
    GameState newstate = Simulate(sim, (Movement)curhand, true);
    if(newstate.end_state() == LOSE_END){
      // no update to sim == undo one step
    }else{
      sim = newstate;
      movement.push_back(curhand);
      if(sim.end_state() == WIN_END){
        simstate = sim;
        return 1;
      }
    }
    trial++;
    if(trial > MAX_TRIAL){
      // never change the state == simstate unchanged
      movement.clear();
      // FIXME TODO: fixed constants will not work
      return -1;
    }
  }
}

/*
  Fix current gene so that it avoids dead-end
 */
void 
fix_gene(GameState &sim, gene &g, int fixfrom)
{
  //cerr << "Lifter st = " << (sim.IsLiftOpen() ? "open" : "closed") << endl;
  if(((size_t)fixfrom == nlambda && !(sim.lambda_collected() == sim.config().num_lambdas())) ||
     (size_t)fixfrom > nlambda){
    GameState st = Simulate(sim, ABORT, true);
    g.update_score(st);
    return;
  }
  

  int pointno;
  if((size_t)fixfrom < nlambda){
    pointno = g.visit_order[fixfrom];
  }else{
    //cerr << "Targets lifter" << endl;
    pointno = (int)nlambda;
  }
  Point target = checkpoints[pointno];

  // try to run until the end of this section
  GameState st_backup = sim;
  int r = try_run(sim, g.movements[fixfrom]);

  //cerr << "target: " << target.x << "," << target.y << endl;
  //cerr << "current: " << sim.getRobot().x << "," << sim.getRobot().y << endl;
  if(r == 1){
    // ＞ 突然の勝利 ＜
    for(size_t j = fixfrom + 1; j < g.movements.size(); ++j){
      g.movements[j].clear();
      g.check_success[j] = false;
    }
    g.check_success[fixfrom] = false; //FIXME
    g.update_score(sim);
    return;
  }

  bool failure = (r == -1);
  // failure -> fix current segment with generate_route
  if(r == 0 &&
     g.check_success[fixfrom] &&
     sim.field().robot().position() != target) {
    // failed to arrive required position
    // rewind all progress and generate route
    sim = st_backup;
    failure = true;
    g.movements[fixfrom].clear();
  }
  if(failure){
    //cerr << "Fixing " << fixfrom << endl;
    int r = generate_route(sim, pointno, g.movements[fixfrom]);
    if(r == 1){ 
      // ＞ 突然の勝利 ＜
      for(size_t j = fixfrom + 1; j < g.movements.size(); ++j){
        g.movements[j].clear();
        g.check_success[j] = false;
      }
      g.check_success[fixfrom] = false; //FIXME
      g.update_score(sim);
      return;
    }
    g.check_success[fixfrom] = (r == 0);
    //cerr << "Fix result: " << r << ", " << g.movements[fixfrom].size() << " steps" << endl;
  }
  
  // and fix the next chunk if exists
  fix_gene(sim, g, fixfrom + 1);
}

void 
instantiate_with_hints(const GameState &initialstate, gene &g)
{
  size_t n = nlambda;
  // This routine is called only for initial state generation
  g.check_success = vector<bool>(n + 1, true);
  g.movements = vector<vector<int> >(n + 1, vector<int>());
  GameState sim = initialstate;
  // All tedious works should be performed
  fix_gene(sim, g, 0);
}

int main(int argc, char* argv[]) {
  ios_base::sync_with_stdio(false);

  signal(SIGINT, sig_int);

  GameState simulator;
  cin >> simulator;

  // get lambda positions
  LambdaList ll = simulator.field().lambda_list();
  // In case type definition changed in the future...
  checkpoints.clear();
  for(LambdaList::iterator it = ll.begin();
      it != ll.end(); 
      ++it){
    checkpoints.push_back(it->position());
  }
  nlambda = checkpoints.size();
  checkpoints.push_back(simulator.field().lift().position());
  subgoal_begin = nlambda + 1;

  GameState initialsim = simulator;
  
  if((long long)simulator.field().width() * (long long)simulator.field().height() * (long long)checkpoints.size() > 100000000LL){
    use_potential = false;
  }

  if(use_potential)
    init_potential(simulator);

  {
    int w = simulator.field().width();
    int h = simulator.field().height();

    wall = vector<vector<bool> >(w + 2, (vector<bool>(h + 2, true)));
    for(int i = 0; i < w; ++i){
      for(int j = 0; j < h; ++j){
        wall[i + 1][j + 1] = (simulator.field().GetCell(i, j).type() == WALL);
      }
    }
  }
  
  const int NGENE = 200;
  const int NGENE_GOOD = 160;

  // Make genetic pools
  vector<gene> gene_pool(NGENE);
  gene bestgene;
  bestgene.score = 0;
  bestgene.genescore = 0;
  string compressed_gene;
  int compressed_gene_score = 0;

  // get initial solution: order checkpoints in arbitrary order  
  for(size_t gi = 0; gi < gene_pool.size(); ++gi){
    gene &g = gene_pool[gi];
    g.score = 0;
    g.genescore = 0;
    for(size_t i = 0; i < checkpoints.size(); ++i){
      g.visit_order.push_back(i);
    }
    random_shuffle(g.visit_order.begin(), g.visit_order.end());
    
    instantiate_with_hints(initialsim, g);

    //cerr << g.output_sequence() << " " << g.score << endl;
    if(g.score > bestgene.score) bestgene = g;
  }

  int counter = 0;

  // each iteration tries to update the genetic pools
  while(true){
    // gene pool is directly copied to the new ones
    vector<gene> new_gene_pool;
    size_t ngene_cur = gene_pool.size();

    // section pool
    map<pair<int, int>, vector<pair<size_t, size_t> > > section_pool;
    for(size_t gi = 0; gi < ngene_cur; ++gi){
      gene &g = gene_pool[gi];
      for(size_t i = 0; i < g.check_success.size() - 1; ++i){
        if(g.check_success[i] && g.check_success[i + 1]){
          section_pool[make_pair(g.visit_order[i], g.visit_order[i + 1])].push_back(make_pair(gi, i + 1));
        }
      }
    }

    for(size_t gi = 0; gi < min(ngene_cur, (size_t)10); ++gi){ 
      gene_pool.push_back(gene_pool[gi]); // first 10 are registered 5 times
      gene_pool.push_back(gene_pool[gi]);
      gene_pool.push_back(gene_pool[gi]);
      gene_pool.push_back(gene_pool[gi]);
    }

    for(size_t gi = 0; gi < ngene_cur; ++gi){
      gene_pool.push_back(gene_pool[gi]);
    }

    for(size_t gi = ngene_cur; gi < gene_pool.size(); ++gi){
      if(to_exit) break;
      gene &g = gene_pool[gi];
      
      // mutate and fix
      switch(rand() % 8){
      case 0:
      case 1:
      case 2:
      case 3:
      case 6:
      case 7:
        {
          if(g.visit_order.size() < 2) break;
          int r = rand() % (int)(g.visit_order.size() - 1);
          swap(g.visit_order[r], g.visit_order[r+1]);
          g.movements[r].clear();
          g.movements[r+1].clear();
          g.check_success[r] = true;
          g.check_success[r+1] = true;
          GameState sim = initialsim;
          fix_gene(sim, g, 0);
        }
        break;
      case 4:
      case 5:
        {
          // cross over from other gene
          if(g.visit_order.size() < 2) break;
          size_t r = (size_t)rand() % (g.visit_order.size() - 1);
          vector<pair<size_t, size_t> > &secp = section_pool[make_pair(g.visit_order[r], g.visit_order[r + 1])];
          if(secp.empty()) break;
          size_t q = (size_t)rand() % secp.size();
          g.movements[r + 1] = gene_pool[secp[q].first].movements[secp[q].second];
          GameState sim = initialsim;
          fix_gene(sim, g, 0);
        }
        if(0){
          if(g.visit_order.empty()) break;
          int r = rand() % (int)g.visit_order.size();
          vector<int> &v = g.movements[r];
          size_t i = 1;
          while(i < v.size()){
            if((v[i - 1] == LEFT && v[i] == RIGHT) ||
               (v[i - 1] == RIGHT && v[i] == LEFT)) {
              v.erase(v.begin() + i - 1, v.begin() + i + 1);
              continue;
            }
            if((v[i - 1] == UP && v[i] == DOWN) ||
               (v[i - 1] == DOWN && v[i] == UP)) {
              v.erase(v.begin() + i - 1, v.begin() + i + 1);
              continue;
            }
            i++;
          }

          g.check_success[r] = true;
          GameState sim = initialsim;
          fix_gene(sim, g, 0);
        }
        break;
      }
    }
    if(to_exit) break;

    // keeps best NGENE solutions without dup
    vector<pair<int, size_t> > score_ix;
    set<string> already;
    for(size_t gi = 0; gi < gene_pool.size(); ++gi) {
      string seq = gene_pool[gi].output_sequence();
      if(already.count(seq) == 0){
        score_ix.push_back(make_pair(gene_pool[gi].genescore, gi));
        already.insert(seq);
      }
    }
    sort(score_ix.begin(), score_ix.end(), greater<pair<int, size_t> >());
    if(score_ix.size() > (size_t)NGENE_GOOD){
      random_shuffle(score_ix.begin() + NGENE_GOOD, score_ix.end());
    }
    for(size_t i = 0; i < min(score_ix.size(), (size_t)NGENE); ++i){
      new_gene_pool.push_back(gene_pool[score_ix[i].second]);
      //gene &g = new_gene_pool.back();
      //cerr << g.output_sequence() << " " << g.score << endl;
    }

    // for debug: print best intermediate solution
    gene_pool.swap(new_gene_pool);

    for(size_t gi = 0; gi < gene_pool.size(); ++gi){
      gene &g = gene_pool[gi];
      if(g.score > bestgene.score) bestgene = g;
    }

    ++counter;
    //if(counter >= 500) break;
    if(counter % 10 == 0) {
      // update compressed sequence
      GameStateStack st(initialsim);
      std::pair<int, string> r = shorten(st, bestgene.output_sequence(), yielder);
      if(r.first > compressed_gene_score){
        compressed_gene_score = r.first;
        compressed_gene = r.second;
      }
      // cerr << "Compressed score: " << compressed_gene_score << endl;
    }
    //cerr << bestgene.score << endl;
  }

  //cerr << "Outputting best gene with score " << bestgene.score << endl;
  if(bestgene.score > compressed_gene_score){
    cout << bestgene.output_sequence() << endl;
  }else{
    cout << compressed_gene << endl;
  }

  return 0;
}
