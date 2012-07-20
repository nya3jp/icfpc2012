#ifndef ICFPC2012_BOULDER_H_
#define ICFPC2012_BOULDER_H_

#include <assert.h>

#include <algorithm>
#include <string>
#include <tr1/memory>
#include <vector>

namespace icfpc2012 {
typedef char CellType;
static const CellType ROBOT = 'R';
static const CellType WALL = '#';
static const CellType ROCK = '*';
static const CellType LAMBDA = '\\';
static const CellType CLOSED_LIFT = 'L';
static const CellType OPEN_LIFT = 'O';
static const CellType EARTH = '.';
static const CellType EMPTY = ' ';
static const CellType TRAMPOLINE_A = 'A';
static const CellType TRAMPOLINE_B = 'B';
static const CellType TRAMPOLINE_C = 'C';
static const CellType TRAMPOLINE_D = 'D';
static const CellType TRAMPOLINE_E = 'E';
static const CellType TRAMPOLINE_F = 'F';
static const CellType TRAMPOLINE_G = 'G';
static const CellType TRAMPOLINE_H = 'H';
static const CellType TRAMPOLINE_I = 'I';
static const CellType TARGET_1 = '1';
static const CellType TARGET_2 = '2';
static const CellType TARGET_3 = '3';
static const CellType TARGET_4 = '4';
static const CellType TARGET_5 = '5';
static const CellType TARGET_6 = '6';
static const CellType TARGET_7 = '7';
static const CellType TARGET_8 = '8';
static const CellType TARGET_9 = '9';
static const CellType WADLER = 'W';
static const CellType RAZOR = '!';
static const CellType HIGHER_ORDER_ROCK = '@';

enum EndState {
  NOT_GAME_OVER,
  WIN_END,
  LOSE_END,
  ABORT_END,
};

std::string EndStateToString(EndState end_state);

enum Movement {
  LEFT = 0,
  RIGHT,
  UP,
  DOWN,
  SHAVE,
  WAIT,
  ABORT,
};

static const Movement MOVEMENTS[] = {
  LEFT, RIGHT, UP, DOWN, SHAVE, WAIT, ABORT,
};
static const int NUM_MOVEMENTS = sizeof(MOVEMENTS) / sizeof(MOVEMENTS[0]);

Movement MovementFromChar(char ch);
char MovementToChar(Movement move);

struct Point {
  int x;
  int y;
  Point() : x(0), y(0) {}
  explicit Point(int x, int y) : x(x), y(y) {}

  static Point FromMovement(Movement move);
  Point operator+(const Point& other) const {
    return Point(x + other.x, y + other.y);
  }
  Point Left() const { return Point(x - 1, y); }
  Point Right() const { return Point(x + 1, y); }
  Point Up() const { return Point(x, y + 1); }
  Point Down() const { return Point(x, y - 1); }

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

template<class T>
class CowArray {
 public:
  CowArray() : size_(0) {}
  explicit CowArray(int size, const T& def = T()) {
    size_ = size;
    if (size_ > 0) {
      int left_size = (size - 1) / 2;
      int right_size = size - 1 - left_size;
      value_.reset(new T(def));
      if (left_size > 0) {
        left_.reset(new CowArray<T>(left_size, def));
      }
      if (right_size > 0) {
        right_.reset(new CowArray<T>(right_size, def));
      }
    }
  }
  CowArray<T>& operator=(const CowArray<T>& rhs) {
    CowArray<T> tmp(rhs);
    Swap(tmp);
    return *this;
  }
  void Swap(CowArray<T>& rhs) {
    value_.swap(rhs.value_);
    left_.swap(rhs.left_);
    right_.swap(rhs.right_);
    std::swap(size_, rhs.size_);
  }
  ~CowArray() {}

  int size() const {
    return size_;
  }

  const T& Get(int index) const {
    const int left_size = (left_.get() == NULL ? 0 : left_->size());
    if (index == left_size) {
      return *value_;
    } else if (index < left_size) {
      return left_->Get(index);
    } else {
      return right_->Get(index - left_size - 1);
    }
  }
  T& GetMutable(int index) {
    const int left_size = (left_.get() == NULL ? 0 : left_->size());
    if (index == left_size) {
      if (!value_.unique()) {
        value_.reset(new T(*value_));
      }
      return *value_;
    } else if (index < left_size) {
      if (!left_.unique()) {
        left_.reset(new CowArray<T>(*left_));
      }
      return left_->GetMutable(index);
    } else {
      if (!right_.unique()) {
        right_.reset(new CowArray<T>(*right_));
      }
      return right_->GetMutable(index - 1 - left_size);
    }
  }

