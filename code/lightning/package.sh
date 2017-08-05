#!/bin/bash
set -e

TEAMID=${TEAMID:-"TEAMID"}
TARGET="icfp-$TEAMID.tar.gz"

rm "$TARGET"

tar -czf "$TARGET" \
    install \
    PACKAGES \
    README \
    src/player.py
