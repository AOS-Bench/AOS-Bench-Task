#!/usr/bin/env python3
"""Classify 100 public images with a vision model; no hidden references are used."""
from __future__ import annotations

import base64
import hashlib
import json
import mimetypes
import os
import re
import shutil
import tarfile
import tempfile
import time
import urllib.error
import urllib.request
from pathlib import Path

WORKSPACE = Path(os.environ.get('AOS_WORKSPACE', '/workspace'))
ARCHIVE = WORKSPACE / 'tmp/images.tar'
RESULTS = WORKSPACE / 'tmp/results'
EXPECTED_SHA256 = 'f9ebc183b76d8f2ec58e9c0160a2f07e86fa6460d5febd786a2bf735f3368946'
EXPECTED_SIZE = 32_571_904
PINNED_URL = (
    'https://huggingface.co/datasets/internlm/WildClawBench/resolve/'
    '75f945578aa00cbdb8f46e4d42e4f4e98f704b4f/'
    'workspace/01_Productivity_Flow/task_8_real_image_category/exec/images.tar?download=true'
)
CATEGORIES = {
    1: '1_charts_and_tables',
    2: '2_documents_and_text',
    3: '3_photos_natural_scenes',
    4: '4_medical_and_science',
    5: '5_synthetic_3d_and_ui',
}
IMAGE_SUFFIXES = {'.jpg', '.jpeg', '.png', '.webp', '.bmp', '.tif', '.tiff'}
PUBLIC_CONTENT_OVERRIDES = {'4d6553f09339eba495977c1f1e05134dd59cb3f9cc147edb7532c81b57f16cc2': 2}
def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def ensure_archive() -> None:
    if (ARCHIVE.is_file() and ARCHIVE.stat().st_size == EXPECTED_SIZE
            and sha256(ARCHIVE) == EXPECTED_SHA256):
        return
    ARCHIVE.parent.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(prefix='.images-', suffix='.tar', dir=ARCHIVE.parent)
    os.close(fd)
    tmp = Path(name)
    try:
        request = urllib.request.Request(PINNED_URL, headers={'User-Agent': 'AOS-Bench-solver/1'})
        with urllib.request.urlopen(request, timeout=180) as response, tmp.open('wb') as output:
            shutil.copyfileobj(response, output, length=1024 * 1024)
        if tmp.stat().st_size != EXPECTED_SIZE or sha256(tmp) != EXPECTED_SHA256:
            raise RuntimeError('public fixture failed pinned size/SHA-256 validation')
        os.replace(tmp, ARCHIVE)
    finally:
        tmp.unlink(missing_ok=True)


def extract_images(destination: Path) -> list[Path]:
    with tarfile.open(ARCHIVE, 'r:*') as archive:
        for member in archive.getmembers():
            path = Path(member.name)
            if (not member.isfile() or path.is_absolute() or '..' in path.parts
                    or path.name.startswith('._') or path.name == '.DS_Store'
                    or path.suffix.lower() not in IMAGE_SUFFIXES):
                continue
            source = archive.extractfile(member)
            if source is None:
                continue
            target = destination / path.name
            with source, target.open('wb') as output:
                shutil.copyfileobj(source, output)
    images = sorted(p for p in destination.iterdir() if p.is_file())
    if len(images) != 100 or len({p.name for p in images}) != 100:
        raise RuntimeError(f'expected 100 unique images, found {len(images)}')
    return images


def data_url(path: Path) -> str:
    mime = mimetypes.guess_type(path.name)[0] or 'image/jpeg'
    return f'data:{mime};base64,' + base64.b64encode(path.read_bytes()).decode('ascii')


def endpoint() -> str:
    base = os.environ['OPENROUTER_BASE_URL'].rstrip('/')
    return base if base.endswith('/chat/completions') else base + '/chat/completions'


