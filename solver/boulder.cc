#include "boulder.h"

#include <assert.h>
#include <stdio.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

namespace icfpc2012 {

////////////////////////////////////////////////////////////////////////////////
//  Basic data
////////////////////////////////////////////////////////////////////////////////

string EndStateToString(EndState end_state) {
  switch (end_state) {
    case NOT_GAME_OVER:
      return "NOT_GAME_OVER";
    case WIN_END:
      return "WIN_END";
    case LOSE_END:
      return "LOSE_END";
    case ABORT_END:
      return "ABORT_END";
  }
  abort();
  return "SUDDEN DEATH";
}

Movement MovementFromChar(char ch) {
  switch (ch) {
    case 'L':
      return LEFT;
    case 'R':
      return RIGHT;
    case 'U':
      return UP;
    case 'D':
      return DOWN;
    case 'S':
      return SHAVE;
    case 'W':
      return WAIT;
    case 'A':
      return ABORT;
  }
  abort();
  return ABORT;
}

char MovementToChar(Movement move) {
  switch (move) {
    case LEFT:
      return 'L';
    case RIGHT:
      return 'R';
    case UP:
      return 'U';
    case DOWN:
      return 'D';
    case SHAVE:
      return 'S';
    case WAIT:
      return 'W';
    case ABORT:
      return 'A';
  }
  abort();
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
//  Field
////////////////////////////////////////////////////////////////////////////////

static const Point kAround[] = {
  Point(-1, -1), Point(0, -1), Point(1, -1),
  Point(-1, 0), Point(1, 0),
  Point(-1, 1), Point(0, 1), Point(1, 1),
};

// static
Point Point::FromMovement(Movement move) {
  switch (move) {
    case LEFT:  return Point(-1, 0);
    case RIGHT: return Point(+1, 0);
    case DOWN:  return Point(0, -1);
    case UP:    return Point(0, +1);
    case SHAVE: return Point(0, 0);
    case WAIT:  return Point(0, 0);
    case ABORT: return Point(0, 0);
  }
  abort();
  return Point();
}

Field::Field() {
  Initialize(MatrixType());
}

void Field::Swap(Field& rhs) {
  matrix_.Swap(rhs.matrix_);
  swap(robot_, rhs.robot_);
  swap(lift_, rhs.lift_);
  swap(rock_list_, rhs.rock_list_);
  swap(higher_order_rock_list_, rhs.higher_order_rock_list_);
  swap(lambda_list_, rhs.lambda_list_);
  swap(wadler_list_, rhs.wadler_list_);
}

Field& Field::operator=(const Field& rhs) {
  Field tmp(rhs);
  Swap(tmp);
  return *this;
}

string Field::DebugString() const {
  string result;
  result.reserve((width() + 1) * height() + 1);
  for (int y = height() - 1; y >= 0; --y) {
    for (int x = 0; x < width(); ++x) {
      result.push_back(GetCell(x, y).type());
    }
    result.push_back('\n');
  }
  return result;
}

const Cell& Field::GetCell(int x, int y) const {
  static const Cell WALL_CELL(WALL);
  if (0 <= x && x < width() && 0 <= y && y < height()) {
    return matrix_.Get(x, y);
  }
  return WALL_CELL;
}

void Field::SetCell(int x, int y, const Cell& cell) {
  assert(0 <= x && x < width() && 0 <= y && y < height());
  matrix_.GetMutable(x, y) = cell;
}

Field::Field(const MatrixType& matrix) {
  Initialize(matrix);
}

void Field::Initialize(const MatrixType& matrix) {
  matrix_ = matrix;
  rock_list_.reset(new RockList);
  higher_order_rock_list_.reset(new RockList);
  lambda_list_.reset(new LambdaList);
  wadler_list_.reset(new WadlerList);

  int rock_id = 0;
  int higher_order_rock_id = 0;
  for (int y = 0; y < height(); ++y) {
    for (int x = 0; x < width(); ++x) {
      switch (GetCell(x, y).type()) {
        case LAMBDA:
          lambda_list_->push_back(Lambda(Point(x, y)));
          break;
        case HIGHER_ORDER_ROCK:
          higher_order_rock_list_->push_back(Rock(higher_order_rock_id++,
                                                  Point(x, y)));
          break;
        case ROCK:
          rock_list_->push_back(Rock(rock_id++, Point(x, y)));
          break;
        case ROBOT:
          robot_.set_position(Point(x, y));
          break;
        case CLOSED_LIFT:
        case OPEN_LIFT:
          lift_.set_position(Point(x, y));
          break;
        case WADLER:
          wadler_list_->push_back(Wadler(Point(x, y)));
          break;
      }
    }
  }
}

std::istream& operator>>(std::istream& input, Field& field) {
  vector<string> lines;
  while (input) {
    string line;
    getline(input, line);
    if (line.empty()) {
      break;
    }
    lines.push_back(line);
  }

  int height = lines.size();
  int width = 0;
  for (int i = 0; i < height; ++i) {
    width = max(width, static_cast<int>(lines[i].size()));
  }
  for (int i = 0; i < height; ++i) {
    lines[i].resize(width, EMPTY);
  }

  Field::MatrixType matrix(width, height);
  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      int i = height - 1 - y;
      int j = x;
      matrix.GetMutable(x, y) = Cell(lines[i][j]);
    }
  }

