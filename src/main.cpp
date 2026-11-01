#ifdef CHESS_WEB_BUILD
#include <emscripten/emscripten.h>
#else
#include <SFML/Graphics.hpp>
#endif
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
constexpr int EMPTY = 0, PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5,
              KING = 6;
constexpr int WHITE = 0, BLACK = 1;
constexpr int WK = 1, WQ = 2, BK = 4, BQ = 8;

int sideOf(int p) {
  return p > 0 ? WHITE : BLACK;
}
int kindOf(int p) {
  return std::abs(p);
}
int fileOf(int s) {
  return s & 7;
}
int rankOf(int s) {
  return s >> 3;
}
bool inside(int f, int r) {
  return f >= 0 && f < 8 && r >= 0 && r < 8;
}
int square(int f, int r) {
  return r * 8 + f;
}
std::string nameOf(int s) {
  return std::string(1, char('a' + fileOf(s))) + char('1' + rankOf(s));
}
int parseSquare(const std::string& s) {
  if (s.size() != 2 || s[0] < 'a' || s[0] > 'h' || s[1] < '1' || s[1] > '8')
    return -1;
  return square(s[0] - 'a', s[1] - '1');
}
char pieceChar(int p) {
  const char chars[] = " pnbrqk";
  char c = chars[kindOf(p)];
  return p < 0 ? char(std::toupper(c)) : c;
}

struct Move {
  int from = -1, to = -1, promotion = 0;
  bool ep = false, castle = false;
  bool operator==(const Move& b) const {
    return from == b.from && to == b.to && promotion == b.promotion && ep == b.ep &&
           castle == b.castle;
  }
};

struct Board {
  std::array<int, 64> sq{};
  int turn = WHITE, castling = WK | WQ | BK | BQ, ep = -1, halfmove = 0, fullmove = 1;

