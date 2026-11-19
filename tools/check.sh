#!/usr/bin/env bash
#
# tools/check.sh - the project gate entry point (G0-G9).
#
#   bash tools/check.sh all
#   bash tools/check.sh g0|g1|...|g9
#
# G0/G3/G9 are implemented by PH-8 T2. The remaining gates are scheduled by
# PH-8 T3-T7 and currently exit 2 (not implemented), which is intentional: the
# phase cannot close until they are real checks.

set -u

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root" || exit 1

SPEC_SRC="${AI_ENGINEERING_HOME:-/Users/samxx/trae-ws/.ai-engineering}"
pass=0
fail=0
ok()  { pass=$((pass + 1)); printf '  ok    %s\n' "$1"; }
bad() { fail=$((fail + 1)); printf '  FAIL  %s: %s\n' "$1" "$2"; }

need_file() {
    if [ -f "$1" ]; then ok "$2"; else bad "$2" "$1 missing"; fi
}

g0() {
    echo "== g0: spec landing =="
    if bash "$SPEC_SRC/bin/sync-agents.sh" --check AGENTS.md >/tmp/std-g0-sync.log 2>&1; then
        ok "AGENTS.md block + docs/spec copy match 1.17.0"
    else
        bad "AGENTS.md block" "$(tail -1 /tmp/std-g0-sync.log)"
    fi
    need_file docs/session-map.md "session map exists"
    if grep -q '^### 0\.2 话术路由' AGENTS.md && grep -q '^| .*开个新 phase' AGENTS.md; then
        ok "0.2 话术路由 has rows"
    else
        bad "0.2 话术路由" "missing route table outside the generated block"
    fi
    if grep -q 'deviation:' AGENTS.md; then ok "0.3 偏离项 registered"; else bad "0.3 偏离项" "no deviation"; fi
    v="$(sed -n 's/^version = //p' ct.pkg | head -1)"
    if [ -n "$v" ] && grep -q "当前版本 | \`$v\`" docs/planning/roadmap.md; then
        ok "version landing points agree ($v)"
    else
        bad "version landing points" "ct.pkg=$v roadmap mismatch"
    fi
}

g1() { echo "== g1: not implemented (PH-8 T3) =="; return 2; }
g2() { echo "== g2: not implemented (PH-8 T3) =="; return 2; }

g3() {
    echo "== g3: build and unit tests =="
    if bash tests/run.sh >/tmp/std-g3-tests.log 2>&1; then
        ok "tests/run.sh"
    else
        bad "tests/run.sh" "$(tail -2 /tmp/std-g3-tests.log)"
    fi
    if ct pkg check . >/tmp/std-g3-pkg.log 2>&1; then
        ok "ct pkg check"
    else
        bad "ct pkg check" "$(tail -2 /tmp/std-g3-pkg.log)"
    fi
}

g4() { echo "== g4: not implemented (PH-8 T4) =="; return 2; }
g5() { echo "== g5: not implemented (PH-8 T5) =="; return 2; }
g6() { echo "== g6: not implemented (PH-8 T6) =="; return 2; }
g7() { echo "== g7: not implemented (PH-8 T5) =="; return 2; }
g8() { echo "== g8: not implemented (PH-8 T7) =="; return 2; }

g9() {
    echo "== g9: spec pin =="
    if bash "$SPEC_SRC/bin/sync-agents.sh" --check AGENTS.md >/tmp/std-g9.log 2>&1; then
        ok "spec-version and sha256"
    else
        bad "spec pin" "$(tail -1 /tmp/std-g9.log)"
    fi
}

run_one() {
    local gate="$1"
    pass=0
    fail=0
    "$gate"
    local rc=$?
    if [ "$rc" -eq 2 ]; then
        echo "  BLOCKED  $gate not implemented (PH-8)"
        return 1
    fi
    [ "$fail" -eq 0 ] && [ "$rc" -eq 0 ]
}

case "${1:-all}" in
    g0) g0 ;;
    g1) g1 ;;
    g2) g2 ;;
    g3) g3 ;;
    g4) g4 ;;
    g5) g5 ;;
    g6) g6 ;;
    g7) g7 ;;
    g8) g8 ;;
    g9) g9 ;;
    all)
        total_bad=0
        for gate in g0 g1 g2 g3 g4 g5 g6 g7 g8 g9; do
            if run_one "$gate"; then :; else total_bad=$((total_bad + 1)); fi
            echo
        done
        echo "gates: $((10 - total_bad)) ok, $total_bad failed/blocked"
        [ "$total_bad" -eq 0 ]
        ;;
    *) echo "usage: $0 [all|g0..g9]" >&2; exit 2 ;;
esac
