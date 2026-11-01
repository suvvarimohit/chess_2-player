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
