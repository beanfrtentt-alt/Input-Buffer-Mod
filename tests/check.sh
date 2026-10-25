#!/bin/sh
# Offline sanity checks that do not require the Geode SDK.
# These checks are intentionally honest about their scope: they validate the
# pure predictor, metadata, and source include consistency, not Geode bindings.
set -eu
cd "$(dirname "$0")/.."

CXX="${CXX:-clang++}"
if ! command -v "$CXX" >/dev/null 2>&1; then
    CXX="${CXX_FALLBACK:-g++}"
fi
if ! command -v "$CXX" >/dev/null 2>&1; then
    echo "ERROR: need clang++ or g++ to run the predictor tests" >&2
    exit 127
fi

echo "== 1. predictor / decision logic unit tests ($CXX)"
"$CXX" -std=c++20 -Wall -Wextra -Wpedantic -Werror \
    -o /tmp/spoon_client_predictor_test tests/predictor_test.cpp
/tmp/spoon_client_predictor_test

echo "== 2. mod.json validation"
python3 - <<'PYJSON'
import json
from pathlib import Path

path = Path("mod.json")
data = json.loads(path.read_text(encoding="utf-8"))
for key in ("geode", "gd", "version", "id", "name", "settings"):
    if key not in data:
        raise SystemExit(f"mod.json is missing required top-level key: {key}")
print(f"mod.json OK: {data['name']} {data['version']}")
PYJSON

echo "== 3. local source include consistency"
python3 - <<'PYSRC'
import re
from pathlib import Path

root = Path.cwd()
source_root = root / "src"
missing = []
for path in source_root.rglob("*"):
    if path.suffix not in {".cpp", ".hpp", ".h"}:
        continue
    text = path.read_text(encoding="utf-8", errors="replace")
    for include in re.findall(r'^\s*#\s*include\s+"([^"]+)"', text, re.M):
        candidates = ((path.parent / include).resolve(), (source_root / include).resolve())
        if not any(candidate.is_file() for candidate in candidates):
            missing.append(f"{path.relative_to(root)} -> {include}")
if missing:
    print("Missing local includes:", *missing, sep="\n  ", flush=True)
    raise SystemExit(1)
print("All quoted includes under src/ resolve.")

# Regression guard: F8 must disable AG, not only flush one frame of inputs.
manager = (source_root / "AssistedGameplay" / "Manager.cpp").read_text(encoding="utf-8")
match = re.search(r"void Manager::emergencyStop\(\)\s*\{(.*?)\n\}", manager, re.S)
if not match or "setEnabled(false)" not in match.group(1):
    raise SystemExit("F8 regression: emergencyStop() must disable Assisted Gameplay")
print("F8 emergency-stop regression check OK.")
PYSRC

echo "All offline checks passed. Note: a real Geode SDK build is still required to validate game bindings and hooks."