 private:
  std::tr1::shared_ptr<T> value_;
  std::tr1::shared_ptr< CowArray<T> > left_;
  std::tr1::shared_ptr< CowArray<T> > right_;
  int size_;
};

template<class T, int CHUNK_SIZE = 4>
class ChunkedCowMatrix {
 public:
  ChunkedCowMatrix() {
    Initialize(0, 0, T());
  }
  explicit ChunkedCowMatrix(int width, int height, const T& def = T()) {
    Initialize(width, height, def);
  }
  ChunkedCowMatrix(const ChunkedCowMatrix<T>& rhs)
      : width_(rhs.width_), height_(rhs.height_),
        chunk_stride_(rhs.chunk_stride_), array_(rhs.array_),
        cached_chunk_index_(-1), cached_chunk_(NULL) {
  }
  ChunkedCowMatrix<T>& operator=(const ChunkedCowMatrix<T>& rhs) {
    ChunkedCowMatrix<T> tmp(rhs);
    Swap(tmp);
    return *this;
  }
  void Swap(ChunkedCowMatrix<T>& rhs) {
    std::swap(width_, rhs.width_);
    std::swap(height_, rhs.height_);
    std::swap(chunk_stride_, rhs.chunk_stride_);
    array_.Swap(rhs.array_);
    std::swap(cached_chunk_index_, rhs.cached_chunk_index_);
    std::swap(cached_chunk_, rhs.cached_chunk_);
  }
  ~ChunkedCowMatrix() {}

  int width() const {
    return width_;
  }
  int height() const {
    return height_;
  }

  const T& Get(int x, int y) const {
    int chunk_index, submatrix_x, submatrix_y;
    GetChunkPosition(x, y, &chunk_index, &submatrix_x, &submatrix_y);
    if (chunk_index != cached_chunk_index_) {
      cached_chunk_index_ = chunk_index;
      cached_chunk_ = &array_.Get(chunk_index);
    }
    return cached_chunk_->submatrix[submatrix_x][submatrix_y];
  }
  T& GetMutable(int x, int y) {
    int chunk_index, submatrix_x, submatrix_y;
    GetChunkPosition(x, y, &chunk_index, &submatrix_x, &submatrix_y);
    cached_chunk_index_ = -1;
    cached_chunk_ = NULL;
    return array_.GetMutable(chunk_index).submatrix[submatrix_x][submatrix_y];
  }

 private:
  struct MatrixChunk {
    T submatrix[CHUNK_SIZE][CHUNK_SIZE];
    MatrixChunk(const T& def = T()) {
      for (int i = 0; i < CHUNK_SIZE; ++i) {
        for (int j = 0; j < CHUNK_SIZE; ++j) {
          submatrix[i][j] = def;
        }
      }
    }
  };

  void Initialize(int width, int height, const T& def) {
    width_ = width;
    height_ = height;
    chunk_stride_ = (width + CHUNK_SIZE - 1) / CHUNK_SIZE;
    int total_chunks = chunk_stride_ * (height + CHUNK_SIZE - 1) / CHUNK_SIZE;
    array_ = CowArray<MatrixChunk>(total_chunks);

    cached_chunk_index_ = -1;
    cached_chunk_ = NULL;
  }

  void GetChunkPosition(
      int x, int y,
      int* chunk_index, int* submatrix_x, int* submatrix_y) const {
    int chunk_x = x / CHUNK_SIZE;
    int chunk_y = y / CHUNK_SIZE;
    *chunk_index = chunk_x + chunk_y * chunk_stride_;
    *submatrix_x = x - chunk_x * CHUNK_SIZE;
    *submatrix_y = y - chunk_y * CHUNK_SIZE;
  }

  int width_;
  int height_;
  int chunk_stride_;
  CowArray<MatrixChunk> array_;

  mutable int cached_chunk_index_;
  mutable const MatrixChunk* cached_chunk_;
};

// Copyable immutable cell representation.
class Cell {
 public:
  explicit Cell(CellType type = EMPTY) : type_(type) {}
  Cell& operator=(const Cell& rhs) {
    Cell tmp(rhs);
    Swap(tmp);
    return *this;
  }
  void Swap(Cell& rhs) {
    std::swap(type_, rhs.type_);
  }
  CellType type() const {
    return type_;
  }

 private:
  CellType type_;
};

class Robot {
 public:
  Robot() {
  }
  const Point& position() const {
    return position_;
  }
  void set_position(const Point& position) {
    position_ = position;
  }

 private:
  Point position_;
};

class Lift {
 public:
  Lift() {
  }
  const Point& position() const {
    return position_;
  }
  void set_position(const Point& position) {
    position_ = position;
  }

