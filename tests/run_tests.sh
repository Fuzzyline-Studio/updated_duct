#!/bin/sh
# 덕트 호스트 테스트 빌드·실행. ARDUINO_LIBS 로 라이브러리 경로를 바꿀 수 있다.
set -e
cd "$(dirname "$0")"
LIBS="${ARDUINO_LIBS:-$HOME/Documents/Arduino/libraries}"
OUT="${TMPDIR:-/tmp}/updated_duct_host_tests"
c++ -std=c++17 -w -I stubs -I "$LIBS/ArduinoJson/src" -I "$LIBS/SimpleTimer" \
    host_tests.cpp "$LIBS/SimpleTimer/SimpleTimer.cpp" -o "$OUT"
"$OUT"
