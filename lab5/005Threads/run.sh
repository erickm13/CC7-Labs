#!/bin/bash

THREADS=${1:-4}

echo "Compiling project..."
make

echo "Running with $THREADS threads..."
./log_analyzer access.log $THREADS
