#!/bin/bash
# Credential-free oracle solution. The timestamps are the creator-published
# chapter markers for the public tutorial identified in VIDEO_INFO.md.
set -euo pipefail

python3 - <<'PY'
import json
from pathlib import Path

chapters = [
    (0, "What we'll do"),
    (15, "How we'll get there"),
    (25, "Getting a floor plan"),
    (92, "Getting started"),
    (109, "Basic Navigation"),
    (126, "Import your plan into Blender"),
    (169, "Basic transform operations"),
    (205, "Setting up the plan and units"),
    (242, "It all starts with a plane"),
    (296, "Scaling the plane to real dimensions"),
    (329, "Getting the plan in place"),
    (400, "Tracing the outline"),
    (550, "Tracing inner walls"),
    (623, "Break"),
    (628, "Continue tracing inner walls"),
    (864, "Remove doubled vertices"),
    (875, "Save"),
    (881, "Make the floor"),
    (941, "Remove unnecessary geometry"),
    (983, "Make the floor's faces"),
    (994, "Make the background"),
    (1040, "Extruding the walls in Z"),
    (1056, "Reviewing face orientation"),
    (1095, "Adding thickness to walls with Modifiers"),
    (1134, "Fixing face orientation errors"),
    (1286, "Note on face orientation"),
    (1325, "Save As"),
    (1350, "If you need thick and thin walls"),
    (1367, "Great job!"),
]

payload = {
    "video_info": {
        "title": "In-Depth Floor Plan Tutorial Part 1",
        "duration_seconds": 1382,
    },
    "chapters": [{"time": time, "title": title} for time, title in chapters],
}
Path("/workspace/tutorial_index.json").write_text(
    json.dumps(payload, indent=2) + "\n", encoding="utf-8"
)
PY
