#!/bin/bash
set -e

TEAMID=${TEAMID:-"TEAMID"}
TARGET="icfp-$TEAMID.tar.gz"

if [ -f "$TARGET" ]; then
    rm "$TARGET"
fi

tar -czf "$TARGET" \
    install \
    PACKAGES \
    README \
    src/player.py

md5 "$TARGET"