  static Board start() {
    Board b;
    const int back[8] = {ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK};
    for (int f = 0; f < 8; ++f) {
      b.sq[square(f, 0)] = back[f];
      b.sq[square(f, 1)] = PAWN;
      b.sq[square(f, 6)] = -PAWN;
      b.sq[square(f, 7)] = -back[f];
    }
    return b;
  }
  int kingSquare(int side) const {
    for (int s = 0; s < 64; ++s)
      if (sq[s] == (side == WHITE ? KING : -KING))
        return s;
    return -1;
  }
  bool attacked(int target, int by) const {
    int tf = fileOf(target), tr = rankOf(target);
    int pawnRank = tr + (by == WHITE ? -1 : 1);
    for (int df : {-1, 1})
      if (inside(tf + df, pawnRank) &&
          sq[square(tf + df, pawnRank)] == (by == WHITE ? PAWN : -PAWN))
        return true;
    constexpr int knight[8][2] = {{1, 2},   {2, 1},   {2, -1}, {1, -2},
                                  {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}};
    for (auto& d : knight)
      if (inside(tf + d[0], tr + d[1]) &&
          sq[square(tf + d[0], tr + d[1])] == (by == WHITE ? KNIGHT : -KNIGHT))
        return true;
    for (int df = -1; df <= 1; ++df)
      for (int dr = -1; dr <= 1; ++dr)
        if (df || dr)
          if (inside(tf + df, tr + dr) &&
              sq[square(tf + df, tr + dr)] == (by == WHITE ? KING : -KING))
            return true;
    for (auto [df, dr] : std::array<std::pair<int, int>, 8>{
             {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}}}) {
      int f = tf + df, r = tr + dr, dist = 1;
      while (inside(f, r)) {
        int p = sq[square(f, r)];
        if (p) {
          if (sideOf(p) == by) {
            int k = kindOf(p);
            if (k == QUEEN || (dist == 1 && k == KING) ||
                (df == 0 || dr == 0 ? k == ROOK : k == BISHOP))
              return true;
          }
          break;
        }
        f += df;
        r += dr;
        ++dist;
      }
    }
    return false;
  }
  bool inCheck(int side) const {
    int k = kingSquare(side);
    return k < 0 || attacked(k, 1 - side);
  }
  void pseudo(std::vector<Move>& out) const {
    out.clear();
    const int dir = turn == WHITE ? 1 : -1;
    auto push = [&](int from, int to, int promo = 0, bool epMove = false,
                    bool castleMove = false) {
      if (!inside(fileOf(to), rankOf(to)))
        return;
      if (sq[to] && sideOf(sq[to]) == turn)
        return;
      if (kindOf(sq[to]) == KING)
        return;
      out.push_back({from, to, promo, epMove, castleMove});
    };
    for (int from = 0; from < 64; ++from) {
      int p = sq[from];
      if (!p || sideOf(p) != turn)
        continue;
      int f = fileOf(from), r = rankOf(from), k = kindOf(p);
      if (k == PAWN) {
        int nr = r + dir;
        if (inside(f, nr) && sq[square(f, nr)] == EMPTY) {
          if (nr == 0 || nr == 7)
            for (int promo : {QUEEN, ROOK, BISHOP, KNIGHT})
              push(from, square(f, nr), promo);
          else {
            push(from, square(f, nr));
            if (r == (turn == WHITE ? 1 : 6) && sq[square(f, r + 2 * dir)] == EMPTY)
              push(from, square(f, r + 2 * dir));
          }
        }
        for (int df : {-1, 1})
          if (inside(f + df, nr)) {
            int to = square(f + df, nr);
            if (sq[to] && sideOf(sq[to]) != turn) {
              if (nr == 0 || nr == 7)
                for (int promo : {QUEEN, ROOK, BISHOP, KNIGHT}) push(from, to, promo);
              else
                push(from, to);
            } else if (to == ep)
              push(from, to, 0, true);
          }
      } else if (k == KNIGHT) {
        constexpr int ds[8][2] = {{1, 2},   {2, 1},   {2, -1}, {1, -2},
                                  {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}};
        for (auto& d : ds)
          if (inside(f + d[0], r + d[1]))
            push(from, square(f + d[0], r + d[1]));
      } else if (k == KING) {
        for (int df = -1; df <= 1; ++df)
          for (int dr = -1; dr <= 1; ++dr)
            if ((df || dr) && inside(f + df, r + dr))
              push(from, square(f + df, r + dr));
        if (turn == WHITE && from == 4 && !inCheck(WHITE)) {
          if ((castling & WK) && sq[5] == 0 && sq[6] == 0 && sq[7] == ROOK &&
              !attacked(5, BLACK) && !attacked(6, BLACK))
            out.push_back({4, 6, 0, false, true});
          if ((castling & WQ) && sq[1] == 0 && sq[2] == 0 && sq[3] == 0 &&
              sq[0] == ROOK && !attacked(3, BLACK) && !attacked(2, BLACK))
            out.push_back({4, 2, 0, false, true});
        }
        if (turn == BLACK && from == 60 && !inCheck(BLACK)) {
          if ((castling & BK) && sq[61] == 0 && sq[62] == 0 && sq[63] == -ROOK &&
              !attacked(61, WHITE) && !attacked(62, WHITE))
            out.push_back({60, 62, 0, false, true});
          if ((castling & BQ) && sq[57] == 0 && sq[58] == 0 && sq[59] == 0 &&
              sq[56] == -ROOK && !attacked(59, WHITE) && !attacked(58, WHITE))
            out.push_back({60, 58, 0, false, true});
        }
      } else {
        for (auto [df, dr] : std::array<std::pair<int, int>, 8>{{{1, 0},
                                                                 {-1, 0},
                                                                 {0, 1},
                                                                 {0, -1},
                                                                 {1, 1},
                                                                 {1, -1},
                                                                 {-1, 1},
                                                                 {-1, -1}}}) {
          if ((k == BISHOP && (df == 0 || dr == 0)) ||
              (k == ROOK && (df != 0 && dr != 0)))
            continue;
          int x = f + df, y = r + dr;
          while (inside(x, y)) {
            int to = square(x, y);
            if (sq[to]) {
              if (sideOf(sq[to]) != turn)
                push(from, to);
              break;
            }
            push(from, to);
            x += df;
            y += dr;
          }
        }
      }
    }
  }
  void apply(const Move& m) {
    int moving = sq[m.from],
        captured = m.ep ? sq[m.to + (turn == WHITE ? -8 : 8)] : sq[m.to];
    sq[m.from] = 0;
    if (m.ep)
      sq[m.to + (turn == WHITE ? -8 : 8)] = 0;
    sq[m.to] = m.promotion ? (turn == WHITE ? m.promotion : -m.promotion) : moving;
    if (m.castle) {
      if (m.to == 6) {
        sq[5] = sq[7];
        sq[7] = 0;
      } else if (m.to == 2) {
        sq[3] = sq[0];
        sq[0] = 0;
      } else if (m.to == 62) {
        sq[61] = sq[63];
        sq[63] = 0;
      } else {
        sq[59] = sq[56];
        sq[56] = 0;
      }
    }
    if (kindOf(moving) == KING)
      castling &= turn == WHITE ? ~(WK | WQ) : ~(BK | BQ);
    if (m.from == 0 || m.to == 0)
      castling &= ~WQ;
    if (m.from == 7 || m.to == 7)
      castling &= ~WK;
    if (m.from == 56 || m.to == 56)
      castling &= ~BQ;
    if (m.from == 63 || m.to == 63)
      castling &= ~BK;
    ep = -1;
    if (kindOf(moving) == PAWN && std::abs(m.to - m.from) == 16)
      ep = (m.from + m.to) / 2;
    halfmove = (kindOf(moving) == PAWN || captured) ? 0 : halfmove + 1;
    if (turn == BLACK)
      ++fullmove;
    turn = 1 - turn;
  }
  std::vector<Move> legal() const {
    std::vector<Move> candidates, result;
    pseudo(candidates);
    for (const Move& m : candidates) {
      Board b = *this;
      b.apply(m);
      if (!b.inCheck(turn))
        result.push_back(m);
    }
    return result;
  }
  std::string fen() const {
    std::string s;
    for (int r = 7; r >= 0; --r) {
      int empty = 0;
      for (int f = 0; f < 8; ++f) {
        int p = sq[square(f, r)];
        if (!p)
          ++empty;
        else {
          if (empty) {
            s += char('0' + empty);
            empty = 0;
          }
          s += pieceChar(p);
        }
      }
      if (empty)
        s += char('0' + empty);
      if (r)
        s += '/';
    }
    s += turn == WHITE ? " w " : " b ";
    std::string c;
    if (castling & WK)
      c += 'K';
    if (castling & WQ)
      c += 'Q';
    if (castling & BK)
      c += 'k';
    if (castling & BQ)
      c += 'q';
    s += c.empty() ? "-" : c;
    s += ' ';
    s += ep < 0 ? "-" : nameOf(ep);
    s += ' ' + std::to_string(halfmove) + ' ' + std::to_string(fullmove);
    return s;
  }
  static bool fromFen(const std::string& text, Board& b) {
    std::istringstream in(text);
    std::string placement, side, rights, e;
    int hm = 0, fm = 1;
    if (!(in >> placement >> side >> rights >> e >> hm >> fm))
      return false;
    b = Board{};
    b.halfmove = hm;
    b.fullmove = fm;
    int f = 0, r = 7;
    for (char c : placement) {
      if (c == '/') {
        if (f != 8)
          return false;
        f = 0;
        --r;
        continue;
      }
      if (c >= '1' && c <= '8') {
        f += c - '0';
        continue;
      }
      int k = 0;
      switch (std::tolower(c)) {
        case 'p':
          k = PAWN;
          break;
        case 'n':
          k = KNIGHT;
          break;
        case 'b':
          k = BISHOP;
          break;
        case 'r':
          k = ROOK;
          break;
        case 'q':
          k = QUEEN;
          break;
        case 'k':
          k = KING;
          break;
        default:
          return false;
      }
      if (!inside(f, r))
        return false;
      b.sq[square(f++, r)] = std::isupper(c) ? -k : k;
    }
    if (f != 8 || r != 0)
      return false;
    if (side == "w")
      b.turn = WHITE;
    else if (side == "b")
      b.turn = BLACK;
    else
      return false;
    b.castling = 0;
    if (rights != "-")
      for (char c : rights) {
        if (c == 'K')
          b.castling |= WK;
        else if (c == 'Q')
          b.castling |= WQ;
        else if (c == 'k')
          b.castling |= BK;
        else if (c == 'q')
          b.castling |= BQ;
        else
          return false;
      }
    b.ep = e == "-" ? -1 : parseSquare(e);
    if (e != "-" && b.ep < 0)
      return false;
    return b.kingSquare(WHITE) >= 0 && b.kingSquare(BLACK) >= 0;
  }
  bool insufficient() const {
    int minors = 0, bishops = 0, bishopColor = -1;
    for (int s = 0; s < 64; ++s)
      if (sq[s]) {
        int k = kindOf(sq[s]);
        if (k == PAWN || k == ROOK || k == QUEEN)
          return false;
        if (k == KNIGHT)
          ++minors;
        if (k == BISHOP) {
          ++minors;
          ++bishops;
          int col = (fileOf(s) + rankOf(s)) & 1;
          if (bishopColor < 0)
            bishopColor = col;
          else if (col != bishopColor)
            return false;
        }
      }
    return minors <= 1 || (bishops == minors);
  }
};

