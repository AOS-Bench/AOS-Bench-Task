#!/bin/bash
# Verifier: zero-network, pure bash + python3 stdlib.
# Hardened reimplementation of CompileAgent's is_compiled(strict=True):
# every expected artifact must exist under /workspace as a REAL build product.
# Checks (no raw-bytes fallback anywhere -- appended strings never help):
#   * exec:    fully parsed ELF, ET_EXEC/ET_DYN, must carry a non-stripped
#              .symtab; required symbols and the minimum defined-symbol count
#              are taken from .symtab only (all distro binaries are stripped,
#              so renamed/copied system ELFs fail).
#   * shlib:   ET_DYN; required symbols from defined .dynsym/.symtab entries;
#              minimum count of defined dynamic exports.
#   * archive: regular (non-thin) ar whose members are real ET_REL objects;
#              minimum member count; required symbols must be DEFINED in a
#              member's .symtab (the ar index is ignored -- it is unverified).
# Structural extent (section-header coverage / archive member walk) must also
# reach min_size, so zero-padding a small stub with truncate(1) fails.
set -o pipefail

# === aos-bench: stream verifier shim (added by task_construction) ===
# Make this test.sh runnable both under harbor's /tests mount and under
# AOS-Bench's per-task /tasks/<id>/tests/ mount.
__AOS_TESTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
# Prefer the env supplied by harbor's StreamVerifier (sets AOS_TESTS_DIR per
# arrival) when present — it points at the per-task upload dir even when
# this test.sh is sourced through a wrapper that loses __AOS_TESTS_DIR.
if [ -n "${AOS_TESTS_DIR:-}" ]; then
  __AOS_TESTS_DIR="$AOS_TESTS_DIR"
fi
# Tests dir basename in stream mode is the task_id, which often contains
# dashes (e.g. `llm-inference-batching-scheduler`). Python can't load such
# a path as a package, so relative imports (`from .cost_model import …`)
# in test_outputs.py fail. Create an underscore-only symlink alias and
# point pytest at it so the package is loadable.
__AOS_TESTS_BASE="$(basename "$__AOS_TESTS_DIR")"
__AOS_TESTS_SAFE_NAME="$(echo "$__AOS_TESTS_BASE" | tr '-' '_')"
if [ "$__AOS_TESTS_SAFE_NAME" != "$__AOS_TESTS_BASE" ]; then
  __AOS_TESTS_PARENT="$(dirname "$__AOS_TESTS_DIR")"
  __AOS_TESTS_SAFE="$__AOS_TESTS_PARENT/$__AOS_TESTS_SAFE_NAME"
  if [ ! -e "$__AOS_TESTS_SAFE" ]; then
    ln -sfn "$__AOS_TESTS_DIR" "$__AOS_TESTS_SAFE" 2>/dev/null || true
  fi
  # Point downstream commands at the safe alias so relative imports work.
  [ -e "$__AOS_TESTS_SAFE" ] && __AOS_TESTS_DIR="$__AOS_TESTS_SAFE"
fi
__AOS_REWARD_DIR="${AOS_REWARD_DIR:-/logs/verifier}"
mkdir -p "$__AOS_REWARD_DIR"
# Put the tests dir on sys.path. We run pytest with `--import-mode=importlib`
# (needed so a task_id containing dashes is still loadable), but that mode does
# NOT add the test file's own directory to sys.path the way the default
# `prepend` mode does. A grader that imports a helper shipped alongside it —
# e.g. skillsbench-seismic-phase-picking's `from evaluate_picks import ...` —
# then dies with ModuleNotFoundError during collection and scores 0 even though
# the helper is present. Prepending here keeps importlib mode and the sibling
# import working together.
PYTHONPATH="$__AOS_TESTS_DIR${PYTHONPATH:+:$PYTHONPATH}"
export PYTHONPATH
# (Old `/tests` symlink was removed: races between concurrent test.sh in
# StreamVerifier parallel mode. We rewrite hardcoded `/tests/<file>` refs
# below to `$__AOS_TESTS_DIR/<file>` instead.)
# Stub out runtime `curl https://astral.sh/uv/...install.sh | sh` so test.sh
# scripts that bootstrap uv don't hang on flaky outbound network. uv is
# pre-installed in /usr/local/bin via the merged Dockerfile (pip install uv).
# Provide $HOME/.local/bin/{uv,uvx,env} so `source $HOME/.local/bin/env` and
# subsequent `uvx ...` calls Just Work.
if command -v uv >/dev/null 2>&1 && [ ! -e "$HOME/.local/bin/uv" ]; then
  mkdir -p "$HOME/.local/bin"
  ln -sf "$(command -v uv)" "$HOME/.local/bin/uv"
  ln -sf "$(command -v uvx)" "$HOME/.local/bin/uvx" 2>/dev/null || true
  : > "$HOME/.local/bin/env"  # empty env stub so `source` is a no-op
