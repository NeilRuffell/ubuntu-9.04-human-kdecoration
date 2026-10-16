#!/usr/bin/env bash
# Build Ubuntu 9.04 Human for the locally installed Plasma 6 / KDecoration3.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

for cmd in cmake ninja curl ar tar; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        printf 'Missing required tool: %s\nSee UBUNTU.md for dependencies.\n' "$cmd" >&2
        exit 1
    fi
done

if ! command -v c++ >/dev/null 2>&1; then
    echo "Missing C++ compiler. See UBUNTU.md." >&2
    exit 1
fi

if [[ ! -f assets/icon_close.png ]]; then
    bash "$ROOT/fetch-jaunty-assets.sh"
fi

cmake -S "$ROOT" -B "$ROOT/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr

cmake --build "$ROOT/build"
printf '\nBuild complete. To install:\n  sudo cmake --install "%s/build"\n  kbuildsycoca6\n' "$ROOT"
