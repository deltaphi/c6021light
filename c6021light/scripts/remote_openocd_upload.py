#!/usr/bin/env python3
"""Upload a PlatformIO-built ELF through an OpenOCD server on an SSH host."""

import os
import subprocess
import sys


REMOTE_ELF = os.environ.get("PIO_REMOTE_ELF", "/tmp/c6021light-firmware.elf")
GDB_PORT = os.environ.get("PIO_REMOTE_GDB_PORT", "3333")
TCL_PORT = os.environ.get("PIO_REMOTE_TCL_PORT", "6666")
INTERFACE = os.environ.get("PIO_OPENOCD_INTERFACE", "interface/stlink.cfg")
TARGET = os.environ.get("PIO_OPENOCD_TARGET", "target/stm32f1x.cfg")


def run(args, **kwargs):
    subprocess.run(args, check=True, **kwargs)


def main():
    if len(sys.argv) != 3:
        raise SystemExit(
            "Usage: remote_openocd_upload.py <firmware.elf> <remote-host>"
        )

    firmware = os.path.abspath(sys.argv[1])
    remote_host = os.environ.get("PIO_REMOTE_HOST", sys.argv[2])
    if not os.path.isfile(firmware):
        raise SystemExit("Firmware ELF not found: {}".format(firmware))

    # Transfer the built ELF before starting or reusing OpenOCD.
    run(["scp", firmware, "{}:{}".format(remote_host, REMOTE_ELF)])

    remote_start = r'''set -eu
gdb_port="$1"
tcl_port="$2"
interface_cfg="$3"
target_cfg="$4"
if ! nc -w 1 127.0.0.1 "$gdb_port" </dev/null >/dev/null 2>&1; then
    command -v openocd >/dev/null 2>&1 || {
        echo "openocd is not installed or not on PATH" >&2
        exit 1
    }
    log_file="${HOME}/openocd-remote.log"
    nohup openocd -f "$interface_cfg" -f "$target_cfg" \
        -c "bindto 127.0.0.1" -c "gdb_port $gdb_port" \
        -c "tcl_port $tcl_port" >"$log_file" 2>&1 </dev/null &
    attempt=0
    while [ "$attempt" -lt 30 ]; do
        if nc -w 1 127.0.0.1 "$gdb_port" </dev/null >/dev/null 2>&1; then
            exit 0
        fi
        attempt=$((attempt + 1))
        sleep 1
    done
    echo "OpenOCD did not start; see $log_file" >&2
    exit 1
fi
'''
    run(
        ["ssh", remote_host, "sh", "-s", "--", GDB_PORT, TCL_PORT, INTERFACE, TARGET],
        input=remote_start,
        text=True,
    )

    # The OpenOCD Tcl interface uses Ctrl-Z command terminators.
    remote_program = r'''set -eu
printf 'program %s verify\032reset run\032' "$1" | nc 127.0.0.1 "$2"
'''
    run(
        ["ssh", remote_host, "sh", "-s", "--", REMOTE_ELF, TCL_PORT],
        input=remote_program,
        text=True,
    )
    print(
        "Programmed {} on {}; OpenOCD GDB server is available via SSH tunnel on port {}.".format(
            REMOTE_ELF, remote_host, GDB_PORT
        )
    )


if __name__ == "__main__":
    main()