fi
# Replace the astral.sh install pipeline with a no-op when uv exists.
__aos_install_uv_noop() { command -v uv >/dev/null 2>&1; }
# === end shim ===

mkdir -p /logs/verifier

python3 - > /logs/verifier/check.log 2>&1 <<'PYEOF'
import os
import struct
import sys

ROOT = os.environ.get("AOS_VERIFY_ROOT", "/workspace")

# (basename, kind, min_size_bytes, max_size_bytes_or_0, min_items, matchers)
# kind: exec | shlib | archive
# min_items: exec  -> min number of DEFINED .symtab symbols
#            shlib -> min number of DEFINED dynamic-symbol exports
#            archive -> min number of ET_REL relocatable members
# matcher: exact defined-symbol name, or "sub:X" = case-insensitive
#          substring of a defined symbol name. Symbols only; no byte scan.
SPEC = [
    ("libcrypto.so", "shlib", 1500000, 0, 1500, ["EVP_sha256", "EVP_DigestSqueeze"]),
    ("libssl.so", "shlib", 300000, 0, 300, ["SSL_CTX_new", "SSL_poll"]),
]

ET_REL, ET_EXEC, ET_DYN = 1, 2, 3
SHT_SYMTAB, SHT_DYNSYM, SHT_NOBITS = 2, 11, 8


