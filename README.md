# CSCE 313 - PA1: Named Pipes

This repository contains my implementation of PA1 for CSCE 313.

## Files:
- `client.cpp` - Main client implementation
- `answer.txt` - Performance analysis and results
- `common.cpp`, `common.h` - Shared utilities
- `FIFORequestChannel.cpp`, `FIFORequestChannel.h` - Communication channel implementation
- `makefile` - Build configuration

## How to run:
```bash
make
./client -p 10 -t 59.004 -e 2  # Single data point
./client -f 1.csv              # File transfer
./client -c -f 5.csv           # New channel file transfer