  field = Field(matrix);
  return input;
}

////////////////////////////////////////////////////////////////////////////////
//  GameState
////////////////////////////////////////////////////////////////////////////////

GameState::GameState()
    : step_(0), end_state_(NOT_GAME_OVER),
      lambda_collected_(0), water_count_(0) {
  ComputeScore();
}

GameState::GameState(
    std::tr1::shared_ptr<GameConfig> config,
    int step, const Field& field, EndState end_state, int lambda_collected,
    int water_count, int growth_rate, int num_razors)
    : config_(config),
      step_(step), field_(field), end_state_(end_state),
      lambda_collected_(lambda_collected), water_count_(water_count),
      growth_rate_(growth_rate), num_razors_(num_razors) {
  ComputeScore();
}

GameState& GameState::operator=(const GameState& rhs) {
  GameState tmp(rhs);
  Swap(tmp);
  return *this;
}

void GameState::Swap(GameState& rhs) {
  swap(config_, rhs.config_);

  swap(step_, rhs.step_);
  field_.Swap(rhs.field_);
  swap(end_state_, rhs.end_state_);
  swap(lambda_collected_, rhs.lambda_collected_);
  swap(water_count_, rhs.water_count_);
  swap(growth_rate_, rhs.growth_rate_);
  swap(num_razors_, rhs.num_razors_);
  swap(score_, rhs.score_);
}

void GameState::ComputeScore() {
  score_ = 0;
  score_ += lambda_collected_ * 25;
  if (end_state_ == WIN_END) {
    score_ += lambda_collected_ * 50;
  } else if (end_state_ == ABORT_END) {
    score_ += lambda_collected_ * 25;
  }
  score_ -= step_;
}

string GameState::DebugString(bool include_field) const {
  char status[1024];
  sprintf(status,
          "Step %d - Score %d - %s\n"
          "Water: %d / %d, Level %d\n"
          "Growth: %d / %d, Razor: %d\n",
          step(), score(), EndStateToString(end_state()).c_str(),
          water_count(), config().water_proof(), GetWaterLevel(),
          growth_rate(), config().growth(), num_razors());
  if (include_field) {
    return string(status) + field().DebugString();
  }
  return string(status);
}


////////////////////////////////////////////////////////////////////////////////
//  GameStateStack
////////////////////////////////////////////////////////////////////////////////

GameStateStack::GameStateStack() {
}

GameStateStack::GameStateStack(const GameState& initial_state)
    : cons_(new StateCons(initial_state)) {
}

GameStateStack::GameStateStack(const GameState& new_state, Movement move,
                               const GameStateStack& stack)
    : cons_(new StateCons(new_state, move, stack.cons_)) {
}

GameStateStack& GameStateStack::operator=(const GameStateStack& rhs) {
  GameStateStack tmp(rhs);
  Swap(tmp);
  return *this;
}

void GameStateStack::Swap(GameStateStack& rhs) {
  cons_.swap(rhs.cons_);
}

GameStateStack GameStateStack::Undo() const {
  if (cons_->parent.get() == NULL) {
    return *this;
  }
  return GameStateStack(cons_->parent);
}

string GameStateStack::GetCommand() const {
  vector<char> commands;
  for (const StateCons* cons = cons_.get();
       cons->parent.get() != NULL;
       cons = cons->parent.get()) {
    commands.push_back(MovementToChar(cons->move));
  }
  return string(commands.rbegin(), commands.rend());
}


////////////////////////////////////////////////////////////////////////////////
//  Simulator
////////////////////////////////////////////////////////////////////////////////

