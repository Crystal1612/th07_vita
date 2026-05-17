# cl outputs its paths with showIncludes with absolute paths, which on wine
# will be prefixed with Z:. very annoying since ninja isn't able to properly
# read that, and as a result just marks everything as dirty which forces a
# complete rebuild every run.

import os
import re
import subprocess
import sys

env = os.environ.copy()
env["LANG"] = "ja_JP.UTF-8"
env["WINEDEBUG"] = "fixme-all"

proc = subprocess.run(
    ["wine", *sys.argv[1:]],
    env=env,
    capture_output=True,
    text=True,
    errors="replace",
)

for line in proc.stdout.splitlines():
    match = re.match(r"Note: including file:\s*(.+)", line)
    if match:
        filepath = match.group(1)

        filepath = filepath.replace("\\", "/")

        # get rid of that Z:
        if filepath.startswith("Z:") or filepath.startswith("z:"):
            filepath = filepath[2:]

        print(f"Note: including file: {filepath}")
    else:
        print(line)

if proc.stderr:
    print(proc.stderr, file=sys.stderr)

sys.exit(proc.returncode)