std::string key(const Board& b) {
  Board c = b;
  c.halfmove = 0;
  c.fullmove = 1;
  return c.fen();
}

std::string terminalState(const Board& b,
                          const std::unordered_map<std::string, int>& reps) {
  auto moves = b.legal();
  if (moves.empty())
    return b.inCheck(b.turn)
               ? (b.turn == WHITE ? "Checkmate — Black wins" : "Checkmate — White wins")
               : "Draw — stalemate";
  if (b.halfmove >= 100)
    return "Draw — 50-move rule";
  if (b.insufficient())
    return "Draw — insufficient material";
  auto it = reps.find(key(b));
  if (it != reps.end() && it->second >= 3)
    return "Draw — threefold repetition";
  return {};
}

std::string san(Board b, const Move& m) {
  const int moving = b.sq[m.from], kind = kindOf(moving);
  const bool capture = b.sq[m.to] != 0 || m.ep;
  if (m.castle)
    return fileOf(m.to) == 6 ? "O-O" : "O-O-O";
  std::string out;
  if (kind != PAWN) {
    const char letters[] = " PNBRQK";
    out += letters[kind];
    bool sameFile = false, sameRank = false, ambiguous = false;
    for (const Move& other : b.legal())
      if (!(other == m) && other.to == m.to && kindOf(b.sq[other.from]) == kind) {
        ambiguous = true;
        if (fileOf(other.from) == fileOf(m.from))
          sameFile = true;
        if (rankOf(other.from) == rankOf(m.from))
          sameRank = true;
      }
    if (ambiguous) {
      if (!sameFile)
        out += char('a' + fileOf(m.from));
      else if (!sameRank)
        out += char('1' + rankOf(m.from));
      else
        out += nameOf(m.from);
    }
  } else if (capture)
    out += char('a' + fileOf(m.from));
  if (capture)
    out += 'x';
  out += nameOf(m.to);
  if (m.promotion) {
    out += '=';
    out += char(std::toupper(pieceChar(m.promotion)));
  }
  b.apply(m);
  if (b.inCheck(b.turn))
    out += b.legal().empty() ? '#' : '+';
  return out;
}