namespace {
Movement GetActualMovement(Movement move, const Field& field, int num_razors) {
  if (move == WAIT || move == ABORT) {
    return move;
  }

  if (move == SHAVE) {
    return num_razors > 0 ? move : WAIT;
  }

  const Point direction = Point::FromMovement(move);
  const Robot& robot = field.robot();

  Cell next_cell = field.GetCell(robot.position() + direction);
  switch (next_cell.type()) {
    case EMPTY:
    case EARTH:
    case LAMBDA:
    case OPEN_LIFT:
    case TRAMPOLINE_A:
    case TRAMPOLINE_B:
    case TRAMPOLINE_C:
    case TRAMPOLINE_D:
    case TRAMPOLINE_E:
    case TRAMPOLINE_F:
    case TRAMPOLINE_G:
    case TRAMPOLINE_H:
    case TRAMPOLINE_I:
    case RAZOR:
      return move;
    case ROCK:
    case HIGHER_ORDER_ROCK:
      // Processed below.
      break;
    case CLOSED_LIFT:
    case WALL:
    case TARGET_1:
    case TARGET_2:
    case TARGET_3:
    case TARGET_4:
    case TARGET_5:
    case TARGET_6:
    case TARGET_7:
    case TARGET_8:
    case TARGET_9:
    case WADLER:
      return WAIT;
    default:
      // TODO support trampoline, bread and shaver.
      abort();
  }

  // Here after, a rock should be located at the next position.
  if (move != LEFT && move != RIGHT) {
    return WAIT;
  }

  Cell next_next = field.GetCell(robot.position() + direction + direction);
  if (next_next.type() != EMPTY) {
    return WAIT;
  }

  return move;
}

struct WadlerPositionEq {
  WadlerPositionEq(const Point& position) : position_(position) {
  }

  bool operator()(const Wadler& wadler) const {
    return wadler.position() == position_;
  }

 private:
  Point position_;
};

// It is necessary to pass move which can be done. The check can be done in
// GetActualMovement above.
EndState MoveInternal(Movement move, const TrampolineMap& trampoline_map,
                      Field* field, int* lambda_collected, int* num_razors) {
  assert(field != NULL);
  assert(lambda_collected != NULL);
  assert(move != ABORT);
  if (move == WAIT) {
    return NOT_GAME_OVER;
  }

  if (move == SHAVE) {
    assert(*num_razors > 0);
    --*num_razors;
    for (int i = 0; i < 8; ++i) {
      Point position = field->robot().position() + kAround[i];
      if (field->GetCell(position).type() == WADLER) {
        field->SetCell(position, Cell(EMPTY));
        field->mutable_wadler_list()->erase(
            find_if(field->mutable_wadler_list()->begin(),
                    field->mutable_wadler_list()->end(),
                    WadlerPositionEq(position)));

      }
    }
    return NOT_GAME_OVER;
  }

  const Point direction = Point::FromMovement(move);
  Robot* robot = field->mutable_robot();
  Point next_position = robot->position() + direction;

  EndState end_state = NOT_GAME_OVER;
  CellType type = field->GetCell(next_position).type();
  switch (type) {
    case ROCK:
    case HIGHER_ORDER_ROCK: {
      const Point next_next_position =
          robot->position() + direction + direction;
      assert(move == LEFT || move == RIGHT);
      assert(field->GetCell(next_next_position).type() == EMPTY);
      field->SetCell(next_next_position, Cell(type));

      // Update rock list.
      if (type == ROCK) {
        RockList* rock_list = field->mutable_rock_list();
        for (int i = 0; i < static_cast<int>(rock_list->size()); ++i) {
          Rock* rock = &(*rock_list)[i];
          if (rock->position() == next_position) {
            rock->set_position(next_next_position);
            break;
          }
        }
      } else {
        assert(type == HIGHER_ORDER_ROCK);
        RockList* rock_list = field->mutable_higher_order_rock_list();
        for (int i = 0; i < static_cast<int>(rock_list->size()); ++i) {
          Rock* rock = &(*rock_list)[i];
          if (rock->position() == next_position) {
            rock->set_position(next_next_position);
            break;
          }
        }
      }
      break;
    }
    case LAMBDA:
      ++(*lambda_collected);
      break;
    case OPEN_LIFT:
      end_state = WIN_END;
      break;
    case RAZOR:
      ++*num_razors;
      break;

    case TRAMPOLINE_A:
    case TRAMPOLINE_B:
    case TRAMPOLINE_C:
    case TRAMPOLINE_D:
    case TRAMPOLINE_E:
    case TRAMPOLINE_F:
    case TRAMPOLINE_G:
    case TRAMPOLINE_H:
    case TRAMPOLINE_I: {
      CellType target = trampoline_map.GetTarget(type);
      // Jump to the target.
      next_position = trampoline_map.GetTargetPosition(target);
      // Destruct all trampolines which target |target|.
      for (CellType cell_type = TRAMPOLINE_A; cell_type <= TRAMPOLINE_I;
           ++cell_type) {
        if (trampoline_map.GetTarget(cell_type) == target) {
          field->SetCell(
              trampoline_map.GetTrampolinePosition(cell_type), Cell(EMPTY));
        }
      }
      break;
    }

    default:
      // DO NOTHING.
      break;
  }

  field->SetCell(robot->position(), Cell(EMPTY));
  field->SetCell(next_position, Cell(ROBOT));
  robot->set_position(next_position);

  return end_state;
}

struct ContainsId {
  ContainsId(const vector<int>& values) : values_(values) {
  }

