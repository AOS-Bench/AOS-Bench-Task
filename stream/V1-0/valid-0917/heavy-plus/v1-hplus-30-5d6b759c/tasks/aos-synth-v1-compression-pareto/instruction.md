# Compression pareto search

A deterministic 2 GiB corpus has been baked into this image at
`/workspace/corpus/`. It's a mix of structured JSON logs, English-shaped text,
fixed-schema binary records, and a random tail. See `ls -lh /workspace/corpus/`
and the sibling manifest at `/workspace/corpus_manifest.json` (per-file
sha256, informational).

Your job is to find a compression setup that hits **both** of the
following gates on this 4-CPU / 8 GiB container:

## Gates

1. **Size gate.** The compressed artefact you produce at `/workspace/output.blob`
   must satisfy
   `size(/workspace/output.blob) <= $TARGET_COMPRESSED_BYTES`.
   Default: `TARGET_COMPRESSED_BYTES=618611712` (590 MiB, ~28.8% of raw).

2. **Wall-time gate.** The wall-clock seconds you spent producing
   `/workspace/output.blob` — measured by *you*, wrapping the compressor
   invocation — must satisfy
   `compression_wall_seconds <= $TARGET_COMPRESS_WALL`.
   Default: `TARGET_COMPRESS_WALL=360` seconds.
   Write the numeric wall time (as a plain float, e.g. `247.4`) to
   `/workspace/compression_wall_seconds.txt`. Only the compressor's wall time
   counts, not any experimentation you did before it.

3. **Correctness gate.** You must ALSO ship a shell script at
   `/workspace/decompress.sh` which, when invoked in a fresh scratch directory,
   reads `/workspace/output.blob` and reproduces the original corpus
   byte-for-byte. The verifier will run:

   ```
   mkdir -p /tmp/reconstructed
   cd /tmp/reconstructed && bash /workspace/decompress.sh
   diff -qr /workspace/corpus /tmp/reconstructed/corpus   # must be silent
   ```

   `decompress.sh` is expected to place the reconstructed tree at
   `./corpus/` under its CWD (i.e. `/tmp/reconstructed/corpus/`).

## Tools available

All of the following compressors are pre-installed and on `$PATH`:

- `zstd` (levels 1..22; `--long`, `--ultra`, `-T <threads>`)
- `xz` (levels 0..9; `-e` for extreme, `-T <threads>`)
- `brotli` (`-q 0..11`)
- `bzip2` (`-1..-9`)
- `gzip` (`-1..-9`)
- `lzma` (`-0..-9`; part of xz-utils)

You have **4 CPU cores** — multi-threaded modes (`zstd -T 4`, `xz -T 4`)
are the fast way to trade extra bytes for less wall time, but higher
compression levels blow through the wall gate. There is no single "best"
tuple — you'll need to iterate.

## Strategy sketch

- The 5% random tail sets a hard lower bound around 105 MiB compressed
  regardless of algorithm; JSON+text (85% of the corpus) is where levels
  matter.
- `zstd -T 4 --long -19` is dense but often blows the wall on 2 GiB;
  `zstd -T 4 -3` hits time but not size. The pareto point that satisfies
  both is somewhere else in the frontier.
- `xz -T 4 -6` and `brotli -q 9` are also candidates for different reasons.
- Rule of thumb: your first attempt won't hit both gates — plan on 5-8
  trials.

## Rules

- Container: 4 CPU, 8 GiB RAM, 20 GiB storage.
- No internet after container start (all binaries are pre-installed).
- You may combine tools: e.g. `tar -cf - /workspace/corpus | zstd -T 4 -19 --long -o /workspace/output.blob`
  is fine — but then `decompress.sh` must invoke the inverse pipeline.
- Do NOT delete `/workspace/corpus/` while iterating — the verifier reads it.
- Deliverables: `/workspace/output.blob`, `/workspace/decompress.sh`,
  `/workspace/compression_wall_seconds.txt`.
