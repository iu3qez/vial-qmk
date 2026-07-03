#!/bin/bash
# SessionStart hook for Claude Code on the web.
# Sets up a full QMK/Vial build+test environment: AVR/ARM toolchain, an
# isolated Python venv (via uv) with the `qmk` CLI, and the git submodules
# required to compile firmware.
#
# Focus board: luma40 (compile with `qmk compile -kb <path> -km vial`).
set -euo pipefail

# --- Only run inside the Claude Code remote (web) environment -----------------
if [ "${CLAUDE_CODE_REMOTE:-}" != "true" ]; then
    echo "Not a remote session; skipping environment setup."
    exit 0
fi

REPO="${CLAUDE_PROJECT_DIR:-$(pwd)}"
cd "$REPO"

# --- 1. System toolchain (AVR + ARM) ------------------------------------------
# apt is idempotent: already-installed packages are skipped, so this stays fast
# on cached containers.
echo ">> Installing AVR/ARM toolchain via apt..."
sudo apt-get update -qq
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential git unzip wget zip \
    gcc-avr avr-libc binutils-avr \
    gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi \
    dfu-util dfu-programmer avrdude \
    libhidapi-hidraw0 libusb-dev

# --- 2. uv (fast, isolated Python) --------------------------------------------
# uv is normally preinstalled in the remote image; install it as a fallback.
if ! command -v uv >/dev/null 2>&1; then
    echo ">> Installing uv..."
    curl -LsSf https://astral.sh/uv/install.sh | sh
    export PATH="$HOME/.local/bin:$PATH"
fi

# --- 3. Python venv with QMK CLI + dev deps -----------------------------------
# A dedicated venv avoids Debian's patched setuptools, which cannot build some
# QMK CLI deps (e.g. `halo`). `uv pip install` is incremental, so re-runs are
# cheap once the container is cached.
echo ">> Creating Python venv and installing QMK CLI + dev deps..."
uv venv --python 3.11 "$REPO/.venv"
# shellcheck disable=SC1091
source "$REPO/.venv/bin/activate"
uv pip install -r "$REPO/requirements-dev.txt" qmk

# --- 4. Git submodules (needed to compile firmware) ---------------------------
# ChibiOS/LUFA/etc. are required by the C build. Cached after the first run.
echo ">> Initializing git submodules..."
git config --global --add safe.directory "$REPO" || true
git submodule sync --recursive
git submodule update --init --recursive

# --- 5. Claude Code plugins (superpowers) -------------------------------------
# The repo declares the `superpowers` plugin in .claude/settings.json, but this
# remote image sets SKIP_PLUGIN_MARKETPLACE=true, which disables the automatic
# "install declared plugins at session start" step. The plugin cache also lives
# in the ephemeral container home (~/.claude/plugins), so it must be repopulated
# on every fresh container. We do it here, non-interactively, overriding the
# skip flag only for these commands. Failures are non-fatal: a missing plugin
# must never block the firmware build environment.
if command -v claude >/dev/null 2>&1; then
    echo ">> Installing Claude Code plugins (superpowers)..."
    SKIP_PLUGIN_MARKETPLACE=false claude plugin marketplace add \
        obra/superpowers-marketplace 2>&1 | sed 's/^/   /' || \
        echo "   (marketplace add skipped/failed; continuing)"
    SKIP_PLUGIN_MARKETPLACE=false claude plugin install \
        superpowers@superpowers-marketplace --scope project 2>&1 | sed 's/^/   /' || \
        echo "   (plugin install skipped/failed; continuing)"
fi

# --- 6. Persist environment for the session -----------------------------------
if [ -n "${CLAUDE_ENV_FILE:-}" ]; then
    {
        echo "export VIRTUAL_ENV=\"$REPO/.venv\""
        echo "export PATH=\"$REPO/.venv/bin:\$PATH\""
        echo "export QMK_HOME=\"$REPO\""
    } >> "$CLAUDE_ENV_FILE"
fi

echo ">> Environment ready. qmk: $(command -v qmk)"
