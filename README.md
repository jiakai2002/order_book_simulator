# order book simulator

C++ order book simulator with price-time priority matching, interactive CLI and unit tests.

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
