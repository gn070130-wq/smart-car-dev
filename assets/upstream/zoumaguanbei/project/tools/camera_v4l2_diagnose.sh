#!/usr/bin/env bash
# V4L2 camera control diagnostics. Read-only unless --write-test --yes is given.
set -u

DEVICE=""
WRITE_TEST=0
CONFIRMED=0
LOG="camera_v4l2_diagnose_$(date +%Y%m%d_%H%M%S).log"

usage() {
  cat <<'USAGE'
Usage:
  camera_v4l2_diagnose.sh [--device /dev/videoN] [--log FILE]
  camera_v4l2_diagnose.sh [--device /dev/videoN] --write-test --yes [--log FILE]

Default mode is read-only. It enumerates devices, formats, frame rates, all controls,
and their min/max/step/default/current values.

--write-test --yes tests writable numeric/menu/boolean controls at representative
values and restores each original value immediately. Stop applications using the
camera first. Camera runtime controls are changed temporarily; no config file is edited.
USAGE
}

while (($#)); do
  case "$1" in
    -d|--device) [[ $# -ge 2 ]] || { echo "Missing device" >&2; exit 2; }; DEVICE=$2; shift 2 ;;
    --log) [[ $# -ge 2 ]] || { echo "Missing log path" >&2; exit 2; }; LOG=$2; shift 2 ;;
    --write-test) WRITE_TEST=1; shift ;;
    --yes) CONFIRMED=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

command -v v4l2-ctl >/dev/null 2>&1 || {
  echo "ERROR: v4l2-ctl not found. Install package v4l-utils." >&2; exit 127;
}
if ((WRITE_TEST && !CONFIRMED)); then
  echo "ERROR: write testing requires both --write-test and --yes." >&2; exit 2
fi

exec > >(tee "$LOG") 2>&1
printf 'V4L2 camera diagnostic\nTime: %s\nKernel: %s\nUser: %s (groups: %s)\nMode: %s\nLog: %s\n\n' \
  "$(date -Is)" "$(uname -a)" "$(id -un)" "$(id -Gn)" \
  "$([[ $WRITE_TEST == 1 ]] && echo WRITE-TEST || echo READ-ONLY)" "$LOG"

mapfile -t ALL_DEVICES < <(compgen -G '/dev/video*' | sort -V)
((${#ALL_DEVICES[@]})) || { echo 'ERROR: no /dev/video* nodes found.'; exit 1; }

print_device_summary() {
  local dev=$1 info card driver bus controls formats
  info=$(v4l2-ctl -d "$dev" --all 2>&1) || {
    printf '%-14s unavailable: %s\n' "$dev" "$(head -n1 <<<"$info")"; return
  }
  card=$(sed -n 's/^[[:space:]]*Card type[[:space:]]*:[[:space:]]*//p' <<<"$info" | head -n1)
  driver=$(sed -n 's/^[[:space:]]*Driver name[[:space:]]*:[[:space:]]*//p' <<<"$info" | head -n1)
  bus=$(sed -n 's/^[[:space:]]*Bus info[[:space:]]*:[[:space:]]*//p' <<<"$info" | head -n1)
  controls=$(v4l2-ctl -d "$dev" --list-ctrls 2>/dev/null | grep -c '0x[0-9a-fA-F]' || true)
  formats=$(v4l2-ctl -d "$dev" --list-formats-ext 2>/dev/null | grep -c '^\[' || true)
  printf '%-14s driver=%-12s controls=%-3s formats=%-3s card=%s bus=%s\n' "$dev" "$driver" "$controls" "$formats" "$card" "$bus"
}

echo '===== DEVICE ENUMERATION ====='
for dev in "${ALL_DEVICES[@]}"; do print_device_summary "$dev"; done

if [[ -z $DEVICE ]]; then
  # Prefer a capture node with controls, then any node with formats.
  for dev in "${ALL_DEVICES[@]}"; do
    if v4l2-ctl -d "$dev" --list-ctrls 2>/dev/null | grep -q '0x[0-9a-fA-F]'; then DEVICE=$dev; break; fi
  done
  if [[ -z $DEVICE ]]; then
    for dev in "${ALL_DEVICES[@]}"; do
      if v4l2-ctl -d "$dev" --list-formats-ext 2>/dev/null | grep -q '^\['; then DEVICE=$dev; break; fi
    done
  fi
fi
[[ -n $DEVICE ]] || { echo 'ERROR: no usable capture/control node found; specify --device.'; exit 1; }
[[ -e $DEVICE ]] || { echo "ERROR: device does not exist: $DEVICE"; exit 1; }
[[ -r $DEVICE && -w $DEVICE ]] || echo "WARNING: $DEVICE is not readable/writable by this user. Check the video group and ACL."

echo; echo "===== SELECTED DEVICE: $DEVICE ====="
ALL_OUTPUT=$(v4l2-ctl -d "$DEVICE" --all 2>&1); ALL_RC=$?
printf '%s\n' "$ALL_OUTPUT"
if ((ALL_RC)); then
  echo 'ERROR: cannot query selected device. If busy, stop the main program/browser preview and retry.'; exit "$ALL_RC"
fi

echo; echo '===== SUPPORTED FORMATS / SIZES / FRAME RATES ====='
v4l2-ctl -d "$DEVICE" --list-formats-ext 2>&1 || true

echo; echo '===== ALL CONTROLS AND MENU VALUES ====='
CTRLS=$(v4l2-ctl -d "$DEVICE" --list-ctrls-menus 2>&1); CTRLS_RC=$?
printf '%s\n' "$CTRLS"
if ((CTRLS_RC)); then echo 'WARNING: control enumeration failed.'; fi

control_line() { sed -n "/^[[:space:]]*$1[[:space:]]/p" <<<"$CTRLS" | head -n1; }
control_exists() { [[ -n $(control_line "$1") ]]; }
get_value() {
  local out
  out=$(v4l2-ctl -d "$DEVICE" --get-ctrl="$1" 2>&1) || { printf 'ERROR:%s' "$out"; return 1; }
  sed -n 's/^[^:]*:[[:space:]]*//p' <<<"$out" | head -n1
}
set_and_verify() {
  local name=$1 value=$2 before after out rc
  before=$(get_value "$name") || { echo "  FAIL read $name: $before"; return 1; }
  out=$(v4l2-ctl -d "$DEVICE" --set-ctrl="$name=$value" 2>&1); rc=$?
  if ((rc)); then echo "  FAIL set $name=$value: $out"; return "$rc"; fi
  after=$(get_value "$name") || { echo "  FAIL verify $name: $after"; return 1; }
  if [[ $after == "$value" ]]; then echo "  PASS $name: $before -> $after";
  else echo "  WARN $name requested=$value readback=$after original=$before (driver clamped/ignored/dynamic)"; fi
}
restore_value() {
  local name=$1 value=$2 out
  out=$(v4l2-ctl -d "$DEVICE" --set-ctrl="$name=$value" 2>&1) || echo "  RESTORE FAIL $name=$value: $out"
}

echo; echo '===== EXPOSURE CONTROL DIAGNOSIS ====='
for name in exposure_auto exposure_absolute exposure exposure_auto_priority; do
  line=$(control_line "$name")
  if [[ -z $line ]]; then
    echo "MISSING: $name"
  else
    echo "FOUND: $line"
    value=$(get_value "$name") && echo "  current=$value" || echo "  read failed: $value"
    [[ $line == *inactive* ]] && echo '  NOTE: inactive because another automatic control/mode currently owns it.'
    [[ $line == *disabled* ]] && echo '  NOTE: disabled by driver.'
    [[ $line == *read-only* ]] && echo '  NOTE: read-only.'
  fi
done
if ! control_exists exposure_auto; then
  echo 'CAUSE: current C++ requires V4L2_CID_EXPOSURE_AUTO; this camera/node cannot use that implementation.'
fi
if ! control_exists exposure_auto_priority; then
  echo 'CAUSE: current C++ unconditionally writes EXPOSURE_AUTO_PRIORITY; its absence makes every apply request report failure.'
fi
if control_exists exposure_auto; then
  echo 'Supported exposure_auto menu (important: many UVC cameras use 1=Manual and 3=Aperture Priority, not 0=Auto):'
  sed -n '/^[[:space:]]*exposure_auto[[:space:]]/,/^[[:space:]]*[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*0x/p' <<<"$CTRLS" | sed '$d'
fi

if ((WRITE_TEST)); then
  echo; echo '===== WRITE / READBACK / RESTORE TEST ====='
  echo 'Each original value is restored immediately. Dependent/inactive controls are skipped.'
  control_re='^[[:space:]]*([a-zA-Z_][a-zA-Z0-9_]*)[[:space:]]+0x[0-9a-fA-F]+[[:space:]]+\(([^()]*)\)[[:space:]]*:[[:space:]]*(.*)$'
  while IFS= read -r line; do
    [[ $line =~ $control_re ]] || continue
    name=${BASH_REMATCH[1]}; type=${BASH_REMATCH[2]}; attrs=${BASH_REMATCH[3]}
    if [[ $attrs == *read-only* || $attrs == *disabled* || $attrs == *inactive* || $type == button || $type == string || $type == bitmask || $type == int64 ]]; then
      echo "SKIP $name ($type): $attrs"; continue
    fi
    original=$(get_value "$name") || { echo "SKIP $name: cannot read ($original)"; continue; }
    candidates=()
    if [[ $type == boolean ]]; then
      candidates=(0 1)
    elif [[ $type == menu || $type == integer-menu ]]; then
      while IFS= read -r v; do candidates+=("$v"); done < <(
        sed -n "/^[[:space:]]*$name[[:space:]]/,/^[[:space:]]*[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*0x/p" <<<"$CTRLS" |
        sed -n 's/^[[:space:]]*\([0-9-][0-9-]*\):.*/\1/p')
    elif [[ $attrs =~ min=(-?[0-9]+).*max=(-?[0-9]+).*step=([0-9]+).*default=(-?[0-9]+) ]]; then
      min=${BASH_REMATCH[1]}; max=${BASH_REMATCH[2]}; step=${BASH_REMATCH[3]}; def=${BASH_REMATCH[4]}
      mid=$(( min + ((max-min)/2/step)*step ))
      candidates=("$min" "$def" "$mid" "$max")
    else
      echo "SKIP $name: unparsed range ($attrs)"; continue
    fi
    echo "TEST $name ($type), original=$original, candidates=${candidates[*]:-none}"
    for value in "${candidates[@]}"; do set_and_verify "$name" "$value" || true; done
    restore_value "$name" "$original"
    restored=$(get_value "$name") || true
    echo "  restored readback=${restored:-unavailable}"
  done <<<"$CTRLS"
fi

echo; echo '===== RESULT HINTS ====='
cat <<'HINTS'
- "Permission denied": run as a user in the video group or with the device ACL.
- "Device or resource busy": stop the project process and all camera viewers first.
- Missing control: the driver/camera does not expose it; software must treat it as optional.
- "inactive": switch the related automatic mode off before setting the manual value.
- "Invalid argument": value/menu mode is unsupported, outside range, or wrong control ID.
- A successful set with different readback means the driver clamped, quantized, or ignored it.
HINTS
printf '\nDone. Send this log for analysis: %s\n' "$LOG"