  bool operator()(const Rock& rock) const {
    return binary_search(values_.begin(), values_.end(), rock.id());
  }

 private:
  const vector<int>& values_;
};

struct MovingRock {
  int index;
  bool is_higher_order;
  bool is_landing;
  Point position;

  MovingRock(int index,
             bool is_higher_order, bool is_landing,
             const Point& position)
      : index(index), is_higher_order(is_higher_order), is_landing(is_landing),
        position(position) {
  }
};

// X: (large -> small),  Y: (small -> large).
struct MovingPositionLess {
  bool operator()(const MovingRock& info1, const MovingRock& info2) const {
    return (info1.position.x != info2.position.x) ?
        (info1.position.x > info2.position.x) :
        (info1.position.y < info2.position.y);
  }
};

void GetMovingRockList(
    const Field& field,
    const RockList& rock_list,
    bool is_higher_order,
    vector<MovingRock>* moving_rock_list) {
  for (int i = 0; i < static_cast<int>(rock_list.size()); ++i) {
    const Rock& rock = rock_list[i];
    Point position = rock.position();
    Point below_position = position.Down();
    bool fall = false;
    Point fall_position;
    switch (field.GetCell(below_position).type()) {
      case EMPTY:
        fall = true;
        fall_position = below_position;
        break;
      case ROCK:
      case HIGHER_ORDER_ROCK:
        if (field.GetCell(position.Right()).type() == EMPTY &&
            field.GetCell(below_position.Right()).type() == EMPTY) {
          fall = true;
          fall_position = below_position.Right();
        } else if (
            field.GetCell(position.Left()).type() == EMPTY &&
            field.GetCell(below_position.Left()).type() == EMPTY) {
          fall = true;
          fall_position = below_position.Left();
        }
        break;
      case LAMBDA:
        if (field.GetCell(position.Right()).type() == EMPTY &&
            field.GetCell(below_position.Right()).type() == EMPTY) {
          fall = true;
          fall_position = below_position.Right();
        }
        break;
      default:
        // Do nothing.
        break;
    }
    if (fall) {
      bool is_landing =
          is_higher_order &&
          (field.GetCell(fall_position.Down()).type() != EMPTY);

      moving_rock_list->push_back(
          MovingRock(i, is_higher_order, is_landing, fall_position));
    }
  }
}

EndState UpdateInternal(int num_lambdas, int growth, int lambda_collected,
                        Field* field, int* growth_rate) {
  // Open the lift if necessary.
  if (lambda_collected == num_lambdas) {
    field->SetCell(field->lift().position(), Cell(OPEN_LIFT));
  }

  vector<MovingRock> moving_rock_list;
  moving_rock_list.reserve(
      field->rock_list().size() + field->higher_order_rock_list().size());
  GetMovingRockList(
      *field, field->rock_list(), false, &moving_rock_list);
  GetMovingRockList(
      *field, field->higher_order_rock_list(), true, &moving_rock_list);

  EndState end_state = NOT_GAME_OVER;
  if (!moving_rock_list.empty()) {
    Point robot_position = field->robot().position();
    vector<int> removed_rock_id_list;
    vector<int> removed_higher_order_rock_id_list;
    removed_rock_id_list.reserve(moving_rock_list.size());
    removed_higher_order_rock_id_list.reserve(moving_rock_list.size());

    // Need to process in the appropriate order to resolve crashing.
    sort(moving_rock_list.begin(), moving_rock_list.end(),
         MovingPositionLess());
    for (int i = 0; i < static_cast<int>(moving_rock_list.size()); ++i) {
      if (moving_rock_list[i].is_higher_order) {
        // Higher order rocks.
        Rock* rock = &(*field->mutable_higher_order_rock_list())[
            moving_rock_list[i].index];
        field->SetCell(rock->position(), Cell(EMPTY));
        if (field->GetCell(moving_rock_list[i].position).type() != EMPTY) {
          // Crashed.
          removed_higher_order_rock_id_list.push_back(rock->id());
        } else {
          if (moving_rock_list[i].is_landing) {
            // Landing. Thus gets lambda.
            removed_higher_order_rock_id_list.push_back(rock->id());
            field->SetCell(moving_rock_list[i].position, Cell(LAMBDA));
            field->mutable_lambda_list()->push_back(
                Lambda(moving_rock_list[i].position));
          } else {
            // Just falling.
            field->SetCell(
                moving_rock_list[i].position, Cell(HIGHER_ORDER_ROCK));
            rock->set_position(moving_rock_list[i].position);
          }
        }
      } else {
        // Simple rock.
        Rock* rock = &(*field->mutable_rock_list())[
            moving_rock_list[i].index];
        field->SetCell(rock->position(), Cell(EMPTY));
        if (field->GetCell(moving_rock_list[i].position).type() != EMPTY) {
          // Crashed.
          removed_rock_id_list.push_back(rock->id());
        } else {
          field->SetCell(moving_rock_list[i].position, Cell(ROCK));
          rock->set_position(moving_rock_list[i].position);
        }
      }

      if (robot_position == moving_rock_list[i].position.Down()) {
        // 突然の死
        end_state = LOSE_END;
      }
    }

    // Clean up crashed rocks.
    if (!removed_rock_id_list.empty()) {
      RockList* rock_list = field->mutable_rock_list();
      sort(removed_rock_id_list.begin(), removed_rock_id_list.end());
      rock_list->erase(
          remove_if(rock_list->begin(), rock_list->end(),
                    ContainsId(removed_rock_id_list)),
          rock_list->end());
    }

    if (!removed_higher_order_rock_id_list.empty()) {
      RockList* rock_list = field->mutable_higher_order_rock_list();
      sort(removed_higher_order_rock_id_list.begin(),
           removed_higher_order_rock_id_list.end());
      rock_list->erase(
          remove_if(rock_list->begin(), rock_list->end(),
                    ContainsId(removed_higher_order_rock_id_list)),
          rock_list->end());
    }
  }

  // Update wadler.
  if (*growth_rate == 0) {
    // Growth walders.
    // mutable_wadler_list inside the inner-for-loop may new another
    // WadlerList, but it won't destroy the older one, so this approch should
    // be fine.
    // If hte new wadler list is NOT created, the contents of |wadler_list|
    // will be updated by push_back. So we access the list in the reverse order
    // and use index for the loop iterator, not vector::iterator families.
    const WadlerList& wadler_list = field->wadler_list();
    for (int i = static_cast<int>(wadler_list.size()) - 1;
         i >= 0; --i) {
      const Point wadler_position = wadler_list[i].position();
      for (int j = 0; j < 8; ++j) {
        const Point position = wadler_position + kAround[j];
        if (field->GetCell(position).type() == EMPTY) {
          field->SetCell(position, Cell(WADLER));
          field->mutable_wadler_list()->push_back(Wadler(position));
        }
      }
    }
    *growth_rate = growth - 1;
  }
  --*growth_rate;

  return end_state;
}

EndState UpdateWater(
    int water_level, int water_proof, const Field& field, int* water_count) {
  if (field.robot().position().y < water_level) {
    ++(*water_count);
  } else {
    *water_count = 0;
  }

  return (*water_count > water_proof) ? LOSE_END : NOT_GAME_OVER;
}
}  // namespace

