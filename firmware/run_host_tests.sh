#!/usr/bin/env bash
# Build and run the portable game-core test suite on your computer (no hardware).
# Requires g++ (C++17). Does NOT touch the ESP32.
set -e
cd "$(dirname "$0")"
CORE="src/chess.cpp src/variants.cpp src/engine.cpp src/game.cpp"

echo "== building host test suite =="
g++ -std=c++17 -O2 -Iinclude $CORE test/tests.cpp -o /tmp/chessboard_tests
echo "== running =="
/tmp/chessboard_tests

echo
echo "== full perft (startpos depth 5, kiwipete depth 4) =="
g++ -std=c++17 -O2 -Iinclude src/chess.cpp test/perft_main.cpp -o /tmp/chessboard_perft
/tmp/chessboard_perft
