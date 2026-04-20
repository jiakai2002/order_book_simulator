# order book simulator

C++ order book simulator with price-time priority matching, interactive CLI and unit tests.

<img width="262" height="624" alt="Screenshot 2026-04-20 at 10 55 48 PM" src="https://github.com/user-attachments/assets/0f2b916a-bc59-4e73-8f6d-103a029d80c8" />

## build

```bash
cmake -B build && cmake --build build
```

## run

```bash
./build/sim
```

## commands

```
limit <buy|sell> <price> <qty>
market <buy|sell> <qty>
quit
```

## gtest

```bash
./build/tests
```

## structure

```
include/   order_book.hpp
src/       order_book.cpp  main.cpp
tests/     test.cpp
```
