#!/bin/bash

[[ $1 == '' ]] && echo "Usage: ./$0 <binary dir>" && exit 1

agent=$1/agent

$agent consumer --listen 0.0.0.0 --listen-port 25553 --etcd etcd.recolic.net --etcd-port 25554 &
agent_pid=$!
echo "Sleep 15s to wait for agent(PID=$agent_pid) launching..."
sleep 15

unfinished_consumer_agent_succ_keyword='<center><h1>301 Moved Permanently</h1></center>'
function failed_msg () {
    echo "$1"
    kill -9 $agent_pid
    exit 2
}
function test_once () {
    curl 'http://127.0.0.1:25553/?test=shit' 2>/dev/null | grep "$unfinished_consumer_agent_succ_keyword" || failed_msg 'EEEEEEEEEEEEEEER'
}

kill -0 $agent_pid || failed_msg 'Agent exited before launching test...'
# Do 2 sync test before 1k async test...
test_once
test_once
# Seems OK...
for i in `seq 1 1000`; do
    test_once &
done

for job in `jobs -p`
do
    wait $job
    echo "$job done."
done
kill -9 $agent_pid
