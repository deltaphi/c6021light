"""Configure PlatformIO to upload and debug through remote OpenOCD."""

import os
import shlex
import sys

Import("env")

project_dir = env.subst("$PROJECT_DIR")
uploader = os.path.join(project_dir, "scripts", "remote_openocd_upload.py")
tunnel = os.path.join(project_dir, "scripts", "remote_openocd_tunnel.py")
remote_host = env.GetProjectOption(
    "custom_remote_host", "root@modellbahn-bpi.fritz.box"
)
gdb_port = os.environ.get("PIO_REMOTE_GDB_PORT", "3333")
env.Replace(
    UPLOADCMD="{} {} $BUILD_DIR/${{PROGNAME}}.elf {}".format(
        shlex.quote(sys.executable),
        shlex.quote(uploader),
        shlex.quote(remote_host),
    )
)
env.Replace(
    DEBUG_SERVER="{} {} {} {}".format(
        shlex.quote(sys.executable),
        shlex.quote(tunnel),
        shlex.quote(remote_host),
        shlex.quote(gdb_port),
    ),
    DEBUG_PORT="localhost:{}".format(gdb_port),
)
