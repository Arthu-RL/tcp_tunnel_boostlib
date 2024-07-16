#!/bin/bash

set -e

RUN=$1

handle_failure() {
    echo -e "Compilation \033[1;31merror\033[1;0m at $(date '+%A day %d at %H:%M:%S')"
    exit 1
}

trap 'handle_failure' ERR

g++ -std=c++11 -Wall -Wextra -o tcp_server tcp_server.cpp -lboost_system

echo -e "Compilation \033[1;32mfinished\033[1;0m at $(date '+%A day %d at %H:%M:%S')"

if [[ -n "$RUN" && "$RUN" -eq 1 ]]; then 
    ./tcp_server 0.0.0.0 25565
fi