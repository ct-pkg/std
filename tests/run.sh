#!/usr/bin/env bash
#
# tests/run.sh - the std package gate.
#
# Ported from ct-lang's hard gate 11 (`test/std/run.sh`). The bootstrap chain is
# gone: this package does not build the compiler, it uses one. Provide it via
# PATH, or point CT at it:
#
#   bash tests/run.sh
#   CT=/path/to/ct CC=clang bash tests/run.sh
#
# For every unit under std/<mod>/<Unit>.h.ct it checks that the unit declares the
# namespace its directory implies and a `pub struct` named after its file, then
# drives every fixture under tests/<name>/:
#
#   every *.ct in the directory (main.ct plus any example, e.g. guide.ct)
#     -> ct build -I . --cflags "-Wall -Wextra" -> run
#
# `main.ct` is the regression matrix, `first.ct` is the opening program a module
# README prints and `guide.ct` is the runnable example set that README's tutorial
# is built on. Every program in the directory is compiled and run, so a snippet
# cannot rot in the docs alone. `tests/fs` receives a directory with exactly two
# entries and a file path outside it, so its listing assertions are deterministic.
#
# Discovery is explicit: every fixture passes `-I .`; the compiler has no
# default include root (D17-2).
#
# Exit code 0 means every assertion passed.

set -u

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root" || exit 1

CT="${CT:-ct}"
if ! command -v "$CT" >/dev/null 2>&1; then
    echo "ct not found (set CT=/path/to/ct or put it on PATH)" >&2
    exit 1
fi

CC="${CC:-cc}"
work="${TMPDIR:-/tmp}"
work="${work%/}/ct-std-pkg.$$"

rm -rf "$work"
mkdir -p "$work" || exit 1

pass=0
fail=0
ok()  { pass=$((pass + 1)); printf '  ok    %s\n' "$1"; }
bad() { fail=$((fail + 1)); printf '  FAIL  %s: %s\n' "$1" "$2"; }

echo "ct: $CT"
echo "cc: $CC"
echo

echo "== layout (the package must not be empty) =="
layout_bad=""
for mod in str io fs utf8 vec map arena test num math; do
    [ -d "std/$mod" ] || layout_bad="$layout_bad std/$mod(missing)"
done
if [ -z "$(find tests -mindepth 2 -name '*.ct' 2>/dev/null | head -1)" ]; then
    layout_bad="$layout_bad tests/*/*.ct(no fixtures)"
fi
if [ -z "$layout_bad" ]; then
    ok "ten modules and at least one fixture per package"
else
    bad "layout" "$layout_bad"
fi

echo
echo "== std units (namespace and type follow the directory) =="
unit_bad=""
for f in $(find std -name '*.h.ct' | sort); do
    mod="$(basename "$(dirname "$f")")"
    base="$(basename "$f" .h.ct)"
    if ! grep -qE "^namespace std::$mod;" "$f"; then
        unit_bad="$unit_bad $f(namespace)"
    fi
    if ! grep -qE "^(pub )?struct $base( |$)" "$f"; then
        unit_bad="$unit_bad $f(type)"
    fi
done
if [ -z "$unit_bad" ]; then
    ok "every std unit names its namespace and its type"
else
    bad "std units" "$unit_bad"
fi

mkdir -p "$work/fsdir"
: > "$work/fsdir/a.txt"
: > "$work/fsdir/b.txt"

echo
echo "== modules =="
for dir in tests/*/; do
    name="$(basename "$dir")"
    out="$work/$name"
    mkdir -p "$out"
    dir_ok=1
    detail=""
    for src in "$dir"*.ct; do
        [ -f "$src" ] || continue
        base="$(basename "$src" .ct)"
        bin="$work/$name-$base.bin"
        # One source of truth for the C command: the driver composes it (shim
        # roots, the generated outdir, the shim sources) and runs it. A
        # hand-written `cc` line here drifted from that one once and silently
        # dropped the shim roots, so do not reintroduce a second spelling.
        if ! CC="$CC" "$CT" build -I . --cflags "-Wall -Wextra" \
                "$src" -o "$bin" >"$out/$base.build.log" 2>&1; then
            dir_ok=0
            detail="$detail
    $base does not build: $(sed 's/^/    /' "$out/$base.build.log")"
            continue
        fi
        cmd=("$bin")
        case "$name" in
            # One output path per program: several fixtures live in the same
            # directory and the first one to run creates the file.
            fs) cmd+=("$work/fsdir" "$work/$base.out.txt") ;;
            io) cmd+=("$work/$base.io.txt") ;;
        esac
        "${cmd[@]}" >"$out/$base.stdout" 2>"$out/$base.stderr"
        rc=$?
        if [ "$name" = "test" ] && [ "$base" = "main" ]; then
            # The negative fixture: a non-zero exit and one FAIL line per failure.
            if [ "$rc" -eq 0 ]; then
                dir_ok=0
                detail="$detail
    $base: the failing fixture exited 0"
            elif [ "$(grep -c '^FAIL:' "$out/$base.stderr")" != "3" ]; then
                dir_ok=0
                detail="$detail
    $base: $(cat "$out/$base.stderr")"
            fi
        elif [ "$rc" -ne 0 ] || ! grep -q ' 0 failures' "$out/$base.stdout"; then
            dir_ok=0
            detail="$detail
    $base: $(cat "$out/$base.stderr" "$out/$base.stdout")"
        fi
    done
    if [ "$dir_ok" -eq 1 ]; then
        if [ "$name" = "test" ]; then
            ok "$name (three failures reported, non-zero exit)"
        else
            ok "$name"
        fi
    else
        bad "$name" "$detail"
    fi
done

echo
printf 'std: %d passed, %d failed\n' "$pass" "$fail"
if [ "$fail" -ne 0 ]; then
    exit 1
fi
