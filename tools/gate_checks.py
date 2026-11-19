#!/usr/bin/env python3
"""Gate assertions for std. Invoked by tools/check.sh."""
import argparse, glob, os, re, subprocess, sys

def read(p):
    with open(p, encoding="utf-8") as f:
        return f.read()

class Gate:
    def __init__(self, root):
        self.root = os.path.abspath(root)
        self.errors = []
    def fail(self, msg): self.errors.append(msg)
    def path(self, rel): return os.path.join(self.root, rel)

    def g1(self):
        formal = glob.glob(self.path("docs/phases/v0-PH-*/card.md"))
        if not formal:
            self.fail("no formal phase card"); return
        decisions = "\n".join(read(p) for p in glob.glob(self.path("docs/decisions/D-*.md")))
        for card in formal:
            s = read(card); base = os.path.basename(os.path.dirname(card))
            m = re.match(r"v0-PH-(\d+)$", base)
            if not m: self.fail(f"{base}: bad phase-id"); continue
            for needle in [f"| 编号 | `{base}` |", f"| version-minor | {m.group(1)}", "## 任务清单", "## 退出判据", "任务清单每项可独立验收"]:
                if needle not in s: self.fail(f"{base}: card missing {needle!r}")
            if not re.search(r"^\| T\d+ \|", s, re.M): self.fail(f"{base}: no task rows")
            if "TBD" in s: self.fail(f"{base}: contains TBD")
            if "| status | draft |" in s: self.fail(f"{base}: formal card marked draft")
            if f"| target | `{base}` |" not in decisions and f"| target-2 | `{base}` |" not in decisions and f"| target-3 | `{base}` |" not in decisions and f"| target-4 | `{base}` |" not in decisions:
                self.fail(f"{base}: no decision target authorization")
        for d in glob.glob(self.path("docs/phases/draft-*/card.md")):
            ds = read(d); base = os.path.basename(os.path.dirname(d))
            if f"| phase-id | `{base}` |" not in ds: self.fail(f"{base}: phase-id mismatch")
            if "| status | draft |" not in ds: self.fail(f"{base}: missing status draft")
            if "version-minor" in ds: self.fail(f"{base}: draft has version-minor")

    def g2(self):
        acc = read(self.path("docs/acceptance.md"))
        req = read(self.path("docs/requirements/scope.md"))
        req_ids = set(re.findall(r"REQ-[A-Z]+-\d+", req))
        acc_ids = {x.rstrip("-") for x in re.findall(r"`(ACC-[A-Za-z0-9-]+)`", acc)}
        for r in req_ids:
            if r not in acc: self.fail(f"{r} has no ACC mapping")
        for a in acc_ids:
            if f"`{a}`" not in acc: self.fail(f"{a} not in table")
        if "TBD" in acc or "TBD" in req: self.fail("TBD in requirements/acceptance")
        if "任务清单每项可独立验收" not in read(self.path("docs/phases/v0-PH-8/card.md")):
            self.fail("phase exit criterion missing T-6")

    def g4(self):
        legacy = set()
        lp = self.path("tools/legacy-docs.txt")
        if os.path.isfile(lp):
            legacy = {l.strip() for l in read(lp).splitlines() if l.strip()}
        docs = []
        for pat in ["*.md", "docs/**/*.md", "std/**/*.md"]:
            docs += glob.glob(self.path(pat), recursive=True)
        docs = sorted(set(os.path.relpath(p, self.root) for p in docs))
        new_docs = [d for d in docs if d not in legacy and d != "AGENTS.md"]
        for rel in new_docs:
            lines = read(self.path(rel)).splitlines()
            if rel != "docs/snippets/README.md" and "applies-to:" not in "\n".join(lines[:20]):
                self.fail(f"{rel}: missing applies-to")
            if len(lines) > 300: self.fail(f"{rel}: {len(lines)} lines > 300")
            # executable snippets in new docs must live under docs/snippets
            for fence in re.findall(r"```(\w+)?", read(self.path(rel))):
                if fence in ("ct", "c", "sh", "bash", "console"):
                    self.fail(f"{rel}: executable code block must come from docs/snippets/")
        # links
        for rel in docs + ["AGENTS.md"]:
            base = os.path.dirname(self.path(rel))
            text = read(self.path(rel))
            for target in re.findall(r"\]\(([^)#]+)(?:#[^)]*)?\)", text):
                if target.startswith(("http", "mailto:")) or not target: continue
                if not os.path.exists(os.path.normpath(os.path.join(base, target))):
                    self.fail(f"{rel}: dead link {target}")
        if not os.path.isfile(self.path("docs/snippets/README.md")):
            self.fail("docs/snippets/README.md missing")

    def g5(self):
        t = self.path("tools/tests/selftest.sh")
        if not os.path.isfile(t): self.fail("tools/tests/selftest.sh missing"); return
        r = subprocess.run(["bash", t], cwd=self.root, capture_output=True, text=True)
        if r.returncode != 0:
            self.fail("gate selftest failed: " + (r.stdout + r.stderr).strip().splitlines()[-1])

    def g6(self):
        if not os.path.isfile(self.path("docs/planning/pool.md")): self.fail("pool ledger missing")
        if not os.path.isfile(self.path("issues/README.md")) and not glob.glob(self.path("issues/*.md")):
            self.fail("issue ledger missing")
        legacy = set(l.strip() for l in read(self.path("tools/legacy-docs.txt")).splitlines())
        for p in glob.glob(self.path("issues/*.md")):
            rel = os.path.relpath(p, self.root)
            if rel in legacy: continue  # D-001
            s = read(p)
            m = re.search(r"^status:\s*(\w+)", s, re.M)
            if not m or m.group(1) not in {"open","fixed","verified","reopened","rejected","deferred","wontfix","superseded"}:
                self.fail(f"{rel}: invalid/missing status")
        # every version mismatch must have an issue: PH-8 checks its own release
        v = self.version()
        cards = sorted(glob.glob(self.path("docs/phases/v0-PH-*/card.md")))
        if v and cards:
            ns = [int(re.search(r"v0-PH-(\d+)", c).group(1)) for c in cards]
            if not any(re.match(rf"0\.{n + 4}\.\d+$", v) for n in ns):
                self.fail(f"version {v} does not match any open/closed phase")

    def version(self):
        m = re.search(r"^version\s*=\s*(\S+)", read(self.path("ct.pkg")), re.M)
        return m.group(1) if m else None

    def g7(self):
        pats = [r"AKIA[0-9A-Z]{16}", r"-----BEGIN (?:RSA|OPENSSH|EC) PRIVATE KEY-----", r"ghp_[A-Za-z0-9]{30,}"]
        for p in glob.glob(self.path("**/*"), recursive=True):
            if not os.path.isfile(p) or "/.git/" in p or "/.ctbuild/" in p: continue
            if os.path.getsize(p) > 2_000_000: continue
            if "/tools/tests/" in p: continue
            try: s = read(p)
            except Exception: continue
            for pat in pats:
                if re.search(pat, s): self.fail(f"secret-like pattern in {os.path.relpath(p,self.root)}")
        if not os.path.isfile(self.path("docs/planning/dependencies.md")): self.fail("dependency ledger missing")
        if not os.path.isfile(self.path("tools/sanitizer.sh")): self.fail("sanitizer entry missing")

    def g8(self):
        v = self.version()
        if not v: self.fail("ct.pkg version missing"); return
        readme = read(self.path("README.md")); road = read(self.path("docs/planning/roadmap.md"))
        if f"version = {v}" not in readme: self.fail("README ct.pkg block version mismatch")
        if f"`{v}`" not in readme: self.fail("README current version mismatch")
        if f"`{v}`" not in road: self.fail("roadmap version mismatch")
        # Annotated phase tag: at closeout v<version> must point at HEAD. While
        # the next phase is in progress the previous tag is allowed to lag.
        try:
            tv = f"v{v}"
            t = subprocess.run(["git", "cat-file", "-t", tv], cwd=self.root, capture_output=True, text=True)
            if t.returncode == 0 and t.stdout.strip() == "tag":
                h = subprocess.run(["git", "rev-parse", f"{tv}^{{commit}}"], cwd=self.root, capture_output=True, text=True)
                head = subprocess.run(["git", "rev-parse", "HEAD"], cwd=self.root, capture_output=True, text=True)
                if h.stdout.strip() != head.stdout.strip():
                    # allow lag only when a newer phase card is open
                    cards = glob.glob(self.path("docs/phases/v0-PH-9/card.md"))
                    lag_ok = False
                    if cards:
                        st = re.search(r"\| 状态 \| ([^|]+) \|", read(cards[0]))
                        lag_ok = bool(st and "已收口" not in st.group(1))
                    if not lag_ok:
                        self.fail(f"{tv} does not point at HEAD")
            else:
                self.fail(f"{tv} is not an annotated tag")
        except Exception as e: self.fail(str(e))
        sm = read(self.path("docs/session-map.md"))
        for d in glob.glob(self.path("docs/phases/draft-*/card.md")):
            slug = os.path.basename(os.path.dirname(d))
            if slug in sm: self.fail(f"draft {slug} listed in session-map")

def main():
    ap = argparse.ArgumentParser(); ap.add_argument("gate"); ap.add_argument("--root", default=".")
    a = ap.parse_args(); g = Gate(a.root)
    fn = getattr(g, a.gate, None)
    if not fn: print(f"unknown gate {a.gate}", file=sys.stderr); return 2
    fn()
    for e in g.errors: print("  FAIL  " + e)
    if g.errors: return 1
    print("  ok    " + a.gate); return 0

if __name__ == "__main__": sys.exit(main())
