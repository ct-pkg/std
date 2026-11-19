#!/usr/bin/env bash
# One ASan fixture; g7 requires this entry to exist and pass.
set -e
cd "$(dirname "$0")/.."
CT="${CT:-ct}"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
"$CT" build -I . --cflags "-Wall -Wextra -g -fsanitize=address" --ldflags "-fsanitize=address" tests/str/main.ct -o "$tmp/fs-asan"
"$tmp/fs-asan" >/dev/null
echo "sanitizer: ok"