struct Snapshot {
  Board board;
  std::vector<std::string> notation;
  std::array<std::vector<int>, 2> captured;
  Move last;
  std::vector<std::string> positions;
};

#ifdef CHESS_WEB_BUILD

std::string jsonEscape(const std::string& value) {
  std::string result;
  result.reserve(value.size() + 8);
  for (unsigned char ch : value) {
    switch (ch) {
      case '"': result += "\\\""; break;
      case '\\': result += "\\\\"; break;
      case '\b': result += "\\b"; break;
      case '\f': result += "\\f"; break;
      case '\n': result += "\\n"; break;
      case '\r': result += "\\r"; break;
      case '\t': result += "\\t"; break;
      default:
        if (ch < 0x20) {
          constexpr char hex[] = "0123456789abcdef";
          result += "\\u00";
          result += hex[ch >> 4];
          result += hex[ch & 0x0f];
        } else {
          result += static_cast<char>(ch);
        }
    }
  }
  return result;
}

class WebGame {
 public:
  WebGame() { reset(); }

  void reset() {
    board = Board::start();
    last = Move{};
    notation.clear();
    positions = {key(board)};
    captured = {};
    undoStack.clear();
    rebuildRepetitions();
  }

  void undo() {
    if (undoStack.empty())
      return;
    Snapshot snapshot = std::move(undoStack.back());
    undoStack.pop_back();
    board = snapshot.board;
    notation = std::move(snapshot.notation);
    captured = std::move(snapshot.captured);
    last = snapshot.last;
    positions = std::move(snapshot.positions);
    rebuildRepetitions();
  }

  bool play(int from, int to, int promotion) {
    if (!terminalState(board, repetitions).empty())
      return false;
    for (const Move& move : board.legal()) {
      if (move.from != from || move.to != to || move.promotion != promotion)
        continue;
      commit(move);
      return true;
    }
    return false;
  }

  std::string state() const {
    std::ostringstream out;
    out << "{\"board\":[";
    for (int i = 0; i < 64; ++i) {
      if (i) out << ',';
      out << board.sq[i];
    }
    out << "],\"turn\":" << board.turn
        << ",\"check\":" << (board.inCheck(board.turn) ? "true" : "false")
        << ",\"status\":\"" << jsonEscape(terminalState(board, repetitions))
        << "\",\"legal\":[";
    const std::vector<Move> moves = board.legal();
    for (std::size_t i = 0; i < moves.size(); ++i) {
      if (i) out << ',';
      out << '[' << moves[i].from << ',' << moves[i].to << ','
          << moves[i].promotion << ']';
    }
    out << "],\"history\":[";
    for (std::size_t i = 0; i < notation.size(); ++i) {
      if (i) out << ',';
      out << '"' << jsonEscape(notation[i]) << '"';
    }
    out << "],\"captured\":[";
    for (int side = 0; side < 2; ++side) {
      if (side) out << ',';
      out << '[';
      for (std::size_t i = 0; i < captured[side].size(); ++i) {
        if (i) out << ',';
        out << captured[side][i];
      }
      out << ']';
    }
    out << "],\"last\":[" << last.from << ',' << last.to << "]}";
    return out.str();
  }

