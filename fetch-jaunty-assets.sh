#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

URL="https://old-releases.ubuntu.com/ubuntu/pool/main/h/human-theme/human-theme_0.28.8_all.deb"
DEB="$WORK/human-theme_0.28.8_all.deb"

echo "Downloading original Ubuntu 9.04 Human theme..."
curl -fL "$URL" -o "$DEB"

cd "$WORK"
ar x "$DEB"
tar -xf data.tar.*

SRC="$WORK/usr/share/themes/Human/metacity-1"
DEST="$ROOT/assets"

mkdir -p "$DEST"

for f in   icon_close.png   icon_close_u.png   icon_minimize.png   icon_minimize_u.png   icon_maximize.png   icon_maximize_u.png   icon_restore.png   icon_restore_u.png
do
  test -f "$SRC/$f"
  cp -f "$SRC/$f" "$DEST/$f"
done

echo "Extracted original Jaunty Human button assets to:"
echo "  $DEST"
