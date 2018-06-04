#!/bin/bash

[[ $1 == '' ]] && echo "Usage: ./$0 <binary dir>" && exit 1

agent=$1/agent
n_test=1000

echo "---> Consumer agent automated testing ($n_test times)..."
$agent consumer --listen 0.0.0.0 --listen-port 25553 --etcd etcd.recolic.net --etcd-port 25554 &
agent_pid=$!
echo "Sleep 5s to wait for agent(PID=$agent_pid) launching..."
sleep 5

unfinished_consumer_agent_succ_keyword='DOCTYPE'
function failed_msg () {
    echo "$1"
    kill -9 $agent_pid
    exit 2
}
function test_once () {
    curl 'http://127.0.0.1:25553/?test=shit' 2>/dev/null | grep "$unfinished_consumer_agent_succ_keyword" > /dev/null || failed_msg '---> One failed request, exiting...'
}

kill -0 $agent_pid || failed_msg '---> Agent exited before launching test...'
# Do 2 sync test before 1k async test...
test_once
test_once
# Seems OK...
for i in `seq 1 $n_test`; do
    test_once &
done

for job in `jobs -p`
do
    [[ $job == $agent_pid ]] && continue
    wait $job
done
echo "---> Consumer agent testing passed. Killing agent and exit."
kill -9 $agent_pid
