#!/bin/bash

function test_once () {
    curl 'http://127.0.0.1:25553/?test=shit' 2>/dev/null | grep "<!DOCTYPE html>" || failed_msg 'EEEEEEEEEEEEEEER'
}

# Do 2 sync test before 1k async test...
test_once
test_once
# Seems OK...
for i in `seq 1 1000`; do
    test_once &
done

