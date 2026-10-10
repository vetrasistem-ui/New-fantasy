import hashlib
from pathlib import Path


def fingerprint(path):
    path = Path(path)
    digest = hashlib.sha256()
    size = 0
    with path.open('rb') as stream:
        while block := stream.read(1024 * 1024):
            digest.update(block)
            size += len(block)
    return {'sha256': digest.hexdigest().upper(), 'byte_size': size}


def verify(paths, before):
    for kind, path in paths.items():
        if fingerprint(path) != before[kind]:
            raise ValueError(f"Input changed during operation: {kind}: {path}")
