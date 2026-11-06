#!/usr/bin/env bash
#
# migrate-from-ct.sh - copy std sources and fixtures out of ct-lang.
#
#   bash tools/migrate-from-ct.sh                  # print the plan, write nothing
#   bash tools/migrate-from-ct.sh --check          # same plan, non-zero on drift
#   CT_SRC=/path/to/ct bash tools/migrate-from-ct.sh --apply
#
# What it copies (and nothing else):
#
#   $CT_SRC/std/{str,fs,utf8,vec,map,arena,test}/  ->  ./std/<mod>/
#   $CT_SRC/std/**/*.md                            ->  ./std/**/*.md  (rewritten, see below)
#   $CT_SRC/test/std/*                             ->  ./tests/
#
# `tests/run.sh` is excluded: it is this package's own gate, adapted from ct-lang
# hard gate 11, not a copy. Copying is idempotent (rsync --delete per module),
# so removing a file upstream and re-running makes it disappear here too.
#
# Link rewriting
# --------------
# The module docs are written for ct-lang's tree. Three link shapes point outside
# this package once copied, so every copied `.md` is piped through `rewrite_md`:
#
#   ../../test/std/<mod>/...     -> ../../tests/<mod>/...      (directory moved)
#   ../../issues/selfhost/...    -> $CT_REPO_URL/issues/selfhost/...
#   ../selfhost/ct-ast/...       -> $CT_REPO_URL/selfhost/ct-ast/...
#
# The last two point at the issue ledgers and compiler sources, which stay in
# ct-lang: an absolute URL cannot rot here, and copying them would go stale.
# These are the only rewrites; everything else is copied byte for byte, and
# `--check` compares the rewritten result, so the rules are reproducible.
#
# It reads ct-lang and never writes to it: no git commands, no deletions, no
# edits. The ct-side removal of std/ and test/std is a separate, coordinated step.

set -eu

here="$(cd "$(dirname "$0")/.." && pwd)"
CT_SRC="${CT_SRC:-$HOME/vscspace/ct}"
CT_REPO_URL="${CT_REPO_URL:-https://github.com/samxxu/ct-lang/blob/main}"

mods="str fs utf8 vec map arena test"
mode="dry-run"

usage() {
    printf 'usage: %s [--apply|--check]\n' "$0" >&2
    printf '  (default)  print the plan; write nothing\n' >&2
    printf '  --apply    copy sources and fixtures into this package\n' >&2
    printf '  --check    like the default, but exit 1 if anything would change\n' >&2
}

for arg in "$@"; do
    case "$arg" in
        --apply) mode="apply" ;;
        --check) mode="check" ;;
        -h|--help) usage; exit 0 ;;
        *) usage; exit 2 ;;
    esac
done

[ -f "$here/ct.pkg" ] || { echo "not a std package: $here" >&2; exit 1; }
if ! grep -qE '^name[[:space:]]*=[[:space:]]*std[[:space:]]*$' "$here/ct.pkg"; then
    echo "refusing: $here/ct.pkg does not declare 'name = std'" >&2
    exit 1
fi
[ -d "$CT_SRC/std" ] || { echo "no std/ under CT_SRC=$CT_SRC" >&2; exit 1; }
[ -d "$CT_SRC/test/std" ] || { echo "no test/std under CT_SRC=$CT_SRC" >&2; exit 1; }

for m in $mods; do
    [ -d "$CT_SRC/std/$m" ] || { echo "missing module: $CT_SRC/std/$m" >&2; exit 1; }
done

fixtures="$(find "$CT_SRC/test/std" -mindepth 2 -name '*.ct' | wc -l | tr -d ' ')"
[ "$fixtures" -gt 0 ] || { echo "no fixtures under $CT_SRC/test/std" >&2; exit 1; }

# Rewrite the three cross-tree link shapes; stdin -> stdout.
rewrite_md() {
    sed \
        -e 's#\.\./\.\./test/std/#../../tests/#g' \
        -e "s#\.\./\.\./issues/selfhost/#$CT_REPO_URL/issues/selfhost/#g" \
        -e "s#\.\./selfhost/ct-ast/#$CT_REPO_URL/selfhost/ct-ast/#g"
}

# Every markdown file under $CT_SRC/std, as a path relative to it.
md_list() {
    (cd "$CT_SRC/std" && find . -name '*.md' ! -path './.*' | sed 's|^\./||' | sort)
}

rsync_flags="-a --delete --exclude=.DS_Store"
# Markdown is transformed, not copied, so rsync must not see it (it would report
# the rewritten destination as drift on every --check).
copy_flags="$rsync_flags --exclude=*.md"

echo "source: $CT_SRC"
echo "target: $here"
echo "modules: $mods"
echo "fixtures: $fixtures .ct"
echo

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

md_drift() {
    # 0 = destination matches the rewritten source, 1 = it does not
    rc=0
    for rel in $(md_list); do
        rewrite_md <"$CT_SRC/std/$rel" >"$work/expected.md"
        if ! cmp -s "$work/expected.md" "$here/std/$rel"; then
            printf 'std/%s: rewritten links differ from source\n' "$rel"
            rc=1
        fi
    done
    return "$rc"
}

if [ "$mode" = "dry-run" ] || [ "$mode" = "check" ]; then
    changed=0
    for m in $mods; do
        # shellcheck disable=SC2086
        out="$(rsync $copy_flags --itemize-changes --dry-run "$CT_SRC/std/$m/" "$here/std/$m/" 2>/dev/null || true)"
        [ -z "$out" ] || { printf 'std/%s:\n%s\n' "$m" "$out"; changed=1; }
    done
    md_drift || changed=1
    # shellcheck disable=SC2086
    out="$(rsync $rsync_flags --exclude=run.sh --itemize-changes --dry-run "$CT_SRC/test/std/" "$here/tests/" 2>/dev/null || true)"
    [ -z "$out" ] || { printf 'tests/:\n%s\n' "$out"; changed=1; }

    if [ "$changed" -eq 0 ]; then
        echo "in sync; nothing to do."
        exit 0
    fi
    if [ "$mode" = "check" ]; then
        echo "drift detected." >&2
        exit 1
    fi
    echo "dry run: pass --apply to write the changes above."
    exit 0
fi

for m in $mods; do
    mkdir -p "$here/std/$m"
    # shellcheck disable=SC2086
    rsync $copy_flags "$CT_SRC/std/$m/" "$here/std/$m/"
done
# Transform-copy each markdown file, then drop any stale one upstream removed.
for rel in $(md_list); do
    mkdir -p "$here/std/$(dirname "$rel")"
    rewrite_md <"$CT_SRC/std/$rel" >"$here/std/$rel"
done
for rel in $(cd "$here/std" && find . -name '*.md' ! -path './.*' | sed 's|^\./||'); do
    if ! md_list | grep -Fxq "$rel"; then
        rm -f "$here/std/$rel"
        printf 'removed stale std/%s\n' "$rel"
    fi
done
mkdir -p "$here/tests"
# shellcheck disable=SC2086
rsync $rsync_flags --exclude=run.sh "$CT_SRC/test/std/" "$here/tests/"

echo
echo "migrated: $(find "$here/std" -type f ! -name '.DS_Store' | wc -l | tr -d ' ') files under std/, $fixtures fixtures under tests/"
echo "next: review 'git status', then commit and tag v0.1.0."