 private:
  Point position_;
};

class Rock {
 public:
  Rock() : id_(-1) {}
  explicit Rock(int id) : id_(id) {
  }
  Rock(int id, const Point& position) : id_(id), position_(position) {
  }
  const int id() const {
    return id_;
  }
  const Point& position() const {
    return position_;
  }
  void set_position(const Point& position) {
    position_ = position;
  }

 private:
  int id_;
  Point position_;
};
// May changed some how.
typedef std::vector<Rock> RockList;

class Lambda {
 public:
  Lambda() {
  }
  explicit Lambda(const Point& position) : position_(position) {
  }
  const Point& position() const {
    return position_;
  }
  void set_position(const Point& position) {
    position_ = position;
  }

 private:
  Point position_;
};
// Maybe changed in future.
typedef std::vector<Lambda> LambdaList;

class Wadler {
 public:
  Wadler() {
  }
  explicit Wadler(const Point& position) : position_(position) {
  }
  const Point& position() const {
    return position_;
  }

 private:
  Point position_;
};
typedef std::vector<Wadler> WadlerList;

// Copyable mutable field representation.
class Field {
 public:
  ~Field() {}
  Field();
  Field& operator=(const Field& rhs);
  void Swap(Field& rhs);

  std::string DebugString() const ;

  const Cell& GetCell(int x, int y) const;
  const Cell& GetCell(const Point& p) const {
    return GetCell(p.x, p.y);
  }
  void SetCell(int x, int y, const Cell& cell);
  void SetCell(const Point& p, const Cell& cell) {
    return SetCell(p.x, p.y, cell);
  }

  int width() const {
    return matrix_.width();
  }
  int height() const {
    return matrix_.height();
  }

  const Robot& robot() const {
    return robot_;
  }
  Robot* mutable_robot() {
    return &robot_;
  }
  const Lift& lift() const {
    return lift_;
  }

  const RockList& rock_list() const {
    return *rock_list_;
  }
  RockList* mutable_rock_list() {
    if (!rock_list_.unique()) {
      rock_list_.reset(new RockList(rock_list()));
    }
    return rock_list_.get();
  }

  const RockList& higher_order_rock_list() const {
    return *higher_order_rock_list_;
  }
  RockList* mutable_higher_order_rock_list() {
    if (!higher_order_rock_list_.unique()) {
      higher_order_rock_list_.reset(new RockList(higher_order_rock_list()));
    }
    return higher_order_rock_list_.get();
  }

  const LambdaList& lambda_list() const {
    return *lambda_list_;
  }
  LambdaList* mutable_lambda_list() {
    if (!lambda_list_.unique()) {
      lambda_list_.reset(new LambdaList(lambda_list()));
    }
    return lambda_list_.get();
  }

  const WadlerList& wadler_list() const {
    return *wadler_list_;
  }
  WadlerList* mutable_wadler_list() {
    if (!wadler_list_.unique()) {
      wadler_list_.reset(new WadlerList(wadler_list()));
    }
    return wadler_list_.get();
  }

 private:
  friend std::istream& operator>>(std::istream& input, Field& field);

  // Takes ownership of matrix.
  typedef ChunkedCowMatrix<Cell> MatrixType;
  explicit Field(const MatrixType& matrix);
  void Initialize(const MatrixType& matrix);

  MatrixType matrix_;

  Robot robot_;
  Lift lift_;
  std::tr1::shared_ptr<RockList> rock_list_;
  std::tr1::shared_ptr<RockList> higher_order_rock_list_;
  std::tr1::shared_ptr<LambdaList> lambda_list_;
  std::tr1::shared_ptr<WadlerList> wadler_list_;
};
std::istream& operator>>(std::istream& input, Field& field);

class TrampolineMap {
 public:
  TrampolineMap() {
  }

  CellType GetTarget(CellType type) const {
    assert(TRAMPOLINE_A <= type && type <= TRAMPOLINE_I);
    return mapping_table_[type - TRAMPOLINE_A];
  }

  const Point& GetTrampolinePosition(CellType type) const {
    assert(TRAMPOLINE_A <= type && type <= TRAMPOLINE_I);
    return trampoline_position_table_[type - TRAMPOLINE_A];
  }

  const Point& GetTargetPosition(CellType type) const {
    assert(TARGET_1 <= type && type <= TARGET_9);
    return target_position_table_[type - TARGET_1];
  }

  void SetTarget(CellType trampoline, CellType target) {
    assert(TRAMPOLINE_A <= trampoline && trampoline <= TRAMPOLINE_I);
    assert(TARGET_1 <= target && target <= TARGET_9);
    mapping_table_[trampoline - TRAMPOLINE_A] = target;
  }

