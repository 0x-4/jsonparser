:3
A simple JSON parser written in C++17.

Features

* Parses objects, arrays, strings and numbers
* Supports booleans and null
* Handles nested JSON structures
* Detects invalid JSON and reports errors
* No external libraries required

Build

g++ -std=c++17 -O2 -Wall -Wextra -pedantic main.cpp -o jsonparser

Usage

Pipe JSON input into the program. It will tell you whether the input is valid.
