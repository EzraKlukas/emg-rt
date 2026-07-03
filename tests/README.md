# Test Suite Overview

This directory contains deterministic doctest coverage for the real-time EMG
decomposition code. The tests use synthetic data and small hand-computed
expectations; they do not depend on the offline replay `.bin` files.

Groups:

- `buffer/`: `RingMatrix`, `RingVector`, acquisition ring storage, conversion,
  stream-mask gathering, wraparound, and consumer-index reads.
- `dsp/`: temporal extension, row-wise rolling sums, demeaning, and wrapped
  logical input windows.
- `decomposition/`: pulse-train projection, signed-square behavior, inverse
  normalization, local-maximum tracking, and centroid classification.
- `config/`: temporary YAML and binary float files for valid and invalid
  `load_online_decomposer` cases.
- `integration/`: tractable adjacent-stage workflows using synthetic samples.

Run locally from the repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

These tests are intended to expose faults. A failing test should not be weakened
unless the test misunderstood the documented public API.
