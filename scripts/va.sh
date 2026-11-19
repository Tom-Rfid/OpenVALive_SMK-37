#!/bin/sh
# OpenVALive - manage the va/ submodule (OpenVALive CORE: the DSP, the engines, the Live Core, the app)
#
#   sh scripts/va.sh init              fetch va/ at the version this repository pins (after a clone)
#   sh scripts/va.sh status            the pinned version, and whether va/ has moved or has local changes
#   sh scripts/va.sh tags              the releases of OpenVALive CORE
#   sh scripts/va.sh update [BRANCH]   move va/ to the latest commit of BRANCH (default: dev)
#   sh scripts/va.sh use REF           move va/ to a tag (v0.4.0), a branch or a commit: to go back, too
#   sh scripts/va.sh reset             put va/ back where this repository pins it (undo update / use)
#
# update and use only move va/: run the tests, then commit the new pin:
#   git add va && git commit -m "va: <version>"
set -eu
cd "$(dirname "$0")/.."

pinned() { git ls-files -s va | awk '{print $2}'; }   # the index: HEAD's pin, or a staged new one
here()   { git -C va rev-parse HEAD 2>/dev/null || echo none; }
name()   { git -C va describe --tags --always "$1" 2>/dev/null || echo "$1"; }

case "${1:-status}" in
init)
    git submodule update --init va
    echo "va/ at $(name "$(here)")"
    ;;
status)
    P=$(pinned); H=$(here)
    echo "pinned : $(name "$P")  ($P)"
    echo "va/    : $(name "$H")  ($H)"
    [ "$P" = "$H" ] || echo "va/ has moved: test, then  git add va && git commit   (or  sh scripts/va.sh reset)"
    [ -z "$(git -C va status --porcelain)" ] || echo "va/ has local changes (commit them in OpenVALive_CORE first)"
    ;;
tags)
    git -C va fetch -q --tags origin
    git -C va tag --sort=-v:refname | head -20
    ;;
update)
    B="${2:-dev}"
    git -C va fetch -q origin "$B"
    git -C va checkout -q --detach FETCH_HEAD
    echo "va/ now at $(name "$(here)") (latest $B). Test: sh tests/run_tests.sh, then: git add va && git commit -m \"va: $(name "$(here)")\""
    ;;
use)
    R="${2:?usage: sh scripts/va.sh use TAG|BRANCH|COMMIT}"
    git -C va fetch -q --tags origin
    git -C va fetch -q origin "$R" 2>/dev/null || true
    git -C va checkout -q --detach "$R" 2>/dev/null || git -C va checkout -q --detach "origin/$R" 2>/dev/null \
        || git -C va checkout -q --detach FETCH_HEAD
    echo "va/ now at $(name "$(here)"). Test: sh tests/run_tests.sh, then: git add va && git commit -m \"va: $(name "$(here)")\""
    ;;
reset)
    git submodule update --init va
    echo "va/ back at the pinned $(name "$(here)")"
    ;;
*)
    sed -n '2,13p' "$0"; exit 1
    ;;
esac
