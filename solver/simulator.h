#ifndef ICFP2012_SIMULATOR_H_
#define ICFP2012_SIMULATOR_H_

#include <deque>
#include <iosfwd>
#include <string>
#include <vector>
#include <utility>

namespace icfpc2012 {
enum Movement {
  LEFT,
  RIGHT,
  UP,
  DOWN,
  WAIT,
  ABORT,
};

enum EndState {
  NOT_GAME_OVER,
  WIN_END,
  LOOSE_END,
  ABORT_END,
};

struct Point {
  int x;
  int y;
  Point(int x, int y) : x(x), y(y) {
  }
  Point() {}

  bool operator==(const Point& other) const {
    return x == other.x && y == other.y;
  }
  bool operator!=(const Point& other) const {
    return !(*this == other);
  }
  bool operator<(const Point& other) const {
    if( x != other.x ) return x < other.x;
    return y < other.y;
  }
};

struct History {
  Movement movement;
  Movement actual_movement;
  char prev_cell;
  int water_count;
  std::vector<std::pair<int, Point> > updated_rocks;
};

class Simulator {
 public:
  Simulator();
  ~Simulator();

  // Runs one cycle (move the robot -> then update the field) at once.
  EndState Run(Movement movement);
  void Undo();

  int GetScore() const ;
  std::string DebugString() const {
    return DebugStringInternal(false);
  }
  std::string DebugColoredString() const {
    return DebugStringInternal(true);
  }

  int GetWidth() const { return width_; }
  int GetHeight() const { return height_; }

  int getField(int x, int y) const { return field_[y][x]; }
  Point getRobot() const { return robot_; }
  Point getLift() const { return lift_; }

  bool IsLiftOpen() const { return total_lambda_ <= collected_lambda_; }

  const std::vector< Point > &getEarths() const { return earths_;}
  std::vector< bool > getEarthState() const {
    std::vector<bool> ret(earths_.size(), false);
    for(int i = 0; i < (int)earths_.size(); i ++ ){
      if( getField(earths_[i].x, earths_[i].y) == '.' )
        ret[i] = true;
    }
    return ret;
  }
  const std::vector< Point > &getLambdas() const { return lambdas_;}
  const std::vector<std::pair<Point, Point> > &getRocks() const { return rocks_;}
  size_t GetHistoryLength() const { return history_.size(); }

  int initial_water() const { return initial_water_; }
  int flooding() const { return flooding_; }
  int waterproof() const { return waterproof_; }
  int current_water_count() const { return current_water_count_; }
  int GetCollectedLambda() const { return collected_lambda_; }

 private:
  friend std::istream& operator>>(std::istream&, Simulator&);

  Movement Move(Movement movement, History *history);
  void Update(History *history);
  EndState end_state() const { return end_state_; }

  std::string DebugStringInternal(bool is_colored) const ;

  // TODO get rid of vector<string> representation to compact the memory.
  std::vector<std::string> field_;
  int width_;
  int height_;
  int total_lambda_;
  int collected_lambda_;

  int initial_water_;
  int flooding_;
  int waterproof_;
  int current_water_count_;

  Point robot_;
  Point lift_;
  std::vector<std::pair<Point, Point> > rocks_;

  std::vector< Point > earths_;
  std::vector< Point > lambdas_;
  EndState end_state_;


  std::deque<History> history_;

  // Caution: The copy is slow!!!
};
std::istream& operator>>(std::istream& is, Simulator& simulator);

}  // namespace icfpc2012

#endif  // ICFP2012_SIMULATOR_H_
