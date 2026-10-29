#!/usr/bin/env python3
"""Run an SSH GDB-port forward for the lifetime of a PlatformIO debug session."""

import os
import subprocess
import sys


def main():
    if len(sys.argv) != 3:
        raise SystemExit("Usage: remote_openocd_tunnel.py <remote-host> <gdb-port>")

    remote_host, port = sys.argv[1:]
    command = [
        "ssh",
        "-N",
        "-o",
        "ExitOnForwardFailure=yes",
        "-o",
        "ServerAliveInterval=15",
        "-o",
        "ServerAliveCountMax=3",
        "-L",
        "127.0.0.1:{0}:127.0.0.1:{0}".format(port),
        remote_host,
    ]
    try:
        subprocess.run(command, check=False)
    except FileNotFoundError:
        raise SystemExit("ssh is not installed or not on PATH")


if __name__ == "__main__":
    main()
