#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

mkdir -p logs
: > logs/server.log
: > logs/client.log

rm -f inpipe outpipe
ln -sfn client/map map 2>/dev/null || true

# pick a python that can import pygame
pick_python() {
  local candidates=(
    "${PYTHON:-}"          # allow override: PYTHON=/path/to/python ./run.sh
    python3
    python
    /usr/bin/python3
    /usr/local/bin/python3
    /opt/homebrew/bin/python3
  )

  for p in "${candidates[@]}"; do
    [[ -z "$p" ]] && continue
    if command -v "$p" >/dev/null 2>&1; then
      if "$p" -c "import pygame" >/dev/null 2>&1; then
        echo "$p"
        return 0
      fi
    fi
  done
  return 1
}

if ! PY="$(pick_python)"; then
  echo "ERROR: No python found with pygame installed." | tee -a logs/client.log
  echo "Fix (recommended): create venv and install pygame:" | tee -a logs/client.log
  echo "  python3 -m venv .venv" | tee -a logs/client.log
  echo "  source .venv/bin/activate" | tee -a logs/client.log
  echo "  python -m pip install -U pip pygame" | tee -a logs/client.log
  echo "Then run: PYTHON=.venv/bin/python ./run.sh" | tee -a logs/client.log
  exit 1
fi

echo "Using python: $PY" >> logs/client.log
"$PY" -V >> logs/client.log 2>&1

# LFS sanity check: detect pointer files
check_lfs_file() {
  local f="$1"
  [[ -f "$f" ]] || return 0
  if head -n 1 "$f" 2>/dev/null | grep -q "version https://git-lfs.github.com/spec/v1"; then
    echo "ERROR: $f is a Git LFS pointer (real file not downloaded)." | tee -a logs/client.log
    if command -v git-lfs >/dev/null 2>&1 || git lfs version >/dev/null 2>&1; then
      echo "Running: git lfs pull" | tee -a logs/client.log
      git lfs pull | tee -a logs/client.log
    else
      echo "Install Git LFS then run: git lfs install && git lfs pull" | tee -a logs/client.log
      echo "macOS: brew install git-lfs" | tee -a logs/client.log
      echo "Ubuntu/WSL: sudo apt install git-lfs" | tee -a logs/client.log
      exit 1
    fi
  fi
}

check_lfs_file "map/15.png"

# start server in background
make run > logs/server.log 2>&1 &
srv_pid=$!

cleanup() {
  kill "$srv_pid" 2>/dev/null || true
  rm -f inpipe outpipe
}
trap cleanup EXIT INT TERM

# wait until inpipe is actually a FIFO (prevents client from creating a normal file)
for i in {1..400}; do
  [[ -p inpipe ]] && break
  sleep 0.05
done

if [[ ! -p inpipe ]]; then
  echo "ERROR: inpipe not created as FIFO. See logs/server.log" >> logs/server.log
  exit 1
fi

# run client (foreground)
"$PY" client/client.py > logs/client.log 2>&1

wait "$srv_pid" 2>/dev/null || true