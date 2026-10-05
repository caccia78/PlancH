"""MkDocs hook: makes renders, video and production files available to the site.

The documentation reuses the files in media/ and production/ instead of keeping copies in
docs/. Before each build they are copied to docs/media/ and docs/production/ (ignored by git),
only when they changed, so that `mkdocs serve` does not rebuild in a loop.
"""

import filecmp
import shutil
from pathlib import Path

SOURCES = {
    "media": ["*"],
    "production": ["planch-ibom.html", "planch-schematic.pdf", "planch-bom.csv"],
}


def on_pre_build(config, **kwargs):
    root = Path(config["config_file_path"]).parent
    docs = Path(config["docs_dir"])
    for folder, patterns in SOURCES.items():
        for pattern in patterns:
            for src in (root / folder).glob(pattern):
                if not src.is_file():
                    continue
                dst = docs / folder / src.name
                if dst.exists() and filecmp.cmp(src, dst, shallow=False):
                    continue
                dst.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(src, dst)
