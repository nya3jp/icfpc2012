#include "simulator.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <sstream>

using namespace std;

namespace icfpc2012 {
Simulator::Simulator() {
}
Simulator::~Simulator() {
}

namespace {
// Returns the expected next position.
Point GetNext(const Point& original, Movement move) {
  switch (move) {
  case LEFT:
    return Point(original.x - 1, original.y);
  case RIGHT:
    return Point(original.x + 1, original.y);
  case UP:
    return Point(original.x, original.y + 1);
  case DOWN:
    return Point(original.x, original.y - 1);
  default:
    abort();
  }
  return original;
}
// Returns the expected next position "of the rock."
Point GetSecondNext(const Point& original, Movement move) {
  switch (move) {
  case LEFT:
      return Point(original.x - 2, original.y);
    case RIGHT:
      return Point(original.x + 2, original.y);
    default:
      break;
  }
  return original;
}
}  // namespace

EndState Simulator::Run(Movement move) {
  history_.push_back(History());
  history_.back().actual_movement = Move(move, &history_.back());
  if (end_state_ == NOT_GAME_OVER) {
    Update(&history_.back());
  }
  return end_state_;
}

void Simulator::Undo() {
  if (history_.empty()) {
    return;
  }

  const History& history = history_.back();

  // Revert updated rocks.
  for (vector<pair<int, Point> >::const_reverse_iterator iter =
         history.updated_rocks.rbegin();
       iter != history.updated_rocks.rend(); ++iter) {
    Point& rock = rocks_[iter->first].first;
    field_[rock.y][rock.x] = ' ';
    field_[iter->second.y][iter->second.x] = '*';
    rock = iter->second;
  }

  // Special handling for lambda collection and rock movement.
  switch (history.prev_cell) {
  case '\\':
    // Close lift if necessary.
    field_[lift_.y][lift_.x] = 'L';
    --collected_lambda_;
    break;
  case '*': {
    // Revert the moved rock.
    Point moved;
    if (history.actual_movement == LEFT) {
      moved = Point(robot_.x - 1, robot_.y);
    } else {
      moved = Point(robot_.x + 1, robot_.y);
    }
    field_[moved.y][moved.x] = ' ';
    for (vector<std::pair<Point, Point> >::iterator iter = rocks_.begin();
         iter != rocks_.end(); ++iter) {
      if (iter->first == moved) {
        iter->first = robot_;
      }
    }
    break;
  }
  default:
    break;
  }

  // Revert movement.
  field_[robot_.y][robot_.x] = history.prev_cell;
  switch (history.actual_movement) {
  case LEFT:
    ++robot_.x;
    break;
  case RIGHT:
    --robot_.x;
    break;
  case UP:
    --robot_.y;
    break;
  case DOWN:
    ++robot_.y;
    break;
  case WAIT:
  case ABORT:
  default:
    // Do nothing.
    break;
  }
  field_[robot_.y][robot_.x] = 'R';
  end_state_ = NOT_GAME_OVER;
  current_water_count_ = history.water_count;

  history_.pop_back();
}

Movement Simulator::Move(Movement move, History *history) {
  history->movement = move;
  history->prev_cell = field_[robot_.y][robot_.x];

  switch (move) {
  case WAIT:
    return WAIT;
  case ABORT:
    end_state_ = ABORT_END;
    return ABORT;
  default:
    break;
    // TODO ERROR check.
  }

  Point next = GetNext(robot_, move);
  char old = field_[next.y][next.x];
  switch (old) {
  case '*':
  {
    Point next_next = GetSecondNext(robot_, move);
    if (field_[next_next.y][next_next.x] != ' ') {
      return WAIT;
    }
    // TODO Remove duplication of rocks and this loop.
    for (vector<pair<Point, Point> >::iterator iter = rocks_.begin();
         iter != rocks_.end(); ++iter) {
      if (iter->first == next) {
        iter->first = next_next;
      }
    }
    field_[next_next.y][next_next.x] = '*';
  }
  goto MOVE;
  case 'O':
    end_state_ = WIN_END;
    goto MOVE;
  case '\\':
    collected_lambda_ += 1;
    goto MOVE;
  case ' ':
  case '.':
  MOVE:
    field_[robot_.y][robot_.x] = ' ';
    field_[next.y][next.x] = 'R';
    robot_ = next;
    break;
  default:
    return WAIT;
  }

  history->prev_cell = old;
  return move;
}

void Simulator::Update(History* history) {
  if (end_state_ != NOT_GAME_OVER) {
    return;
  }

  // Update locks.
  for (vector<pair<Point, Point> >::iterator iter = rocks_.begin();
       iter != rocks_.end(); ++iter) {
    const Point& rock = iter->first;
    iter->second = iter->first;
    switch (field_[rock.y - 1][rock.x]) {
    case ' ':
      iter->second = Point(rock.x, rock.y - 1);
      break;
    case '*':
      if (field_[rock.y][rock.x + 1] == ' ' &&
          field_[rock.y - 1][rock.x + 1] == ' ') {
        iter->second = Point(rock.x + 1, rock.y - 1);
      } else if (field_[rock.y][rock.x - 1] == ' ' &&
                 field_[rock.y - 1][rock.x - 1] == ' ') {
        iter->second = Point(rock.x - 1, rock.y - 1);
      }
      break;
    case '\\':
      if (field_[rock.y][rock.x + 1] == ' ' &&
          field_[rock.y - 1][rock.x + 1] == ' ') {
        iter->second = Point(rock.x + 1, rock.y - 1);
      }
      break;
    default:
      break;
    }
  }

  for (vector<pair<Point, Point> >::iterator iter = rocks_.begin();
       iter != rocks_.end(); ++iter) {
    if (iter->first != iter->second) {
      history->updated_rocks.push_back(
        make_pair(distance(rocks_.begin(), iter), iter->first));
      field_[iter->first.y][iter->first.x] = ' ';
      field_[iter->second.y][iter->second.x] = '*';
      if (robot_.x == iter->second.x && robot_.y == iter->second.y - 1) {
        // 突然の死
        end_state_ = LOOSE_END;
      }
      iter->first = iter->second;
    }
  }

  // Update lift.
  if (collected_lambda_ == total_lambda_) {
    field_[lift_.y][lift_.x] = 'O';
  }

  // Update water count.
  if (initial_water_ >= 0) {
    if (robot_.y <
	  initial_water_ + static_cast<int>(history_.size()) / flooding_) {
      ++current_water_count_;
    } else {
      current_water_count_ = -1;
    }
    if (current_water_count_ > waterproof_) {
      // 突然の死
      end_state_ = LOOSE_END;
    }
  }
  history->water_count = current_water_count_;
}

int Simulator::GetScore() const {
  int score = 25 * collected_lambda_;
  switch (end_state_) {
  case WIN_END:
    score += 50 * collected_lambda_;
    break;
  case ABORT_END:
    score += 25 * collected_lambda_ + 1;
    break;
  default:
    break;
  }
  return score - history_.size();
}

string Simulator::DebugStringInternal(bool is_colored) const {
  string result;
  result.reserve((width_ + 1) * height_ + 12);
  int water =
      (initial_water_ < 0) ?
       0 :
       initial_water_ + static_cast<int>(history_.size()) / flooding_;
  for (vector<string>::const_reverse_iterator iter = field_.rbegin();
       iter != field_.rend(); ++iter) {
    if (is_colored && distance(field_.begin(), iter.base()) < water) {
      result.append("\x1b[1;34m");
    }
    result.append(*iter);
    result.push_back('\n');
  }
  if (is_colored) {
    result.append("\x1b[0m");
  }
  return result;
}

istream& operator>>(istream& is, Simulator &simulator) {
  vector<string> lines;
  string line;
  int width = 0;
  while (getline(is, line)) {
    if (line.empty()) {
      break;
    }
    lines.push_back(line);
    width = max(width, static_cast<int>(line.size()));
  }

  // Parse meta data.
  int water = -1, flooding = 0, waterproof = 0;
  while (getline(is, line)) {
    string key;
    istringstream s(line);
    s >> key;
    if (key == "Water") {
      s >> water;
    } else if (key == "Flooding") {
      s >> flooding;
    } else if (key == "Waterproof") {
      s >> waterproof;
    } else {
      cerr << "Unknown metadata: " << line;
    }
  }

  for (vector<string>::iterator iter = lines.begin();
       iter != lines.end(); ++iter) {
    iter->resize(width, ' ');
  }
  reverse(lines.begin(), lines.end());

  int height = lines.size();
  swap(simulator.field_, lines);
  simulator.width_ = width;
  simulator.height_ = height;
  simulator.total_lambda_ = 0;
  simulator.collected_lambda_ = 0;
  simulator.end_state_ = NOT_GAME_OVER;
  simulator.history_.clear();
  simulator.rocks_.clear();
  simulator.initial_water_ = water;
  simulator.flooding_ = flooding;
  simulator.waterproof_ = waterproof;
  simulator.current_water_count_ = 0;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      switch (simulator.field_[y][x]) {
      case 'R':
        simulator.robot_ = Point(x, y);
        break;
      case 'L':
        simulator.lift_ = Point(x, y);
        break;
      case '.':
        simulator.earths_.push_back(Point(x, y));
        break;
      case '\\':
        simulator.lambdas_.push_back(Point(x, y));
        ++simulator.total_lambda_;
        break;
      case '*':
        simulator.rocks_.push_back(make_pair(Point(x, y), Point()));
        break;
      }
    }
  }
  return is;
}
}  // namespace icfpc2012
