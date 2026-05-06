# Generate include/fw_version.h before compile (PlatformIO pre: hook).
Import("env")

import subprocess
from pathlib import Path

root = Path(env["PROJECT_DIR"])
ver_path = root / "VERSION"
version = ver_path.read_text().strip() if ver_path.exists() else "0.0.0"


def _git(*args: str) -> str:
    try:
        return subprocess.check_output(
            ["git", *args],
            cwd=root,
            stderr=subprocess.DEVNULL,
            text=True,
            timeout=8,
        ).strip()
    except (subprocess.CalledProcessError, OSError, subprocess.TimeoutExpired):
        return ""


describe = _git("describe", "--tags", "--always", "--dirty")
if not describe:
    describe = _git("rev-parse", "--short", "HEAD") or "unknown"

commit = _git("rev-parse", "HEAD") or "unknown"

out = root / "include" / "fw_version.h"
out.parent.mkdir(parents=True, exist_ok=True)

def _c_string(s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"')


out.write_text(
    "#pragma once\n"
    f'#define FW_VERSION "{_c_string(version)}"\n'
    f'#define FW_GIT_DESCRIBE "{_c_string(describe)}"\n'
    f'#define FW_GIT_COMMIT "{_c_string(commit)}"\n',
    encoding="utf-8",
)