bool IsMovable(const GameState& current_state, Movement move) {
  if (current_state.end_state() != NOT_GAME_OVER) {
    return false;
  }

  if (current_state.step() ==
      current_state.field().width() * current_state.field().height()) {
    return false;
  }

  return GetActualMovement(
      move, current_state.field(), current_state.num_razors()) == move;
}

GameState Simulate(const GameState& current_state, Movement move,
                   bool ignore_step_exceeding) {
  if (current_state.end_state() != NOT_GAME_OVER) {
    return current_state;
  }

  const GameConfig& config = current_state.config();

  // Limit the search length.
  if (!ignore_step_exceeding &&
      (current_state.step() ==
       current_state.field().width() * current_state.field().height())) {
    // 突然の死
    return GameState(current_state.config_ptr(),
                     current_state.step(), current_state.field(),
                     ABORT_END, current_state.lambda_collected(),
                     current_state.water_count(),
                     current_state.growth_rate(),
                     current_state.num_razors());
  }

  int num_razors = current_state.num_razors();
  Movement actual_movement = GetActualMovement(
      move, current_state.field(), num_razors);
  if (actual_movement == ABORT) {
    return GameState(current_state.config_ptr(),
                     current_state.step(), current_state.field(),
                     ABORT_END, current_state.lambda_collected(),
                     current_state.water_count(),
                     current_state.growth_rate(), num_razors);
  }

  Field next_field(current_state.field());
  int lambda_collected = current_state.lambda_collected();
  EndState end_state = MoveInternal(
      actual_movement, config.trampoline_map(),
      &next_field, &lambda_collected, &num_razors);
  int water_count = current_state.water_count();
  int growth_rate = current_state.growth_rate();
  if (end_state == NOT_GAME_OVER) {
    end_state = UpdateInternal(
        config.num_lambdas(), config.growth(), lambda_collected,
        &next_field, &growth_rate);
    EndState end_state2 = UpdateWater(
        current_state.GetWaterLevel(), config.water_proof(), next_field,
        &water_count);
    if (end_state == NOT_GAME_OVER) {
      end_state = end_state2;
    }
  }

  return GameState(
      current_state.config_ptr(), current_state.step() + 1,
      next_field, end_state, lambda_collected, water_count,
      growth_rate, num_razors);
}

