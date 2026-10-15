import json
import subprocess
from pathlib import Path

Import("env")

project_dir = Path(env.subst("$PROJECT_DIR"))

try:
    revision = subprocess.check_output(
        ["git", "rev-parse", "--verify", "HEAD"],
        cwd=str(project_dir),
        stderr=subprocess.PIPE,
        text=True,
    ).strip()
except (OSError, subprocess.CalledProcessError) as error:
    raise RuntimeError(
        "Cannot determine firmware Git revision. Install Git on PATH and build "
        "from a Git checkout containing a commit."
    ) from error

generated_dir = Path(env.subst("$BUILD_DIR")) / "generated"
generated_dir.mkdir(parents=True, exist_ok=True)

header = generated_dir / "build_version.h"
content = (
    "#pragma once\n"
    f"#define BUILD_GIT_REVISION {json.dumps(revision)}\n"
)

# Preserve the timestamp when unchanged, avoiding needless recompilation.
if not header.exists() or header.read_text(encoding="utf-8") != content:
    header.write_text(content, encoding="utf-8")

env.Append(CPPPATH=[str(generated_dir)])
