#include "boulder.h"

#include <signal.h>

#include <iostream>
#include <cstdlib>
#include <vector>

using namespace icfpc2012;
using namespace std;

static volatile bool g_interrupted = false;
static string best_solution = "A";
static int max_score = 0;

static void OnSIGINT(int) {
  g_interrupted = true;
  signal(SIGINT, SIG_DFL);
}

static void PrintBestSolutionAndExit() {
  cout << best_solution << "A" << endl;
  exit(0);
}

void Yield() {
  if (g_interrupted) {
    PrintBestSolutionAndExit();
  }
}

void MaybeUpdateSolution(
    int score, const string& prefix, const string& solution) {
  if (score > max_score) {
    max_score = score;
    best_solution = prefix + solution;
    // cerr << "Updated: " << max_score << ", " << best_solution << endl;
  }
}

const Movement kMove[] = { LEFT, RIGHT, UP, DOWN, SHAVE };
const Point kAround[] = {
  Point(-1, -1), Point(0, -1), Point(1, -1),
  Point(-1, 0), Point(1, 0),
  Point(-1, 1), Point(0, 1), Point(1, 1),
};

bool HasWadlerAroundRobot(const GameState& state) {
  const Field& field = state.field();
  const Point robot_position = field.robot().position();
  for (int i = 0; i < 8; ++i) {
    if (field.GetCell(robot_position + kAround[i]).type() == WADLER) {
      return true;
    }
  }
  return false;
}

pair<int, bool> Run(int depth, const GameState& initial, string *result) {
  if (initial.end_state() == NOT_GAME_OVER && depth > 0) {
    const int max_rand = HasWadlerAroundRobot(initial) ? 5 : 4;
    for (int i = 0; i < 10; ++i) {
      int x = rand() % max_rand;
      Movement movement = kMove[x];
      if (IsMovable(initial, movement)) {
        result->push_back("LRUDS"[x]);
        return Run(depth - 1, Simulate(initial, movement), result);
      }
    }
  }
  return make_pair(initial.score(), initial.end_state() == WIN_END);
}

int FindCommonPrefix(const string& s1, const string& s2) {
  int len = min(s1.length(), s2.length());
  for (int i = 0; i < len; ++i) {
    if (s1[i] != s2[i]) {
      return i;
    }
  }
  return len;
}


void SolverMain(const GameState& initial_game_state) {
  GameState game_state(initial_game_state);

  string result;
  int depth = 500;
  int num_candidates = 10000;
  int max_num_candidates = 40000;
  vector<pair<int, string> > data;
  data.resize(max_num_candidates);
  bool solved = false;
  // int trial = 0;
  while (true) {
    // cerr << "trial: " << (trial++) << endl;
    // cerr << "result: " << result << endl;

    for (int j = 0; j < num_candidates; ++j) {
      if (j % 100 == 0) Yield();
      data[j].second.clear();
      pair<int, bool> run_result = Run(depth, game_state, &data[j].second);
      data[j].first = run_result.first;
      solved |= run_result.second;
      MaybeUpdateSolution(run_result.first, result, data[j].second);
    }
    vector<pair<int, string> >::iterator iter =
        max_element(data.begin(), data.end());
    iter_swap(data.begin(), iter);
    iter = max_element(data.begin() + 1, data.end());
    iter_swap(data.begin() + 1, iter);
    // sort(data.rbegin(), data.rend());
    // cerr << "data[0]: " << data[0].second << ", " << data[0].first << endl;
    // cerr << "data[1]: " << data[1].second << ", " << data[1].first << endl;

#if 0
    int index = FindCommonPrefix(data[0].second, data[1].second);
    if (index >= 3) {
      for (int x = 0; x < index; ++x) {
        game_state =
            Simulate(game_state, MovementFromChar(data[0].second[x]));
      }
      result.append(data[0].second.substr(0, index));
    } else {
      // Move at least one character.
      int count[] = {0, 0, 0, 0};
      int threshold = 2;
      for (threshold = 2; threshold < 20; ++threshold) {
        if (data[threshold].first > data[threshold + 1].first * 3) {
          break;
        }
      }
      for (int x = 0; x < threshold; ++x) {
        if (!data[x].second.empty()) {
          ++count[MovementFromChar(data[x].second[0])];
        }
      }
      int* max_elem = max_element(count, count + 4);
      Movement movement = static_cast<Movement>(max_elem - count);
      game_state = Simulate(game_state, movement);
      result.push_back("LRUD"[movement]);
    }

#else
    bool retry = false;
    if (data[0].second.empty()) {
      // cerr << "NO search result." << endl;
      if (solved) {
        return;
      }
      retry = true;
    }

    for (int index = 0;
         index < static_cast<int>(
             min(data[0].second.length(), data[1].second.length()));
         ++index) {
      if (data[0].second[index] != data[1].second[index]) {
        if (index == 0) {
          depth += min(depth + 500, 5000);
          num_candidates = min(num_candidates + 1000, max_num_candidates);
        } else {
          depth = 500;
          num_candidates = 10000;
        }
        break;
      }
      game_state =
          Simulate(game_state, MovementFromChar(data[0].second[index]));
      result.push_back(data[0].second[index]);
    }

    if (game_state.end_state() == WIN_END) {
      return;
    }
    if (retry || game_state.end_state() != NOT_GAME_OVER) {
      // cerr << "RESET" << endl;
      result.clear();
      game_state = initial_game_state;
      depth = 500;
      num_candidates = 10000;
    }
#endif
  }
}


int main() {
  ios_base::sync_with_stdio(false);
  signal(SIGINT, OnSIGINT);

  GameState initial_state;
  cin >> initial_state;
  SolverMain(initial_state);
  Yield();
  PrintBestSolutionAndExit();

  return 0;
}
