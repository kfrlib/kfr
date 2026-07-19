import argparse
from pathlib import Path
import re

parser = argparse.ArgumentParser()
parser.add_argument("root", type=Path)
parser.add_argument(
    "--thirdparty",
    action="store_true",
    help="include headers from include/kfr/thirdparty in the amalgamation",
)
args = parser.parse_args()

root = args.root.resolve()
entry = root / "include/kfr/all.hpp"
output = root / "kfr_amalgamated.hpp"
thirdparty = (root / "include/kfr/thirdparty").resolve()

include_re = re.compile(r'^(\s*)#\s*include\s*"([^"]+)"\s*$', re.MULTILINE)
included = set()


def expand(path: Path) -> str:
    path = path.resolve()

    if path in included:
        return ""

    included.add(path)
    text = path.read_text(encoding="utf-8")

    def replace(match: re.Match[str]) -> str:
        indentation, name = match.groups()

        # Resolve includes relative to the including file first, then include/kfr.
        candidates = [
            path.parent / name,
            root / "include/kfr" / name,
        ]

        dependency = next((candidate for candidate in candidates if candidate.exists()), None)

        if dependency is None:
            # Keep unresolved local includes unchanged.
            return match.group(0)

        if not args.thirdparty and thirdparty in dependency.resolve().parents:
            # Third-party headers remain external unless explicitly requested.
            return match.group(0)

        return (
            f"{indentation}// Begin amalgamated include: {name}\n"
            f"{expand(dependency)}\n"
            f"{indentation}// End amalgamated include: {name}"
        )

    return include_re.sub(replace, text)


output.write_text(
    "// KFR amalgamated header\n"
    "// Generated from include/kfr/all.hpp. Do not edit manually.\n"
    "#pragma once\n\n"
    + expand(entry),
    encoding="utf-8",
)

print(f"Generated {output}")