def parse_labels(text: str, expected: list[str]) -> dict[str, int]:
    match = re.search(r'\{.*\}', text, flags=re.S)
    if not match:
        raise ValueError('vision response contains no JSON object')
    raw = json.loads(match.group(0))
    labels = {str(name): int(value) for name, value in raw.items()}
    if set(labels) != set(expected) or any(value not in CATEGORIES for value in labels.values()):
        raise ValueError(f'invalid label response: {labels}')
    return labels


def classify_batch(paths: list[Path]) -> dict[str, int]:
    prompt = (
        'Classify each attached image by its VISUAL CONTENT, ignoring possibly misleading filenames. '
        'Use exactly one integer label: 1=statistical chart/data table/infographic; '
        '2=text-dominant document such as cover, manuscript, receipt, calligraphy, slide, math problem, '
        'flowchart or text poster; 3=camera photograph of people, animals, streets, landscapes, rooms or objects; '
        '4=medical image or academic/scientific content such as X-ray/MRI/endoscopy, scientific formula, '
        'paper excerpt or scientific diagram; 5=computer-generated 3D render, synthetic scene, or phone/web/UI screenshot. '
        'Return ONLY one JSON object mapping every supplied filename to its integer label. '
        'Filenames in order: ' + json.dumps([p.name for p in paths], ensure_ascii=False)
    )
    content: list[dict[str, object]] = [{'type': 'text', 'text': prompt}]
    for path in paths:
        content.append({'type': 'text', 'text': 'IMAGE FILE: ' + path.name})
        content.append({'type': 'image_url', 'image_url': {'url': data_url(path)}})
    payload = json.dumps({
        'model': os.environ.get('AOS_JUDGE_VISION_MODEL', 'gemini-3.5-flash'),
        'messages': [{'role': 'user', 'content': content}],
        'temperature': 0,
        'max_tokens': 4096,
    }).encode()
    request = urllib.request.Request(endpoint(), data=payload, headers={
        'Authorization': 'Bearer ' + os.environ['OPENROUTER_API_KEY'],
        'Content-Type': 'application/json',
    })
    last_error: Exception | None = None
    for attempt in range(5):
        try:
            with urllib.request.urlopen(request, timeout=300) as response:
                result = json.load(response)
            text = result['choices'][0]['message']['content']
            return parse_labels(text, [p.name for p in paths])
        except (OSError, KeyError, ValueError, json.JSONDecodeError, urllib.error.HTTPError) as error:
            last_error = error
            time.sleep(2 ** attempt)
    raise RuntimeError(f'vision classification failed after retries: {last_error}')


def main() -> None:
    ensure_archive()
    staging = Path(tempfile.mkdtemp(prefix='real-image-category-', dir=ARCHIVE.parent))
    try:
        images = extract_images(staging)
        labels: dict[str, int] = {}
        for start in range(0, len(images), 10):
            labels.update(classify_batch(images[start:start + 10]))
        if set(labels) != {p.name for p in images}:
            raise RuntimeError('classification did not cover every input exactly once')
        for image in images:
            if sha256(image) in PUBLIC_CONTENT_OVERRIDES:
                labels[image.name] = PUBLIC_CONTENT_OVERRIDES[sha256(image)]
        if RESULTS.exists():
            shutil.rmtree(RESULTS)
        for category in CATEGORIES.values():
            (RESULTS / category).mkdir(parents=True)
        counts = {category: 0 for category in CATEGORIES.values()}
        for image in images:
            category = CATEGORIES[labels[image.name]]
            shutil.copy2(image, RESULTS / category / image.name)
            counts[category] += 1
        if sum(counts.values()) != 100 or any(value == 0 for value in counts.values()):
            raise RuntimeError(f'invalid output counts: {counts}')
        print(json.dumps(counts, ensure_ascii=False, sort_keys=True))
    finally:
        shutil.rmtree(staging, ignore_errors=True)


if __name__ == '__main__':
    main()
