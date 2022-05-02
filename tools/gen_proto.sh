#!/usr/bin/env bash

app=$(python -c "import os; print(os.path.dirname(os.path.realpath(\"$0\")))")
cd ${app}/..

bazel-dosa/external/nanopb/generator/nanopb_generator.py -I lib/proto/protos/ -D lib/proto/src/protobuf/ lib/proto/protos/*.proto
