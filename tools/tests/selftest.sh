#!/usr/bin/env bash
# Negative fixtures for the gate assertions. Each case mutates a throwaway copy
# and expects gate_checks.py to fail. Baseline before PH-8 T3-T7: cf53009.
set -u
cd "$(dirname "$0")/../.."
repo="$(pwd)"
base="$(mktemp -d)"
trap 'rm -rf "$base"' EXIT
case_no=0
copy() {
    rm -rf "$base/r"; mkdir -p "$base/r"
    rsync -a --exclude .git --exclude .ctbuild --exclude .temp "$repo/" "$base/r/"
}
expect_fail() {
    local gate="$1" label="$2"
    if python3 "$base/r/tools/gate_checks.py" "$gate" --root "$base/r" >/tmp/std-selftest.out 2>&1; then
        echo "FAIL: $label: gate $gate unexpectedly passed"; exit 1
    fi
    case_no=$((case_no + 1))
}
mut() { # mut file python snippet
    python3 - "$base/r/$1" "$2" <<'PY'
import sys
p, mode = sys.argv[1], sys.argv[2]
s=open(p).read()
if mode=='version-minor': s=s.replace('| version-minor | 8','| version-minor | 9')
elif mode=='drop-acc': s=s.replace('| `ACC-REG-1` | REQ-REG-1 | `tools/check.sh all` 退出 0 | `bash tools/check.sh all` |\n','')
elif mode=='secret': s='AKIAABCDEFGHIJKLMNOP\n'
elif mode=='bad-issue': s='---\nstatus: done\n---\n# bad\n'
elif mode=='bad-version': s=s.replace('version = 0.11.0','version = 0.11.1')
open(p,'w').write(s)
PY
}
copy; mut docs/phases/v0-PH-8/card.md version-minor; expect_fail g1 "card version-minor"
copy; mut docs/acceptance.md drop-acc; expect_fail g2 "missing ACC"
copy; printf 'applies-to: v0.11.0\n\n# big\n' > "$base/r/docs/big.md"; for i in $(seq 1 305); do echo "line $i" >> "$base/r/docs/big.md"; done; expect_fail g4 "doc over 300 lines"
copy; printf -- '---\nstatus: done\n---\n# bad\n' > "$base/r/issues/9999-new.md"; expect_fail g6 "invalid issue status"
copy; printf 'applies-to: v0.11.0\n\n# secret\n' > "$base/r/docs/secret.md"; echo 'AKIAABCDEFGHIJKLMNOP' >> "$base/r/docs/secret.md"; expect_fail g7 "secret-like string"
copy; mut ct.pkg bad-version; expect_fail g8 "version mismatch"
echo "selftest: $case_no negative gates ok"
