#include <SFML/Graphics.hpp>
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <iostream>
#include <optional>
#include <sstream>
#include <utility>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
constexpr int EMPTY=0, PAWN=1, KNIGHT=2, BISHOP=3, ROOK=4, QUEEN=5, KING=6;
constexpr int WHITE=0, BLACK=1;
constexpr int WK=1, WQ=2, BK=4, BQ=8;

