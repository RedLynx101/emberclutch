#!/usr/bin/env bash
# One autotest run in Azahar with nobody's screen involved (docs/tech/headless-emulator.md).
# tools/autotest.ps1 -Headless calls it inside the emberclutch-test WSL distro:
#   autotest.sh --game <3dsx> --script <txt> --out <dir> [--save-in <dir>] [--timeout <s>]
#               [--speed <percent, 0 = as fast as it goes>] [--clock fixed|system] [--new3ds]
#               [--dsp <dspfirm.cdc>] [--gpu <adapter name, e.g. NVIDIA>]
# Every run gets its own emulator folder (config, SD card, log) under /tmp, so runs never share
# a save and several can go at once. The game plays the script and writes shots/done.txt; this
# copies the SD card's shots folder, the emulator's log and the save to --out.
# Exit codes: 0 finished, 2 timed out, 3 the emulator quit early.
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
game="" script="" out="" save_in="" dsp="" gpu="" timeout=180 speed=0 clock=fixed new3ds=false
fixed_time=1780308000  # 2026-06-01 10:00 on the 3DS clock: the same morning every run
while (($#)); do
    case $1 in
        --game) game=$2; shift ;;
        --script) script=$2; shift ;;
        --out) out=$2; shift ;;
        --save-in) save_in=$2; shift ;;
        --dsp) dsp=$2; shift ;;
        --gpu) gpu=$2; shift ;;
        --timeout) timeout=$2; shift ;;
        --speed) speed=$2; shift ;;
        --clock) clock=$2; shift ;;
        --new3ds) new3ds=true ;;
        *) echo "unknown option $1" >&2; exit 64 ;;
    esac
    shift
done
[[ -f $game && -f $script && -n $out ]] || { echo "need --game, --script and --out" >&2; exit 64; }
app=/opt/azahar/current/squashfs-root/AppRun
[[ -x $app ]] || { echo "Azahar missing: run tools/wsl/setup.sh as root" >&2; exit 65; }

# A run that was killed outright (its PowerShell closed) can't clean up: stop any emulator
# whose run folder is gone.
for pid in $(pgrep -f '^/opt/azahar/.*/AppRun.wrapped /tmp/ec-autotest\.' || true); do
    folder=$(tr '\0' '\n' < "/proc/$pid/cmdline" 2>/dev/null | sed -n 2p | xargs -r dirname)
    [[ -n $folder && ! -d $folder ]] && kill -KILL "$pid" 2>/dev/null || true
done

run=$(mktemp -d /tmp/ec-autotest.XXXXXX)
emu_pid=""
cleanup() {
    if [[ -n $emu_pid ]]; then
        # Xvfb and Azahar together. Azahar can shrug off SIGTERM, so then SIGKILL:
        # an emulator left running would hold its run folder and a display forever.
        kill -TERM -- -"$emu_pid" 2>/dev/null || true
        for _ in 1 2 3 4 5 6 7 8; do
            pgrep -g "$emu_pid" >/dev/null || break
            sleep 0.25
        done
        kill -KILL -- -"$emu_pid" 2>/dev/null || true
        wait "$emu_pid" 2>/dev/null || true
    fi
    rm -rf "$run"
}
trap cleanup EXIT

# Azahar's portable mode: a "user" folder in the working directory holds everything.
user=$run/user
sd=$user/sdmc/3ds/emberclutch
mkdir -p "$user/config" "$sd"
cp "$game" "$run/game.3dsx"  # off /mnt/c: reading a 75 MB romfs across the bridge is slow
cp "$script" "$sd/autotest.txt"
# ndsp needs the DSP firmware dumped from a 3DS; without it the game runs with the sound off.
[[ -n $dsp && -f $dsp ]] && cp "$dsp" "$user/sdmc/3ds/dspfirm.cdc"
if [[ -n $save_in ]]; then
    for f in save.a save.b; do [[ -f $save_in/$f ]] && cp "$save_in/$f" "$sd/"; done
fi
init_clock=0
if [[ $clock == fixed ]]; then
    # Azahar shifts a fixed time by the PC's time zone (WSL takes Windows'): in UTC the game
    # sees exactly fixed_time.
    init_clock=1
    export TZ=UTC
fi
sed -e "s/@FRAME_LIMIT@/$speed/" -e "s/@NEW_3DS@/$new3ds/" \
    -e "s/@INIT_CLOCK@/$init_clock/" -e "s/@INIT_TIME@/$fixed_time/" \
    "$here/qt-config.ini" > "$user/config/qt-config.ini"

# WSLg hands every distro a Wayland display (and an X one) that show up on the Windows
# desktop, and Qt prefers Wayland: without this Azahar opens a window in front of you.
unset WAYLAND_DISPLAY
export QT_QPA_PLATFORM=xcb
# OpenGL is drawn on the CPU (llvmpipe) unless --gpu names a Windows graphics adapter: Mesa's
# d3d12 driver then draws through WSL's GPU bridge (the Intel one only offers GL 4.1, too old).
if [[ -n $gpu ]]; then
    export GALLIUM_DRIVER=d3d12 MESA_D3D12_DEFAULT_ADAPTER_NAME=$gpu
fi

start_ms=$(date +%s%3N)
# setsid: Xvfb and Azahar in one process group, stopped together at the end. The display
# number is a random one that Xvfb could take: its lock file (/tmp/.X<n>-lock) is atomic, so
# two runs starting together can't share one (xvfb-run -a can, and -displayfd trips over
# WSLg's X server at :0).
(cd "$run" && exec setsid bash -c '
    for _ in $(seq 30); do
        n=$((100 + RANDOM % 900))
        Xvfb ":$n" -screen 0 1280x1024x24 -nolisten tcp 2> "$1/xvfb.txt" &
        sleep 0.3
        if kill -0 $! 2>/dev/null; then
            export DISPLAY=":$n"
            exec "$2" "$1/game.3dsx"
        fi
    done
    echo "Xvfb did not start"; cat "$1/xvfb.txt"; exit 1' _ "$run" "$app" > "$run/stdout.txt" 2>&1) &
emu_pid=$!

status=2
deadline=$(( $(date +%s) + timeout ))
while (( $(date +%s) < deadline )); do
    if [[ -f $sd/shots/done.txt ]]; then status=0; break; fi
    if ! kill -0 "$emu_pid" 2>/dev/null; then status=3; break; fi
    sleep 0.25
done
[[ $status == 0 ]] && sleep 0.5  # the last picture's file finishing
ms=$(( $(date +%s%3N) - start_ms ))
elapsed=$(printf "%d.%01d" $((ms / 1000)) $((ms % 1000 / 100)))

mkdir -p "$out"
rm -rf "$out/shots" "$out/save"
[[ -d $sd/shots ]] && cp -r "$sd/shots" "$out/shots"
mkdir -p "$out/save"
for f in save.a save.b; do [[ -f $sd/$f ]] && cp "$sd/$f" "$out/save/"; done
[[ -f $user/log/azahar_log.txt ]] && cp "$user/log/azahar_log.txt" "$out/"
cp "$run/stdout.txt" "$out/emulator_stdout.txt"
echo "$status $elapsed" > "$out/result.txt"
case $status in
    0) echo "finished in ${elapsed}s" ;;
    2) echo "TIMED OUT after ${timeout}s" ;;
    3) echo "the emulator quit early (see emulator_stdout.txt and azahar_log.txt)" ;;
esac
exit "$status"
