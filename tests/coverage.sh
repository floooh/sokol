#!/bin/sh
# LLM maintained.
#
# Builds a mock-backend test preset with clang source-based coverage enabled,
# runs the test binary, and reports coverage for sokol_gfx.h.
#
#   ./coverage.sh <mock-preset> <test-binary> [function-prefix] [show]
#
#   ./coverage.sh glcore_mock_debug sokol-gl43-test _sg_gl_
#   ./coverage.sh metal_mock_debug sokol-metal-test _sg_mtl_ show
#
# With a function-prefix, prints per-function line coverage for matching
# functions plus a summed total (e.g. for a whole backend section). With
# 'show', also lists the uncovered lines of sokol_gfx.h.
#
# Uses a separate build dir (build/cov_<preset>), regular builds are untouched.
set -e
preset=$1
binary=$2
prefix=$3
mode=$4
if [ -z "$preset" ] || [ -z "$binary" ]; then
    echo "usage: coverage.sh <mock-preset> <test-binary> [function-prefix] [show]"
    exit 1
fi
dir=build/cov_$preset
cmake --preset $preset -B $dir -DSOKOL_COVERAGE=ON > /dev/null
cmake --build $dir --target $binary > /dev/null
rm -f $dir/cov.profraw $dir/cov.profdata
(cd $dir && LLVM_PROFILE_FILE=cov.profraw ./$binary > cov_run.log 2>&1) || { tail -30 $dir/cov_run.log; exit 1; }
tail -2 $dir/cov_run.log
xcrun llvm-profdata merge -sparse $dir/cov.profraw -o $dir/cov.profdata
bin=$dir/$binary
sokol_gfx=$(cd .. && pwd)/sokol_gfx.h
if [ "$mode" = "show" ]; then
    xcrun llvm-cov show $bin -instr-profile=$dir/cov.profdata $sokol_gfx -show-line-counts | grep -E '^ *[0-9]+\| +0\|'
fi
xcrun llvm-cov report $bin -instr-profile=$dir/cov.profdata $sokol_gfx | tail -1
if [ -n "$prefix" ]; then
    # report columns: name regions rmiss rcover lines lmiss lcover ...
    xcrun llvm-cov report $bin -instr-profile=$dir/cov.profdata -show-functions $sokol_gfx \
        | awk -v p=":$prefix" 'index($1, p) {
            n = $1; sub(/^[^:]*:/, "", n);
            printf "%-60s %6d %6d %8s\n", n, $5, $6, $7;
            lines += $5; miss += $6
        } END {
            if (lines > 0) printf "%s* total: %d lines, %d missed, %.2f%% covered\n", substr(p, 2), lines, miss, 100.0 * (lines - miss) / lines
        }'
fi