GameStateStack Simulate(const GameStateStack& current_stack, Movement move) {
  return GameStateStack(Simulate(current_stack.current_state(), move),
                        move, current_stack);
}

std::istream& operator>>(std::istream& input, GameState& game_state) {
  Field field;
  input >> field;

  int water = 0;
  int flooding = 0;
  int water_proof = 10;

  TrampolineMap trampoline_map;

  int growth = 25;
  int num_razors = 0;

  string line;
  while (getline(input, line)) {
    string key;
    istringstream is(line);
    is >> key;
    if (key == "Water") {
      is >> water;
      continue;
    }
    if (key == "Flooding") {
      is >> flooding;
      continue;
    }
    if (key == "Waterproof") {
      is >> water_proof;
      continue;
    }
    if (key == "Trampoline") {
      string trampoline;
      string target;
      is >> trampoline;
      is >> target;  // dummy
      is >> target;
      assert(trampoline.size() == 1);
      assert(target.size() == 1);
      trampoline_map.SetTarget(trampoline[0], target[0]);
      continue;
    }
    if (key == "Growth") {
      is >> growth;
      continue;
    }
    if (key == "Razors") {
      is >> num_razors;
      continue;
    }
  }

  for (int y = 0; y < field.height(); ++y) {
    for (int x = 0; x < field.width(); ++x) {
      CellType type = field.GetCell(x, y).type();
      switch (type) {
        case TRAMPOLINE_A:
        case TRAMPOLINE_B:
        case TRAMPOLINE_C:
        case TRAMPOLINE_D:
        case TRAMPOLINE_E:
        case TRAMPOLINE_F:
        case TRAMPOLINE_G:
        case TRAMPOLINE_H:
        case TRAMPOLINE_I:
          trampoline_map.SetTrampolinePosition(type, Point(x, y));
          break;
        case TARGET_1:
        case TARGET_2:
        case TARGET_3:
        case TARGET_4:
        case TARGET_5:
        case TARGET_6:
        case TARGET_7:
        case TARGET_8:
        case TARGET_9:
          trampoline_map.SetTargetPosition(type, Point(x, y));
          break;
      }
    }
  }

  game_state = GameState(
      std::tr1::shared_ptr<GameConfig>(new GameConfig(
          static_cast<int>(
              field.lambda_list().size() +
              field.higher_order_rock_list().size()),
          water, flooding, water_proof, trampoline_map, growth)),
      0, field, NOT_GAME_OVER, 0, 0, growth - 1, num_razors);

  return input;
}

}  // namespace icfpc2012
