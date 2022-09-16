#!/bin/bash

export TALKER_EXECUTABLE=$1
export LISTENER_EXECUTABLE=$2
export TIMEOUT=2.0  # in seconds

# Avoid issues when trying to expand '~'
export DEFAULT_ROS_ARGS="--disable-external-lib-logs"

function test_isolation
{
    if [ ! -z "${TEST_TMPDIR}" ]; then
        if [ ! -z "$1" ]; then
            # Handle concurrent subtests in TEST_TMPDIR
            export TEST_TMPDIR="${TEST_TMPDIR}/$1"
            mkdir -p "${TEST_TMPDIR}"
        fi
    fi

    # Start talker in the background
    TALKER_ROS_ARGS=$DEFAULT_ROS_ARGS
    $TALKER_EXECUTABLE --ros-args $TALKER_ROS_ARGS -- &
    TALKER_PID=$!

    sleep 1.0  # Let the talker start up

    # Run listener in the foreground
    LISTENER_ROS_ARGS="$DEFAULT_ROS_ARGS --param timeout:=$TIMEOUT"
    $LISTENER_EXECUTABLE --ros-args $LISTENER_ROS_ARGS
    return_code=$?  # Keep listener return code

    # Kill talker in the background
    if ! kill $TALKER_PID; then
        # Talker exited too early, likely with an error
        wait $TALKER_PID
        return_code=$?  # Keep talker return code
    fi

    return $return_code
}
export -f test_isolation  # Export to subshells

# Run as many concurrent tests as shards are available
[ -z "${TEST_SHARD_STATUS_FILE-}" ] || touch "$TEST_SHARD_STATUS_FILE"

# Run as many SUBTESTS_PER_TEST as requested, across WORKER_COUNT processes
SUBTESTS_PER_TEST=${SUBTESTS_PER_TEST:-10}
WORKER_COUNT=${WORKER_COUNT:-$(nproc --all)}

seq 0 ${SUBTESTS_PER_TEST} | xargs  -n 1 -P ${WORKER_COUNT} -I{} bash -c "test_isolation subtest_{}_of_${SUBTESTS_PER_TEST}"
