#!/bin/bash
set -euo pipefail

PYTHON="$(command -v python3)"
"$PYTHON" -m pip install --no-cache-dir \
  --index-url https://download.pytorch.org/whl/cpu \
  'torch==2.5.1+cpu'
"$PYTHON" -m pip install --no-cache-dir 'setuptools<81' 'seisbench==0.10.2'

export SEISBENCH_CACHE_ROOT="$(mktemp -d /tmp/seismic-phase-picking-cache.XXXXXX)"
"$PYTHON" <<'PY'
from __future__ import annotations

import hashlib
import os
import time
import urllib.request
from glob import glob
from pathlib import Path

import numpy as np
import pandas as pd
import seisbench.models as sbm
from obspy import Stream, Trace, UTCDateTime

MODEL_BASE = "https://seisbench.gfz.de/mirror/models/v3/phasenet"
MODEL_FILES = {
    "original.json.v2": "33a2540c49b87ebc78fe7a913c4d369f394d21b03f2a791713a23eb512b4e282",
    "original.pt.v2": "184dd85b896eebf8d5967ee15a722a477fa2e4ce479a3b113ecbcb8a6419113f",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def ensure_model_cache() -> None:
    cache_root = Path(os.environ["SEISBENCH_CACHE_ROOT"])
    model_dir = cache_root / "models" / "v3" / "phasenet"
    model_dir.mkdir(parents=True, exist_ok=True)

    for name, expected_hash in MODEL_FILES.items():
        destination = model_dir / name
        if destination.is_file() and sha256(destination) == expected_hash:
            continue

        temporary = destination.with_suffix(destination.suffix + ".part")
        temporary.unlink(missing_ok=True)
        last_error: Exception | None = None
        for attempt in range(1, 4):
            try:
                request = urllib.request.Request(
                    f"{MODEL_BASE}/{name}",
                    headers={"User-Agent": "AOS-Bench seismic phase picker"},
                )
                with urllib.request.urlopen(request, timeout=120) as response:
                    temporary.write_bytes(response.read())
                actual_hash = sha256(temporary)
                if actual_hash != expected_hash:
                    raise RuntimeError(
                        f"checksum mismatch for {name}: {actual_hash} != {expected_hash}"
                    )
                temporary.replace(destination)
                break
            except Exception as exc:
                last_error = exc
                temporary.unlink(missing_ok=True)
                if attempt < 3:
                    time.sleep(attempt * 2)
        else:
            raise RuntimeError(f"could not download verified {name}: {last_error}")


def find_input_dir() -> Path:
    candidates = [
        Path("/workspace/data__skillsbench-seismic-phase-picking"),
        Path("/workspace/data"),
    ]
    for candidate in candidates:
        if candidate.is_dir() and any(candidate.glob("*.npz")):
            return candidate
    raise FileNotFoundError("no seismic NPZ inputs found in the public workspace paths")


def load_streams(npz_dir: Path) -> dict[str, Stream]:
    streams: dict[str, Stream] = {}
    required_fields = {"data", "dt", "start_time", "network", "station", "channels"}

    for npz_file in sorted(glob(str(npz_dir / "*.npz"))):
        filename = Path(npz_file).name
        archive = np.load(npz_file, allow_pickle=False)
        missing = required_fields.difference(archive.files)
        if missing:
            raise ValueError(f"{filename} is missing required fields: {sorted(missing)}")

        waveform = np.asarray(archive["data"], dtype=np.float64) * 1e10
        if waveform.ndim != 2 or waveform.shape[1] < 1:
            raise ValueError(f"{filename} has invalid waveform shape {waveform.shape}")

        channel_names = [part.strip() for part in str(archive["channels"]).split(",")]
        if len(channel_names) < waveform.shape[1]:
            # Some public traces retain three data columns while metadata names only
            # the available one or two components. Preserve the named components and
            # mark the zero-filled/missing columns with distinct synthetic names.
            prefix = channel_names[0][:-1] if channel_names and len(channel_names[0]) > 1 else "HH"
            used = set(channel_names)
            for suffix in "ENZ123":
                candidate = prefix + suffix
                if candidate not in used:
                    channel_names.append(candidate)
                    used.add(candidate)
                if len(channel_names) == waveform.shape[1]:
                    break
        if len(channel_names) != waveform.shape[1]:
            raise ValueError(
                f"{filename} has {waveform.shape[1]} components but {len(channel_names)} channel names"
            )

        try:
            start_time = UTCDateTime(str(archive["start_time"]))
        except Exception:
            start_time = UTCDateTime(0)

        stream = Stream()
        for component, channel_name in zip(waveform.T, channel_names, strict=True):
            trace = Trace(data=component)
            trace.stats.network = str(archive["network"])
            trace.stats.station = str(archive["station"])
            trace.stats.channel = channel_name
            trace.stats.sampling_rate = 1.0 / float(archive["dt"])
            trace.stats.starttime = start_time
            stream.append(trace)
        streams[filename] = stream

    if not streams:
        raise RuntimeError(f"no NPZ traces found under {npz_dir}")
    return streams


def main() -> None:
    ensure_model_cache()
    streams = load_streams(find_input_dir())
    model = sbm.PhaseNet.from_pretrained("original", version_str="2")
    model.to("cpu")

    rows: list[dict[str, object]] = []
    for filename, stream in streams.items():
        sampling_rate = float(stream[0].stats.sampling_rate)
        start_time = stream[0].stats.starttime
        for pick in model.classify(stream).picks:
            if pick.phase not in {"P", "S"}:
                continue
            rows.append(
                {
                    "file_name": filename,
                    "phase": pick.phase,
                    "pick_idx": int((pick.peak_time - start_time) * sampling_rate),
                }
            )

    output = Path("/workspace/results.csv")
    output.parent.mkdir(parents=True, exist_ok=True)
    pd.DataFrame(rows, columns=["file_name", "phase", "pick_idx"]).to_csv(
        output, index=False
    )
    print(f"Wrote {len(rows)} P/S picks for {len(streams)} traces to {output}")


if __name__ == "__main__":
    main()
PY
