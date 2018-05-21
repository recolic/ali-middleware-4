# This file is intended for testing on developing.
# This is NOT a building file.

cagent:
	clang++ -c consumer_agent.cc -I . -I ../lib -DBOOST_COROUTINES_NO_DEPRECATION_WARNING -std=c++14 -o /tmp/c.o
	g++ -c consumer_agent.cc -I . -I ../lib -DBOOST_COROUTINES_NO_DEPRECATION_WARNING -std=c++14 -o /tmp/c.o
	
	clang++ -c main.cc -I . -I ../lib -std=c++14 -o /tmp/c.o
	g++ -c main.cc -I . -I ../lib -std=c++14 -o /tmp/c.o