  void SetTrampolinePosition(CellType trampoline, const Point& position) {
    assert(TRAMPOLINE_A <= trampoline && trampoline <= TRAMPOLINE_I);
    trampoline_position_table_[trampoline - TRAMPOLINE_A] = position;
  }

  void SetTargetPosition(CellType target, const Point& position) {
    assert(TARGET_1 <= target && target <= TARGET_9);
    target_position_table_[target - TARGET_1] = position;
  }

 private:
  Point trampoline_position_table_[9];
  Point target_position_table_[9];
  CellType mapping_table_[9];
};

// Parameter and Data which are not changed during game simulation steps.
class GameConfig {
 public:
  GameConfig(int num_lambdas, int water, int flooding, int water_proof,
             const TrampolineMap& trampoline_map,
             int growth)
      : num_lambdas_(num_lambdas),
        water_(water), flooding_(flooding), water_proof_(water_proof),
        trampoline_map_(trampoline_map), growth_(growth) {
  }

  int num_lambdas() const { return num_lambdas_; }

  int water() const { return water_; }
  int flooding() const { return flooding_; }
  int water_proof() const { return water_proof_; }

  const TrampolineMap& trampoline_map() const { return trampoline_map_; }

  int growth() const { return growth_; }

 private:
  int num_lambdas_;

  int water_;
  int flooding_;
  int water_proof_;

  TrampolineMap trampoline_map_;

  int growth_;

  // The parameter should not be copied.
  // Disallow copy and assign to trapping programming errors in case.
  GameConfig(const GameConfig&);
  void operator=(const GameConfig&);
};

// Copyable immutable game state representation.
class GameState {
 public:
  GameState();
  GameState(
      std::tr1::shared_ptr<GameConfig> config,
      int step, const Field& field, EndState end_state, int lambda_collected,
      int water_count, int growth_rate, int num_razors);
  ~GameState() {}
  GameState& operator=(const GameState& rhs);
  void Swap(GameState& rhs);

  const GameConfig& config() const {
    return *config_;
  }

  std::tr1::shared_ptr<GameConfig> config_ptr() const {
    return config_;
  }

  int step() const {
    return step_;
  }
  const Field& field() const {
    return field_;
  }
  EndState end_state() const {
    return end_state_;
  }
  int lambda_collected() const {
    return lambda_collected_;
  }
  int water_count() const {
    return water_count_;
  }
  int score() const {
    return score_;
  }
  int growth_rate() const {
    return growth_rate_;
  }
  int num_razors() const {
    return num_razors_;
  }
  int GetWaterLevel() const {
    if (config_->flooding() == 0) {
      return config_->water();
    }
    return config_->water() + step_ / config_->flooding();
  }

  std::string DebugString(bool include_field = true) const;

 private:
  void ComputeScore();

  std::tr1::shared_ptr<GameConfig> config_;

  int step_;
  Field field_;
  EndState end_state_;
  int lambda_collected_;
  int water_count_;
  int growth_rate_;  // For ひげ
  int num_razors_;

  int score_;
};
std::istream& operator>>(std::istream& input, GameState& game_state);

class GameStateStack {
 public:
  GameStateStack();
  explicit GameStateStack(const GameState& initial_state);
  explicit GameStateStack(const GameState& new_state, Movement move,
                          const GameStateStack& stack);
  GameStateStack& operator=(const GameStateStack& rhs);
  void Swap(GameStateStack& rhs);
  ~GameStateStack() {}

  const GameState& current_state() const {
    return cons_->state;
  }
  GameStateStack Undo() const;
  std::string GetCommand() const;

 private:
  struct StateCons {
    GameState state;
    Movement move;
    std::tr1::shared_ptr<const StateCons> parent;
    StateCons() {}
    explicit StateCons(const GameState& state) : state(state) {}
    explicit StateCons(const GameState& state,
                       Movement move,
                       const std::tr1::shared_ptr<const StateCons>& parent)
        : state(state), move(move), parent(parent) {}
  };

  explicit GameStateStack(const std::tr1::shared_ptr<const StateCons>& cons)
      : cons_(cons) {}

  std::tr1::shared_ptr<const StateCons> cons_;
};

bool IsMovable(const GameState& state, Movement move);
GameState Simulate(const GameState& current_state, Movement move,
                   bool ignore_step_exceeding = false);
GameStateStack Simulate(const GameStateStack& current_stack, Movement move);

}  // namespace icfpc2012

#endif  // ICFPC2012_BOULDER_H_
