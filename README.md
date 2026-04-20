# order book simulator

C++ order book simulator with price-time priority matching, interactive CLI and unit tests.

<img width="270" height="632" alt="Screenshot 2026-04-20 at 10 55 48 PM" src="https://github.com/user-attachments/assets/df0ab333-2b6d-47f2-ab7d-7950d164bb56" />

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
