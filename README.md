# order book simulator

C++ order book simulator with price-time priority matching and interactive CLI.

## Build

```bash
cmake -B build && cmake --build build
```

## Run

```bash
./build/sim
```

## Commands

```
limit <buy|sell> <price> <qty>
market <buy|sell> <qty>
quit
```

## Test

```bash
ctest --test-dir build
```

## Structure

```
include/   order_book.hpp
src/       order_book.cpp  main.cpp
tests/     test.cpp
```