def parse_elf(data):
    """Return (e_type, symtab_defined, dyn_defined, extent) or None."""
    if len(data) < 52 or data[:4] != b"\x7fELF":
        return None
    ei_class, ei_data = data[4], data[5]
    if ei_class not in (1, 2) or ei_data not in (1, 2):
        return None
    end = "<" if ei_data == 1 else ">"
    is64 = ei_class == 2
    try:
        (e_type,) = struct.unpack_from(end + "H", data, 16)
        if is64:
            (e_shoff,) = struct.unpack_from(end + "Q", data, 40)
            e_shentsize, e_shnum = struct.unpack_from(end + "HH", data, 58)
        else:
            (e_shoff,) = struct.unpack_from(end + "I", data, 32)
            e_shentsize, e_shnum = struct.unpack_from(end + "HH", data, 46)
    except struct.error:
        return None
    if e_type not in (ET_REL, ET_EXEC, ET_DYN):
        return None
    symtab_defined, dyn_defined = set(), set()
    shdrs = []
    extent = 64 if is64 else 52
    if e_shoff and e_shentsize >= (64 if is64 else 40):
        extent = max(extent, e_shoff + e_shnum * e_shentsize)
        for i in range(min(e_shnum, 65535)):
            off = e_shoff + i * e_shentsize
            if off + e_shentsize > len(data):
                shdrs = []
                break
            sh_type = struct.unpack_from(end + "I", data, off + 4)[0]
            if is64:
                sh_offset, sh_size = struct.unpack_from(end + "QQ", data, off + 24)
                (sh_link,) = struct.unpack_from(end + "I", data, off + 40)
                (sh_entsize,) = struct.unpack_from(end + "Q", data, off + 56)
            else:
                sh_offset, sh_size = struct.unpack_from(end + "II", data, off + 16)
                (sh_link,) = struct.unpack_from(end + "I", data, off + 24)
                (sh_entsize,) = struct.unpack_from(end + "I", data, off + 36)
            shdrs.append((sh_type, sh_offset, sh_size, sh_link, sh_entsize))
            if sh_type != SHT_NOBITS and sh_offset + sh_size <= len(data):
                extent = max(extent, sh_offset + sh_size)
    for sh_type, off, size, link, entsize in shdrs:
        if sh_type not in (SHT_SYMTAB, SHT_DYNSYM) or not entsize:
            continue
        if link >= len(shdrs):
            continue
        stroff, strsize = shdrs[link][1], shdrs[link][2]
        strtab = data[stroff : stroff + strsize]
        nsyms = min(size // entsize, 2_000_000)
        target = symtab_defined if sh_type == SHT_SYMTAB else dyn_defined
        for i in range(int(nsyms)):
            so = off + i * entsize
            if so + entsize > len(data):
                break
            (st_name,) = struct.unpack_from(end + "I", data, so)
            if is64:
                (st_shndx,) = struct.unpack_from(end + "H", data, so + 6)
            else:
                (st_shndx,) = struct.unpack_from(end + "H", data, so + 14)
            if st_name == 0 or st_name >= len(strtab):
                continue
            nul = strtab.find(b"\0", st_name)
            if nul == -1:
                continue
            if st_shndx != 0:  # not SHN_UNDEF
                target.add(strtab[st_name:nul].decode("latin-1"))
    return (e_type, symtab_defined, dyn_defined, extent)


def parse_archive(data):
    """Return (n_rel_members, member_defined_syms, extent) or None."""
    if not data.startswith(b"!<arch>\n"):
        return None
    off, nrel, syms = 8, 0, set()
    while off + 60 <= len(data):
        hdr = data[off : off + 60]
        if hdr[58:60] != b"`\n":
            break
        name = hdr[0:16].decode("latin-1").rstrip()
        try:
            size = int(hdr[48:58].decode("ascii").strip() or "0")
        except (ValueError, UnicodeDecodeError):
            break
        body = data[off + 60 : off + 60 + size]
        # NB: the ar symbol index ("/", "__.SYMDEF") is deliberately NOT
        # trusted for symbol matching -- it is free-form text an attacker
        # controls. Only symbols DEFINED inside real ET_REL members count.
        if name not in ("/", "//", "__.SYMDEF", "__.SYMDEF SORTED"):
            r = parse_elf(body)
            if r and r[0] == ET_REL:
                nrel += 1
                syms |= r[1]
        off += 60 + size + (size & 1)
    return (nrel, syms, off)


def match_one(matcher, syms):
    if matcher.startswith("sub:"):
        needle = matcher[4:].lower()
        for s in syms:
            if needle in s.lower():
                return "symbol-substr:" + s
        return None
    if matcher in syms:
        return "symbol"
    return None


def check_candidate(path, kind, min_size, max_size, min_items, matchers):
    msgs = []
    try:
        size = os.path.getsize(path)  # follows symlinks
        with open(path, "rb") as f:
            data = f.read()
    except OSError as exc:
        return False, ["unreadable: %s" % exc]
    if size < min_size:
        return False, ["too small: %d < %d bytes" % (size, min_size)]
    if max_size and size > max_size:
        return False, ["too large: %d > %d bytes" % (size, max_size)]
    if kind == "archive":
        if data.startswith(b"!<thin>\n"):
            return False, ["thin archive (members not embedded) rejected"]
        r = parse_archive(data)
        if r is None:
            return False, ["not a regular ar archive"]
        nitems, syms, extent = r
        if nitems < min_items:
            return False, [
                "only %d ET_REL member(s), need >= %d" % (nitems, min_items)
            ]
        msgs.append("%d relocatable member(s)" % nitems)
    else:
        r = parse_elf(data)
        if r is None:
            return False, ["not a parseable ELF"]
        e_type, symtab_defined, dyn_defined, extent = r
        if kind == "shlib":
            if e_type != ET_DYN:
                return False, ["ELF type %d, expected ET_DYN shared object" % e_type]
            syms = symtab_defined | dyn_defined
            nitems = len(dyn_defined)
            if nitems < min_items:
                return False, [
                    "only %d defined dynamic export(s), need >= %d"
                    % (nitems, min_items)
                ]
            msgs.append("%d defined dynamic export(s)" % nitems)
        else:  # exec
            if e_type not in (ET_EXEC, ET_DYN):
                return False, ["ELF type %d, expected ET_EXEC/ET_DYN" % e_type]
            syms = symtab_defined
            nitems = len(symtab_defined)
            if nitems < min_items:
                return False, [
                    "only %d defined .symtab symbol(s), need >= %d"
                    " (stripped/copied system binaries do not count)"
                    % (nitems, min_items)
                ]
            msgs.append("%d defined .symtab symbol(s)" % nitems)
    if extent < min_size:
        return False, [
            "structural extent %d < %d bytes (zero-padding does not count)"
            % (extent, min_size)
        ]
    for m in matchers:
        how = match_one(m, syms)
        if how is None:
            return False, ["required defined symbol %r not found" % m]
        msgs.append("matcher %r -> %s" % (m, how))
    return True, msgs


expected = {
    name: (kind, min_size, max_size, min_items, matchers)
    for name, kind, min_size, max_size, min_items, matchers in SPEC
}
candidates = {name: [] for name in expected}
for root, dirs, files in os.walk(ROOT):
    for name in files:
        if name in expected:
            candidates[name].append(os.path.join(root, name))

found, missing = [], []
for name, (kind, min_size, max_size, min_items, matchers) in expected.items():
    ok = False
    for path in sorted(candidates[name]):
        good, msgs = check_candidate(
            path, kind, min_size, max_size, min_items, matchers
        )
        print("%s %s [%s]: %s" % ("PASS" if good else "FAIL", path, kind, "; ".join(msgs)))
        if good:
            ok = True
            break
    if not candidates[name]:
        print("FAIL %s [%s]: no file with this basename under %s" % (name, kind, ROOT))
    (found if ok else missing).append(name)

print("expected:", len(expected))
print("found   :", sorted(found))
print("missing :", sorted(missing))
sys.exit(0 if not missing else 1)
PYEOF
rc=$?

if [ $rc -eq 0 ]; then
    echo 1 > "$__AOS_REWARD_DIR"/reward.txt
else
    echo 0 > "$__AOS_REWARD_DIR"/reward.txt
fi

cat /logs/verifier/check.log
