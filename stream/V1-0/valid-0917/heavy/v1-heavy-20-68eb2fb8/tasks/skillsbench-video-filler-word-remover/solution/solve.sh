#!/usr/bin/env bash
set -euo pipefail
export HF_HUB_DISABLE_XET=1

INPUT=/workspace/input.mp4
ANNOTATIONS=/workspace/annotations.json
OUTPUT=/workspace/output.mp4
WORKDIR=$(mktemp -d /tmp/aos-filler.XXXXXX)
trap 'rm -rf "$WORKDIR"' EXIT

[ -s "$INPUT" ] || { echo "missing input video: $INPUT" >&2; exit 1; }
command -v ffmpeg >/dev/null
command -v ffprobe >/dev/null

# The frozen image intentionally contains only ffmpeg. Install the public,
# CPU-capable Whisper runtime into an isolated temporary venv rather than
# requiring an external transcription API or credential.
python3 -m venv "$WORKDIR/venv"
"$WORKDIR/venv/bin/pip" install --disable-pip-version-check --quiet \
  'faster-whisper==1.2.1' 'av<19'

"$WORKDIR/venv/bin/python" - "$INPUT" "$ANNOTATIONS" "$WORKDIR/segments.json" <<'PY'
import json
import re
import sys
from faster_whisper import WhisperModel

input_path, annotations_path, segments_path = sys.argv[1:]
model = WhisperModel("small.en", device="cpu", compute_type="int8", cpu_threads=4)
segments, _ = model.transcribe(
    input_path,
    language="en",
    beam_size=5,
    word_timestamps=True,
    vad_filter=True,
    condition_on_previous_text=True,
)

words = []
for segment in segments:
    for item in segment.words or []:
        token = re.sub(r"[^a-z0-9]+", "", item.word.lower())
        if token:
            words.append({
                "word": token,
                "start": float(item.start),
                "end": float(item.end),
                "probability": float(item.probability),
            })

single = {"um", "uh", "hum", "hmm", "mhm", "like", "yeah", "so", "basically", "well", "okay"}
phrases = {
    ("you", "know"): "you know",
    ("i", "mean"): "i mean",
    ("kind", "of"): "kind of",
    ("i", "guess"): "i guess",
}

found = []
covered = set()
for i in range(len(words) - 1):
    pair = (words[i]["word"], words[i + 1]["word"])
    label = phrases.get(pair)
    if label and words[i + 1]["start"] - words[i]["end"] <= 0.8:
        found.append({
            "word": label,
            "timestamp": round(words[i]["start"], 2),
            "start": words[i]["start"],
            "end": words[i + 1]["end"],
        })
        covered.update((i, i + 1))

for i, item in enumerate(words):
    token = item["word"]
    # Whisper commonly splits the spoken hesitation “mm-hmm” into mm + hmm.
    if token == "mm" and i + 1 < len(words) and words[i + 1]["word"] == "hmm":
        if words[i + 1]["start"] - item["end"] <= 0.25:
            found.append({
                "word": "mhm",
                "timestamp": round(item["start"], 2),
                "start": item["start"],
                "end": words[i + 1]["end"],
            })
            covered.update((i, i + 1))
            continue
    # A standalone “mm” is the same hesitation family as hmm/mhm.
    if token == "mm":
        token = "hmm"
    if i not in covered and token in single:
        found.append({
            "word": token,
            "timestamp": round(item["start"], 2),
            "start": item["start"],
            "end": item["end"],
        })

found.sort(key=lambda row: (row["start"], -(row["end"] - row["start"]), row["word"]))
unique = []
seen = set()
for item in found:
    key = (item["word"], item["timestamp"])
    if key not in seen:
        seen.add(key)
        unique.append(item)

with open(annotations_path, "w") as handle:
    json.dump(
        [{"word": row["word"], "timestamp": row["timestamp"]} for row in unique],
        handle,
        indent=2,
    )
with open(segments_path, "w") as handle:
    json.dump([[row["start"], row["end"]] for row in unique], handle)
print(f"detected {len(unique)} filler occurrences")
PY

python3 - "$WORKDIR/segments.json" "$INPUT" "$OUTPUT" "$WORKDIR" <<'PY'
import json
import pathlib
import subprocess
import sys

segments_path, input_path, output_path, workdir = sys.argv[1:]
with open(segments_path) as handle:
    raw = json.load(handle)

duration = float(subprocess.check_output([
    "ffprobe", "-v", "error", "-show_entries", "format=duration",
    "-of", "default=noprint_wrappers=1:nokey=1", input_path,
], text=True).strip())

padded = []
for start, end in raw:
    padded.append((max(0.0, float(start) - 0.08), min(duration, float(end) + 0.10)))
padded.sort()
merged = []
for start, end in padded:
    if merged and start <= merged[-1][1] + 0.04:
        merged[-1] = (merged[-1][0], max(merged[-1][1], end))
    else:
        merged.append((start, end))
if not merged:
    raise RuntimeError("transcription produced no requested filler words")

parts = []
for index, (start, end) in enumerate(merged):
    part = pathlib.Path(workdir) / f"part-{index:04d}.mp4"
    subprocess.run([
        "ffmpeg", "-nostdin", "-loglevel", "error", "-y",
        "-ss", f"{start:.3f}", "-to", f"{end:.3f}", "-i", input_path,
        "-map", "0:v:0", "-map", "0:a:0?",
        "-c:v", "mpeg4", "-q:v", "3",
        "-c:a", "aac", "-b:a", "128k", "-movflags", "+faststart", str(part),
    ], check=True)
    parts.append(part)

concat_file = pathlib.Path(workdir) / "concat.txt"
with concat_file.open("w") as handle:
    for part in parts:
        handle.write("file '" + str(part).replace("'", "'\\''") + "'\n")
subprocess.run([
    "ffmpeg", "-nostdin", "-loglevel", "error", "-y", "-f", "concat",
    "-safe", "0", "-i", str(concat_file), "-c", "copy", output_path,
], check=True)
PY

python3 -m json.tool "$ANNOTATIONS" >/dev/null
[ -s "$OUTPUT" ]
ffprobe -v error "$OUTPUT" >/dev/null
echo "wrote $ANNOTATIONS and $OUTPUT"
