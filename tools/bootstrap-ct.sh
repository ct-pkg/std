#!/usr/bin/env bash
#
# bootstrap-ct.sh - build the ct compiler from ct-lang sources, without
# installing it. Same chain as ct-lang's own CI (L0 -> L1 -> L2 -> ct-ast).
#
#   bash tools/bootstrap-ct.sh <ct-lang-src> <out-dir>
#
# Leaves the executable at <out-dir>/ct. The caller puts <out-dir> on PATH.
# Builds with ${CC:-gcc}; -Wall -Wextra -std=c99 matches the upstream chain.

set -eu

if [ "$#" -ne 2 ]; then
    echo "usage: $0 <ct-lang-src> <out-dir>" >&2
    exit 2
fi

src="$1"
out="$2"
cc="${CC:-gcc}"

[ -f "$src/selfhost/l0/ctc.c" ] || { echo "not a ct-lang checkout: $src" >&2; exit 1; }

mkdir -p "$out/l2src" "$out/astsrc"

cd "$src"

"$cc" -Wall -Wextra -std=c99 selfhost/l0/ctc.c -o "$out/ctc"
"$out/ctc" selfhost/l1/ctc.ct -o "$out/l1.c"
"$cc" -Wall -Wextra -std=c99 "$out/l1.c" -o "$out/l1"
"$out/l1" build selfhost/l2/driver.ct -o "$out/l2src"
"$cc" -Wall -Wextra -std=c99 "$out"/l2src/*.c -o "$out/l2"
"$out/l2" build -I selfhost/ct-ast selfhost/ct-ast/main.ct -o "$out/astsrc"
"$cc" -Wall -Wextra -std=c99 "$out"/astsrc/*.c -o "$out/ct"

echo "built $out/ct"
