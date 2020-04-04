#!/bin/bash
g++-9.1 -W -Wall -pedantic --std=c++17 -o script_bot tokenizer.cpp operation_type.cpp compiler.cpp main.cpp 2>&1 |less
