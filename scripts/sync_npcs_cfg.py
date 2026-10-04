#!/usr/bin/env python3
"""
Copy the repo npcs.cfg into Jedi Outcast GameData so SP loads the updated NPC stats.

The game reads ext_data/NPCs.cfg (see NPC_stats.cpp). This script copies from the
repo root npcs.cfg to that path under Steam's default install.
"""

from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_SOURCE = REPO_ROOT / "npcs.cfg"
DEFAULT_DEST_DIR = Path(
    r"C:\Program Files (x86)\Steam\steamapps\common\Jedi Outcast\GameData\base\ext_data"
)
GAME_FILENAME = "NPCs.cfg"


def sync_npcs_cfg(
    source: Path,
    dest_dir: Path,
    dry_run: bool = False,
) -> int:
    dest = dest_dir / GAME_FILENAME

    if not source.is_file():
        print(f"Source not found: {source}", file=sys.stderr)
        return 1

    if not dest_dir.is_dir():
        print(f"Destination folder not found: {dest_dir}", file=sys.stderr)
        print("Check your Steam install path or pass --dest-dir.", file=sys.stderr)
        return 1

    if dry_run:
        print(f"Would copy:\n  {source}\n  -> {dest}")
        return 0

    try:
        shutil.copy2(source, dest)
    except OSError as exc:
        print(f"Copy failed: {exc}", file=sys.stderr)
        return 1

    print(f"Copied npcs.cfg -> {dest}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Copy repo npcs.cfg to Jedi Outcast base/ext_data/NPCs.cfg.",
    )
    parser.add_argument(
        "--source",
        type=Path,
        default=DEFAULT_SOURCE,
        help=f"npcs.cfg to deploy (default: {DEFAULT_SOURCE})",
    )
    parser.add_argument(
        "--dest-dir",
        type=Path,
        default=DEFAULT_DEST_DIR,
        help="Jedi Outcast base/ext_data directory",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print paths only; do not copy",
    )
    args = parser.parse_args()
    return sync_npcs_cfg(args.source, args.dest_dir, args.dry_run)


if __name__ == "__main__":
    raise SystemExit(main())
