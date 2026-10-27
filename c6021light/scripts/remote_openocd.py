"""Configure PlatformIO's upload target to program via remote OpenOCD."""

import os
import shlex
import sys

Import("env")

project_dir = env.subst("$PROJECT_DIR")
uploader = os.path.join(project_dir, "scripts", "remote_openocd_upload.py")
remote_host = env.GetProjectOption(
    "custom_remote_host", "root@modellbahn-bpi.fritz.box"
)
env.Replace(
    UPLOADCMD="{} {} $SOURCE {}".format(
        shlex.quote(sys.executable),
        shlex.quote(uploader),
        shlex.quote(remote_host),
    )
)