 private:
  Board board;
  Move last;
  std::vector<std::string> notation;
  std::vector<std::string> positions;
  std::array<std::vector<int>, 2> captured;
  std::vector<Snapshot> undoStack;
  std::unordered_map<std::string, int> repetitions;

  void rebuildRepetitions() {
    repetitions.clear();
    for (const std::string& position : positions)
      ++repetitions[position];
  }

  void commit(const Move& move) {
    const std::string moveText = san(board, move);
    undoStack.push_back({board, notation, captured, last, positions});
    const int victim = move.ep
                           ? board.sq[move.to + (board.turn == WHITE ? -8 : 8)]
                           : board.sq[move.to];
    if (victim)
      captured[board.turn].push_back(kindOf(victim));
    board.apply(move);
    notation.push_back(moveText);
    last = move;
    positions.push_back(key(board));
    ++repetitions[key(board)];
  }
};

WebGame& webGame() {
  static WebGame game;
  return game;
}
std::string stateBuffer;

}  // namespace

extern "C" {
EMSCRIPTEN_KEEPALIVE const char* chess_state_json() {
  stateBuffer = webGame().state();
  return stateBuffer.c_str();
}
EMSCRIPTEN_KEEPALIVE int chess_play(int from, int to, int promotion) {
  return webGame().play(from, to, promotion) ? 1 : 0;
}
EMSCRIPTEN_KEEPALIVE void chess_reset() { webGame().reset(); }
EMSCRIPTEN_KEEPALIVE void chess_undo() { webGame().undo(); }
}

int main() { return 0; }

#else

void drawText(sf::RenderWindow& window, const sf::Font& font, const std::string& value,
              float x, float y, unsigned size, sf::Color color) {
  sf::Text text(font, value, size);
  text.setPosition({x, y});
  text.setFillColor(color);
  window.draw(text);
}

int pieceTextureIndex(int p) {
  return (p > 0 ? 0 : 6) + kindOf(p) - 1;
}

void drawPiece(sf::RenderWindow& window, const std::array<sf::Texture, 12>& textures,
               int p, float x, float y, float size) {
  const sf::Texture& texture = textures[pieceTextureIndex(p)];
  const sf::Vector2u textureSize = texture.getSize();
  if (textureSize.x == 0 || textureSize.y == 0)
    return;

  sf::Sprite sprite(texture);
  const float scale =
      size / static_cast<float>(std::max(textureSize.x, textureSize.y));
  sprite.setScale({scale, scale});
  const float renderedWidth = static_cast<float>(textureSize.x) * scale;
  const float renderedHeight = static_cast<float>(textureSize.y) * scale;
  sprite.setPosition({x + (size - renderedWidth) / 2.f,
                      y + (size - renderedHeight) / 2.f});
  window.draw(sprite);
}

