#!/usr/bin/env bash
#
# Provision and activate a repo-local Python virtual environment
# with the required dependencies (i.e., protobuf) for the build system.
#
# This script is meant to be sourced, not executed, e.g.:
#   source "$(dirname "$0")/activate_venv.sh"
#
# Behaviour:
#  - If SKIP_PYTHON_VENV is set, do nothing (use the system Python / protobuf).
#  - If the repo's own ".venv" is already the active one, do nothing (e.g. when
#    configure.sh already activated it and then calls a generate script).
#  - Otherwise create (once) and activate an isolated ".venv" at the repository
#    root and install protobuf into it. This takes precedence over any other venv
#    that happens to be active, so the correct protobuf is always used.
#  - If the venv cannot be created (e.g. the python3 venv module is missing) but a
#    compatible system protobuf is already importable, fall back to it instead of
#    failing. This keeps pre-provisioned CI runners / containers working without
#    needing the venv module.
#
# It avoids 'exit' so that a failure here doesn't kill a caller that sourced us.

_VENV_PROTOBUF_SPEC="protobuf>=3.20,<4.0"

# Returns success if the active python3 already has a protobuf in the range
# CMakeLists.txt accepts (>=3.20, <4.0).
_venv_system_protobuf_ok() {
  python3 -c 'import sys
import google.protobuf as p
v = tuple(int(x) for x in p.__version__.split(".")[:2])
sys.exit(0 if (3, 20) <= v < (4, 0) else 1)' 2>/dev/null
}

# Opt-out for users who manage protobuf via their distro / system Python.
if [ -n "${SKIP_PYTHON_VENV:-}" ]; then
  unset -f _venv_system_protobuf_ok 2>/dev/null
  unset _VENV_PROTOBUF_SPEC
  return 0 2>/dev/null || true
fi

# Resolve the repository root relative to this file
_VENV_SELF="${BASH_SOURCE[0]:-$0}"
_VENV_OMIM_PATH="$(cd "$(dirname "$_VENV_SELF")/../.." && pwd)"
_VENV_DIR="$_VENV_OMIM_PATH/.venv"

_venv_cleanup() {
  unset -f _venv_system_protobuf_ok 2>/dev/null
  unset _VENV_PROTOBUF_SPEC _VENV_SELF _VENV_OMIM_PATH _VENV_DIR
  unset -f _venv_cleanup 2>/dev/null
}

if [ ! -f "$_VENV_OMIM_PATH/tools/unix/activate_venv.sh" ]; then
  echo "ERROR: activate_venv.sh could not locate the repository root (got '$_VENV_OMIM_PATH')." >&2
  echo "       Source it from a bash script, or set SKIP_PYTHON_VENV=1 to use system Python." >&2
  _venv_cleanup
  return 1 2>/dev/null || true
fi

# Our own venv is already active and present
if [ "${VIRTUAL_ENV:-}" = "$_VENV_DIR" ] && [ -d "$_VENV_DIR" ]; then
  _venv_cleanup
  return 0 2>/dev/null || true
fi

if [ ! -d "$_VENV_DIR" ]; then
  echo "Creating Python virtual environment at $_VENV_DIR ..."
  if ! python3 -m venv "$_VENV_DIR"; then
    # Remove any partially-created venv so it isn't mistaken for a good one later.
    rm -rf "$_VENV_DIR"
    # Fall back to a compatible system protobuf if one is available.
    if _venv_system_protobuf_ok; then
      echo "WARNING: could not create a virtual environment; falling back to the" >&2
      echo "         compatible protobuf already available in the system Python." >&2
      _venv_cleanup
      return 0 2>/dev/null || true
    fi
    echo "ERROR: failed to create a Python virtual environment at $_VENV_DIR, and no" >&2
    echo "       compatible system protobuf (>=3.20, <4.0) was found." >&2
    echo "       Install the python3 venv module (e.g. 'apt install python3-venv')," >&2
    echo "       or install protobuf and set SKIP_PYTHON_VENV=1 to use system Python." >&2
    _venv_cleanup
    return 1 2>/dev/null || true
  fi
fi

# Activate it (handle both Unix and Windows Git Bash layouts).
if [ -f "$_VENV_DIR/bin/activate" ]; then
  # shellcheck disable=SC1091
  source "$_VENV_DIR/bin/activate"
elif [ -f "$_VENV_DIR/Scripts/activate" ]; then
  # shellcheck disable=SC1091
  source "$_VENV_DIR/Scripts/activate"
else
  echo "ERROR: could not find an activate script in $_VENV_DIR" >&2
  _venv_cleanup
  return 1 2>/dev/null || true
fi

# Install protobuf
if ! python3 -m pip install -q "$_VENV_PROTOBUF_SPEC"; then
  echo "ERROR: failed to install $_VENV_PROTOBUF_SPEC into the venv" >&2
  _venv_cleanup
  return 1 2>/dev/null || true
fi

_venv_cleanup
