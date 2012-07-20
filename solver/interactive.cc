#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <fstream>

#include "boulder.h"

using namespace std;
using namespace icfpc2012;

const int KEY_MAP[][2] = {
  {'L', 'L'}, {'D', 'D'}, {'U', 'U'}, {'R', 'R'},
  {'h', 'L'}, {'j', 'D'}, {'k', 'U'}, {'l', 'R'},
  {'S', 'S'}, {'s', 'S'},
  {'.', 'W'}, {'q', 'A'},
  {KEY_LEFT, 'L'}, {KEY_DOWN, 'D'}, {KEY_UP, 'U'}, {KEY_RIGHT, 'R'},
  {0, 0},
};
const int COLOR_WATER = 1;

class InteractiveSimulator {
 public:
  InteractiveSimulator(const GameState& initial_state)
      : stack_(initial_state) {
  }

  void Run() {
    StartCurses();
    running_ = true;
    for (;;) {
      erase();
      Draw();
      if (!running_) {
        break;
      }
      int key = getch();
      OnKey(key);
    }
    EndCurses();
    PrintResult();
  }

 private:
  static void StartCurses() {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, true);
    start_color();
    use_default_colors();
    init_pair(COLOR_WATER, -1, COLOR_BLUE);
    atexit(EndCurses);
  }

  static void EndCurses() {
    endwin();
  }

  void Draw() {
    int current_line = 0;
    string status_string = current_state().DebugString(false);
    {
      istringstream sin(status_string);
      while (sin) {
        string line;
        getline(sin, line);
        mvaddstr(current_line++, 0, line.c_str());
      }
    }

    string field_string = current_state().field().DebugString();
    {
      istringstream sin(field_string);
      for (int i = 0; sin; ++i) {
        string line;
        getline(sin, line);
        int y = current_state().field().height() - 1 - i;
        int attr = (0 <= y && y < current_state().GetWaterLevel() ?
                    COLOR_PAIR(COLOR_WATER) : 0);
        attrset(attr);
        mvaddstr(current_line++, 0, line.c_str());
      }
      attrset(0);
    }

    if (current_state().end_state() != NOT_GAME_OVER) {
      mvaddstr(current_line++, 0, GetCommand().c_str());
    }

    refresh();
  }

  void OnKey(int key) {
    char ch = 0;
    for (int i = 0; KEY_MAP[i][0] != 0; ++i) {
      if (key == KEY_MAP[i][0]) {
        ch = KEY_MAP[i][1];
        break;
      }
    }
    if (ch != 0) {
      if (ch == 'A' && current_state().end_state() != NOT_GAME_OVER) {
        running_ = false;
      }
      if (current_state().end_state() == NOT_GAME_OVER) {
        Movement move = MovementFromChar(ch);
        stack_ = Simulate(stack_, move);
      }
    } else if (key == 'u') {
      stack_ = stack_.Undo();
    }
  }

  void PrintResult() {
    printf("%d\n%s\n", stack_.current_state().score(), GetCommand().c_str());
  }

  string GetCommand() {
    return stack_.GetCommand();
  }

  const GameState& current_state() {
    return stack_.current_state();
  }

  GameStateStack stack_;
  bool running_;
};

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("usage: %s <mapfile>\n", argv[0]);
    return 1;
  }

  GameState initial_state;
  {
    fstream fin(argv[1]);
    fin >> initial_state;
  }

  InteractiveSimulator simulator(initial_state);
  simulator.Run();

  return 0;
}
