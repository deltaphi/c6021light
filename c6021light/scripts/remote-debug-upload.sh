#!/usr/bin/env bash
set -euo pipefail

# Usage: scripts/remote-debug-upload.sh [user@]openocd-host
# Override the SSH destination with PIO_REMOTE_HOST if preferred.
REMOTE_HOST="${1:-${PIO_REMOTE_HOST:-root@modellbahn-bpi.fritz.box}}"
REMOTE_ELF="${PIO_REMOTE_ELF:-/tmp/c6021light-firmware.elf}"
GDB_PORT="${PIO_REMOTE_GDB_PORT:-3333}"
TCL_PORT="${PIO_REMOTE_TCL_PORT:-6666}"
OPENOCD_INTERFACE="${PIO_OPENOCD_INTERFACE:-interface/stlink.cfg}"
OPENOCD_TARGET="${PIO_OPENOCD_TARGET:-target/stm32f1x.cfg}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ELF="$PROJECT_DIR/.pio/build/bluepill/firmware.elf"

command -v pio >/dev/null || { echo "pio is required" >&2; exit 1; }
command -v ssh >/dev/null || { echo "ssh is required" >&2; exit 1; }
command -v scp >/dev/null || { echo "scp is required" >&2; exit 1; }
[[ -n "$REMOTE_HOST" ]] || {
    echo "Usage: $0 [user@]openocd-host (or set PIO_REMOTE_HOST)" >&2
    exit 1
}

pio run -d "$PROJECT_DIR" -e bluepill
[[ -f "$ELF" ]] || { echo "Debug ELF not found: $ELF" >&2; exit 1; }

scp "$ELF" "$REMOTE_HOST:$REMOTE_ELF"

# Reuse an OpenOCD GDB server if one is already listening. Otherwise launch it
# detached with both GDB (3333) and Tcl (6666) services available remotely.
ssh "$REMOTE_HOST" sh -s -- "$GDB_PORT" "$TCL_PORT" "$OPENOCD_INTERFACE" "$OPENOCD_TARGET" <<'REMOTE'
set -eu
gdb_port="$1"
tcl_port="$2"
interface_cfg="$3"
target_cfg="$4"

# BusyBox nc may not implement OpenBSD nc's -z option. Connect with a short
# timeout and immediately close stdin instead to test whether the port accepts.
if ! nc -w 1 127.0.0.1 "$gdb_port" </dev/null >/dev/null 2>&1; then
    command -v openocd >/dev/null 2>&1 || {
        echo "openocd is not installed or not on PATH" >&2
        exit 1
    }
    log_file="${HOME}/openocd-remote.log"
    # Manual equivalent (run on the remote host):
    # openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "bindto 0.0.0.0" -c "gdb_port 3333" -c "tcl_port 6666"
    nohup openocd \
        -f "$interface_cfg" \
        -f "$target_cfg" \
        -c "bindto 0.0.0.0" \
        -c "gdb_port $gdb_port" \
        -c "tcl_port $tcl_port" \
        >"$log_file" 2>&1 </dev/null &

    ready=0
    attempt=0
    while [ "$attempt" -lt 30 ]; do
        if nc -w 1 127.0.0.1 "$gdb_port" </dev/null >/dev/null 2>&1; then
            ready=1
            break
        fi
        attempt=$((attempt + 1))
        sleep 1
    done
    if [ "$ready" -ne 1 ]; then
        echo "OpenOCD did not start; see $log_file" >&2
        exit 1
    fi
fi
REMOTE

# OpenOCD's Tcl port frames commands with ASCII Ctrl-Z (0x1a), not newlines.
# Program and verify first, then explicitly resume/reset the target. Do not
# shut down the server; PlatformIO/debuggers can attach to its GDB port after.
ssh "$REMOTE_HOST" sh -s -- "$TCL_PORT" "$REMOTE_ELF" <<'REMOTE'
set -eu
tcl_port="$1"
firmware="$2"
printf 'program %s verify\032reset run\032' "$firmware" | nc 127.0.0.1 "$tcl_port"
REMOTE

echo "Programmed $REMOTE_ELF on $REMOTE_HOST; OpenOCD GDB server is available on port $GDB_PORT."