void gui() {
  constexpr unsigned layoutWidth = 1088, layoutHeight = 760;
  constexpr float bx = 48.f, by = 78.f, tile = 76.f, boardSize = tile * 8.f;
  constexpr float panelX = 700.f, panelW = 350.f;
  const sf::Vector2u desktopSize = sf::VideoMode::getDesktopMode().size;
  const float initialScale = std::min(
      {1.f, 0.9f * static_cast<float>(desktopSize.x) / layoutWidth,
       0.9f * static_cast<float>(desktopSize.y) / layoutHeight});
  const sf::Vector2u initialSize{
      std::max(1u, static_cast<unsigned>(layoutWidth * initialScale)),
      std::max(1u, static_cast<unsigned>(layoutHeight * initialScale))};
  sf::RenderWindow window(sf::VideoMode(initialSize), "Chess — Local two-player",
                          sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize);

  sf::View gameView(sf::FloatRect({0.f, 0.f},
                                  {static_cast<float>(layoutWidth),
                                   static_cast<float>(layoutHeight)}));
  auto updateViewport = [&](sf::Vector2u windowSize) {
    if (windowSize.x == 0 || windowSize.y == 0)
      return;

    const float windowRatio = static_cast<float>(windowSize.x) /
                              static_cast<float>(windowSize.y);
    const float layoutRatio = static_cast<float>(layoutWidth) /
                              static_cast<float>(layoutHeight);
    sf::FloatRect viewport;
    if (windowRatio > layoutRatio) {
      viewport.size = {layoutRatio / windowRatio, 1.f};
      viewport.position = {(1.f - viewport.size.x) / 2.f, 0.f};
    } else {
      viewport.size = {1.f, windowRatio / layoutRatio};
      viewport.position = {0.f, (1.f - viewport.size.y) / 2.f};
    }
    gameView.setViewport(viewport);
    window.setView(gameView);
  };
  updateViewport(window.getSize());

  const int centeredX = static_cast<int>((desktopSize.x - initialSize.x) / 2);
  const int centeredY = static_cast<int>((desktopSize.y - initialSize.y) / 2);
  window.setPosition({centeredX, centeredY});
  window.setFramerateLimit(60);
  sf::Font font;
  const std::string assetRoot = CHESS_ASSET_DIR;
  if (!font.openFromFile(assetRoot + "/fonts/AdwaitaSans-Regular.ttf")) {
    std::cerr << "Could not load bundled font from " << assetRoot << "/fonts\n";
    return;
  }
  const std::array<std::string, 6> names{"pawn", "knight", "bishop",
                                         "rook", "queen",  "king"};
  std::array<sf::Texture, 12> textures;
  for (int side = 0; side < 2; ++side)
    for (int kind = 0; kind < 6; ++kind) {
      const std::string color = side == 0 ? "white" : "black";
      const std::string path =
          assetRoot + "/pieces/" + color + "_" + names[kind] + ".png";
      if (!textures[side * 6 + kind].loadFromFile(path)) {
        std::cerr << "Could not load piece image: " << path << "\n";
        return;
      }
      textures[side * 6 + kind].setSmooth(true);
    }

  Board b = Board::start();
  bool flipped = false;
  int selected = -1;
  Move last, pending;
  bool choosingPromotion = false;
  std::vector<std::string> notation, positions{key(b)};
  std::array<std::vector<int>, 2> captured;
  std::vector<Snapshot> undoStack;
  std::unordered_map<std::string, int> reps;
  reps[key(b)] = 1;
  auto rebuildRepetitions = [&]() {
    reps.clear();
    for (const auto& k : positions) ++reps[k];
  };
  auto reset = [&]() {
    b = Board::start();
    flipped = false;
    selected = -1;
    last = Move{};
    pending = Move{};
    choosingPromotion = false;
    notation.clear();
    positions = {key(b)};
    captured = {};
    undoStack.clear();
    rebuildRepetitions();
  };
  auto undo = [&]() {
    if (undoStack.empty())
      return;
    Snapshot s = std::move(undoStack.back());
    undoStack.pop_back();
    b = s.board;
    notation = std::move(s.notation);
    captured = std::move(s.captured);
    last = s.last;
    positions = std::move(s.positions);
    selected = -1;
    choosingPromotion = false;
    rebuildRepetitions();
  };
  auto commit = [&](const Move& m) {
    std::string sanText = san(b, m);
    undoStack.push_back({b, notation, captured, last, positions});
    int victim = m.ep ? b.sq[m.to + (b.turn == WHITE ? -8 : 8)] : b.sq[m.to];
    if (victim)
      captured[b.turn].push_back(kindOf(victim));
    b.apply(m);
    notation.push_back(sanText);
    last = m;
    positions.push_back(key(b));
    ++reps[key(b)];
    selected = -1;
    choosingPromotion = false;
  };
  auto button = [&](float x, float y, float w, float h, const std::string& label,
                    bool active) {
    sf::RectangleShape shape({w, h});
    shape.setPosition({x, y});
    shape.setFillColor(active ? sf::Color(75, 103, 84) : sf::Color(47, 55, 64));
    shape.setOutlineColor(sf::Color(92, 104, 112));
    shape.setOutlineThickness(1);
    window.draw(shape);
    drawText(window, font, label, x + 12, y + 9, 16, sf::Color(239, 237, 228));
  };

  while (window.isOpen()) {
    while (const std::optional event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>())
        window.close();
      if (const auto* resized = event->getIf<sf::Event::Resized>())
        updateViewport(resized->size);
      if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
        if (keyEvent->code == sf::Keyboard::Key::Escape)
          window.close();
        if (keyEvent->code == sf::Keyboard::Key::F2)
          reset();
        if (keyEvent->code == sf::Keyboard::Key::U)
          undo();
        if (keyEvent->code == sf::Keyboard::Key::F)
          flipped = !flipped;
        if (choosingPromotion) {
          int promo = keyEvent->code == sf::Keyboard::Key::Q   ? QUEEN
                      : keyEvent->code == sf::Keyboard::Key::R ? ROOK
                      : keyEvent->code == sf::Keyboard::Key::B ? BISHOP
                      : keyEvent->code == sf::Keyboard::Key::N ? KNIGHT
                                                               : 0;
          if (promo) {
            pending.promotion = promo;
            commit(pending);
          }
        }
      }
      if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
        if (mouse->button != sf::Mouse::Button::Left)
          continue;
        const sf::Vector2f mousePosition = window.mapPixelToCoords(mouse->position);
        const float mx = mousePosition.x, my = mousePosition.y;
        if (mx >= panelX && mx < panelX + panelW && my >= 650 && my < 702) {
          reset();
          continue;
        }
        if (mx >= panelX + 178 && mx < panelX + panelW && my >= 650 && my < 702) {
          undo();
          continue;
        }
        if (mx >= panelX && mx < panelX + panelW && my >= 708 && my < 754) {
          flipped = !flipped;
          continue;
        }
        if (choosingPromotion) {
          for (int i = 0; i < 4; ++i)
            if (mx >= bx + boardSize / 2 - 132 + i * 66 &&
                mx < bx + boardSize / 2 - 132 + (i + 1) * 66 &&
                my >= by + boardSize / 2 - 36 && my < by + boardSize / 2 + 36) {
              pending.promotion = std::array<int, 4>{QUEEN, ROOK, BISHOP, KNIGHT}[i];
              commit(pending);
              break;
            }
          continue;
        }
        if (!terminalState(b, reps).empty())
          continue;
        if (mx < bx || mx >= bx + boardSize || my < by || my >= by + boardSize)
          continue;
        int displayFile = int((mx - bx) / tile), displayRank = int((my - by) / tile);
        int f = flipped ? 7 - displayFile : displayFile,
            r = flipped ? displayRank : 7 - displayRank;
        int target = square(f, r);
        bool moved = false;
        for (const Move& m : b.legal())
          if (selected >= 0 && m.from == selected && m.to == target) {
            if (m.promotion) {
              pending = m;
              choosingPromotion = true;
            } else
              commit(m);
            moved = true;
            break;
          }
        if (!moved) {
          if (b.sq[target] && sideOf(b.sq[target]) == b.turn)
            selected = target;
          else
            selected = -1;
        }
      }
    }

    const std::string terminal = terminalState(b, reps);
    window.clear(sf::Color(25, 30, 36));
    drawText(window, font, "C H E S S", bx, 24, 22, sf::Color(235, 219, 184));
    drawText(window, font, "LOCAL TWO-PLAYER", panelX, 29, 14,
             sf::Color(167, 179, 184));

    for (int dr = 0; dr < 8; ++dr)
      for (int df = 0; df < 8; ++df) {
        int f = flipped ? 7 - df : df, r = flipped ? dr : 7 - dr, s = square(f, r);
        sf::RectangleShape sqr({tile, tile});
        sqr.setPosition({bx + df * tile, by + dr * tile});
        sf::Color color =
            ((f + r) & 1) ? sf::Color(104, 137, 119) : sf::Color(232, 224, 202);
        if (last.from == s || last.to == s)
          color = sf::Color(191, 174, 103);
        if (selected == s)
          color = sf::Color(205, 190, 102);
        if (b.inCheck(b.turn) && s == b.kingSquare(b.turn))
          color = sf::Color(190, 91, 82);
        sqr.setFillColor(color);
        window.draw(sqr);
      }
    if (selected >= 0)
      for (const Move& m : b.legal())
        if (m.from == selected) {
          int df = flipped ? 7 - fileOf(m.to) : fileOf(m.to),
              dr = flipped ? rankOf(m.to) : 7 - rankOf(m.to);
          const bool occupied = b.sq[m.to] != 0 || m.ep;
          if (occupied) {
            sf::CircleShape ring(tile * .43f);
            ring.setPosition(
                {bx + df * tile + tile * .07f, by + dr * tile + tile * .07f});
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineColor(sf::Color(41, 54, 45, 185));
            ring.setOutlineThickness(5);
            window.draw(ring);
          } else {
            sf::CircleShape dot(tile * .12f);
            dot.setPosition(
                {bx + df * tile + tile * .38f, by + dr * tile + tile * .38f});
            dot.setFillColor(sf::Color(41, 54, 45, 145));
            window.draw(dot);
          }
        }
    for (int s = 0; s < 64; ++s)
      if (b.sq[s]) {
        int df = flipped ? 7 - fileOf(s) : fileOf(s),
            dr = flipped ? rankOf(s) : 7 - rankOf(s);
        drawPiece(window, textures, b.sq[s], bx + df * tile, by + dr * tile, tile);
      }
    for (int i = 0; i < 8; ++i) {
      drawText(window, font, std::string(1, char('a' + (flipped ? 7 - i : i))),
               bx + i * tile + tile - 15, by + boardSize + 2, 12,
               sf::Color(65, 77, 68));
      drawText(window, font, std::string(1, char('1' + (flipped ? i : 7 - i))), bx - 19,
               by + i * tile + tile - 18, 12, sf::Color(65, 77, 68));
    }
    sf::RectangleShape frame({boardSize, boardSize});
    frame.setPosition({bx, by});
    frame.setFillColor(sf::Color::Transparent);
    frame.setOutlineColor(sf::Color(18, 22, 26));
    frame.setOutlineThickness(2);
    window.draw(frame);

    sf::RectangleShape card({panelW, 102});
    card.setPosition({panelX, 65});
    card.setFillColor(sf::Color(36, 43, 50));
    window.draw(card);
    if (!terminal.empty()) {
      drawText(window, font, terminal, panelX + 18, 82, 19, sf::Color(239, 204, 129));
      drawText(window, font, "Press F2 to start another game", panelX + 18, 118, 15,
               sf::Color(190, 198, 196));
    } else {
      drawText(window, font, b.turn == WHITE ? "WHITE TO MOVE" : "BLACK TO MOVE",
               panelX + 18, 82, 21, sf::Color(241, 237, 224));
      drawText(window, font,
               b.inCheck(b.turn) ? "Check!" : "Select a piece, then a destination",
               panelX + 18, 119, 15,
               b.inCheck(b.turn) ? sf::Color(245, 139, 124) : sf::Color(180, 190, 188));
    }
    drawText(window, font, "CAPTURED", panelX + 4, 190, 13, sf::Color(165, 178, 177));
    for (int side = 0; side < 2; ++side)
      for (std::size_t i = 0; i < captured[side].size(); ++i) {
        int p = (side == WHITE ? 1 : -1) * captured[side][i];
        drawPiece(window, textures, p, panelX + side * 170 + 4 + float(i % 8) * 20,
                  210 + float(i / 8) * 22, 20);
      }
    sf::RectangleShape listBg({panelW, 338});
    listBg.setPosition({panelX, 270});
    listBg.setFillColor(sf::Color(32, 38, 44));
    window.draw(listBg);
    drawText(window, font, "MOVE HISTORY", panelX + 14, 280, 14,
             sf::Color(165, 178, 177));
    const std::size_t firstPair =
        notation.size() > 24 ? (notation.size() - 24 + 1) / 2 : 0;
    const std::size_t pairCount = (notation.size() + 1) / 2;
    for (std::size_t pair = firstPair; pair < pairCount; ++pair) {
      const float rowY = 310 + float(pair - firstPair) * 23;
      drawText(window, font, std::to_string(pair + 1) + ".", panelX + 14, rowY, 14,
               sf::Color(129, 145, 147));
      drawText(window, font, notation[pair * 2], panelX + 53, rowY, 15,
               sf::Color(232, 231, 219));
      if (pair * 2 + 1 < notation.size())
        drawText(window, font, notation[pair * 2 + 1], panelX + 153, rowY, 15,
                 sf::Color(232, 231, 219));
    }
    button(panelX, 650, 160, 48, "New game  F2", false);
    button(panelX + 178, 650, 172, 48, "Undo move  U", false);
    button(panelX, 708, 350, 44, "Flip board  F", flipped);

    if (choosingPromotion) {
      sf::RectangleShape shade({static_cast<float>(layoutWidth),
                                static_cast<float>(layoutHeight)});
      shade.setFillColor(sf::Color(10, 13, 17, 175));
      window.draw(shade);
      sf::RectangleShape popup({330.f, 112.f});
      popup.setPosition({bx + boardSize / 2 - 165, by + boardSize / 2 - 56});
      popup.setFillColor(sf::Color(43, 50, 57));
      popup.setOutlineColor(sf::Color(216, 194, 143));
      popup.setOutlineThickness(2);
      window.draw(popup);
      drawText(window, font, "Choose promotion", bx + boardSize / 2 - 84,
               by + boardSize / 2 - 47, 16, sf::Color(245, 239, 220));
      const std::array<std::string, 4> labels{"Queen (Q)", "Rook (R)", "Bishop (B)",
                                              "Knight (N)"};
      for (int i = 0; i < 4; ++i)
        drawText(window, font, labels[i], bx + boardSize / 2 - 148 + i * 74,
                 by + boardSize / 2 + 2, 12, sf::Color(232, 223, 200));
    }
    window.display();
  }
}
}  // namespace

int main() {
  gui();
  return 0;
}

#endif  // CHESS_WEB_BUILD
