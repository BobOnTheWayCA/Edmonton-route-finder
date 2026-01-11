#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

# logs folder in project root
mkdir -p logs
: > logs/server.log
: > logs/client.log

# clean leftover pipes
rm -f inpipe outpipe

# map symlink (optional)
ln -sfn client/map map 2>/dev/null || true

# start server in background, log to logs/server.log
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

# run client in foreground, log to logs/client.log
python3 client/client.py > logs/client.log 2>&1

# wait for server to exit
wait "$srv_pid" 2>/dev/null || true
