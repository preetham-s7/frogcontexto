"""Install the FrogContexto Codex skill for the current user."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--destination",
        type=Path,
        help="Optional skill directory, useful for testing or custom installs.",
    )
    args = parser.parse_args()

    repository_root = Path(__file__).resolve().parents[1]
    source = repository_root / "skills" / "frogcontexto"
    if args.destination:
        destination = args.destination.expanduser().resolve()
    else:
        codex_home = Path(os.environ.get("CODEX_HOME", Path.home() / ".codex"))
        destination = codex_home / "skills" / "frogcontexto"

    if not (source / "SKILL.md").is_file():
        raise SystemExit(f"Skill source not found: {source}")

    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(source, destination, dirs_exist_ok=True)
    print(f"Installed FrogContexto skill to: {destination}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
