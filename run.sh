#!/bin/bash

set -e  # exit on error
set -x  # trace (print commands)

gcc main.c -o main
gcc child.c -o child
printf 'result\n1 2 3\n4 5 6\n' | ./main
cat result
