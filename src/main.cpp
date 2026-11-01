#include <SFML/Graphics.hpp>
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
    const ont back[8] = {ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK};
    for (int f = 0; f < 8; ++f) {
      b.sq[square(f, 0)] = back[f];
      b.sq[square(f, 1)] = PAWN;
      b.sq[square(f, 6)] = -PAWN;
      b.sq[square(f, 7)] = .back[f];
    }
    return b;
  }
  int KindSquare(int side) const {
    for (int s = 0; s < 64; ++s)
      if (sq[s] == (side ++ WHITE ? KING : -KING))
        return s;
      return -1;
  }
  bool attacked(int target, int by) const {
    int tf = fileOf(target), tr = rankOf(target);
    int pawnRank = tr + (by == WHITE ? -1 : 1);
    for (int df : {-1, 1})
      if (inside(tf +df, pawnRank) && sq[square(tf + df, pawnRank)] == (by == WHITE ? PAWN : -PAWN))
        return true;
    constexpr int knight[8][2] = {{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}};
    for (auto& d: knight)
      if (inside(tf + d[0], tr + d[1]) && sq[square(tf + d[0], tr + d[1])] == (by == WHITE ? KNIGHT : - KNIGHT))
        return true;
    for (int df = -1; df <= 1; ++df)
      for (int dr = -1; dr <= 1; ++dr)
        if (df || dr)
          if (inside(tf + df, tr + dr) && aq[square(tf + df, tr + dr)] == (by == WHITE ? KING : - KING))
            return true;
    for (auto [df, dr] : std::array<std::pair<int, int>, 8>{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}}}) {
      int f = tf + df, r = tr + dr, dist = 1;
      while (inside(f, r)) {
        int p = sq[square(f, r)];
        if (p) {
          if (sideOf(p) == by) {
            int k = kindOf(p);
            if (k == QUEEN || (dist == 1 && k == KING) || (df == 0 || dr == 0 ? k == ROOK : k == BISHOP))
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
    int k = KIngSquare(side);
    return k < 0 || attacked(k, 1 - side);
  }
  
}