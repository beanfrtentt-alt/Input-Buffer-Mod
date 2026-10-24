#!/bin/sh
# Sanity checks that run WITHOUT the Geode SDK. They are not a substitute for the real build.
set -e
cd "$(dirname "$0")/.."
echo "== 1. predictor / decision logic unit test"
clang++ -std=c++20 -Wall -Wextra -o /tmp/ibd_predictor_test tests/predictor_test.cpp && /tmp/ibd_predictor_test
echo "== 2. main.cpp syntax check against a minimal MOCK of the Geode API (catches typos/scoping only)"
python3 - <<'PY'
import re
s=open('src/main.cpp').read()
s=re.sub(r'(    struct Fields \{.*?\n    \};)', r'\1\n    Fields m_fields_obj; Fields* m_fields = &m_fields_obj;', s, flags=re.S)
open('/tmp/ibd_main_mock.cpp','w').write(s)
PY
cp src/predictor.hpp /tmp/predictor.hpp
clang++ -std=c++23 -fsyntax-only -Wall -Wextra -Wno-unused-parameter -Itests/geode_mock /tmp/ibd_main_mock.cpp
echo "mock syntax check OK"
