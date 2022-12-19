![SplinterDB Project Logo](docs/images/splinterDB-logo.png)

## What is SplinterDB?
SplinterDB is a key-value store designed for high performance on fast storage devices.

See
> Alexander Conway, Abhishek Gupta, Vijay Chidambaram, Martin Farach-Colton, Richard P. Spillane, Amy Tai, Rob Johnson:
[SplinterDB: Closing the Bandwidth Gap for NVMe Key-Value Stores](https://www.usenix.org/conference/atc20/presentation/conway). USENIX Annual Technical Conference 2020: 49-63

## Usage
SplinterDB is a library intended to be embedded within another program.  See [Usage](docs/usage.md) for a guide on how to integrate SplinterDB into an application.

SplinterDB is *provided as-is* and is not recommended for use in production until version 1.0. See [Limitations](docs/limitations.md) for details.

## Build
See [Build](docs/build.md) for instructions on building SplinterDB from source.

See [Documentation](docs/README.md) for preliminary documentation.

## Build Commands:

export COMPILER=gcc 

export CC=$COMPILER

export LD=$COMPILER

make clean 

make 

## Test and Evaluation Commands:

make run-tests

INCLUDE_SLOW_TESTS=true make run-tests

perf stat -e cpu-clock, faults ./build/release/bin/driver_test splinter_test --perf --max-async-inflight 0 --num-insert-threads 4 --num-lookup-threads 4 --num-range-lookup-threads 0 --tree-size-gib 2 --cache-capacity-mib 512

perf record ./build/release/bin/driver_test splinter_test --perf --max-async-inflight 0 --num-insert-threads 4 --num-lookup-threads 4 --num-range-lookup-threads 0 --tree-size-gib 2 --cache-capacity-mib 512

perf report

