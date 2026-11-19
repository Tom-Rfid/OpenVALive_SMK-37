#!/bin/sh
# OpenVALive SMK-37 - every automatic test
#
#   FM1_BASE=~/fm1-base SMK37_OPEN=~/smk37_open sh tests/run_tests.sh
#     1. OpenVALive CORE's own tests (the va/ submodule: core, Live Core, audio, no-libc)
#     2. the SMK-37 app on the host: a fake SDK and panel play its keys, pads, faders, wheels and buttons
#
# Prints one PASS / FAIL / SKIP line per test; exits with 1 if any test fails.
set -u
cd "$(dirname "$0")/.."

B=build/tests
mkdir -p "$B"
FAILS=0
pass() { echo "PASS  $1"; }
fail() { echo "FAIL  $1"; FAILS=$((FAILS + 1)); }
skip() { echo "SKIP  $1"; }

if [ ! -f va/app/openvalive_app.c ]; then
    echo "va/ is empty: run  sh scripts/va.sh init"
    exit 1
fi

# 1. the core's tests, at the pinned version
if sh va/tests/run_tests.sh >$B/va.log 2>&1; then
    pass "OpenVALive CORE $(git -C va describe --tags --always 2>/dev/null): $(grep -c '^PASS' $B/va.log) tests"
else
    grep -v '^PASS' $B/va.log
    fail "OpenVALive CORE tests"
fi

# 2. the SMK-37 app on the host
if [ -n "${FM1_BASE:-}" ] && [ -n "${SMK37_OPEN:-}" ]; then
    if python3 targets/smk37/build.py --test --smk37-open "$SMK37_OPEN" --fm1-base "$FM1_BASE" \
         --out $B/smk37 >$B/smk37.log 2>&1; then
        pass "SMK-37: $(tail -1 $B/smk37.log | sed 's/smk37 host: //')"
    else
        cat $B/smk37.log
        fail "SMK-37: host test"
    fi
else
    skip "SMK-37 host test (set SMK37_OPEN=path/to/smk37_open and FM1_BASE=path/to/fm1-base)"
fi

echo
if [ $FAILS -eq 0 ]; then
    echo "All tests pass."
else
    echo "$FAILS test(s) failed."
    exit 1
fi
