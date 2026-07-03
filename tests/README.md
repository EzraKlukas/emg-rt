# Tests

This suite is correctness-focused and uses tiny synthetic data with explicit
golden expectations. It intentionally does not depend on repository `.bin`
fixtures for tractable DSP/decomposition examples.

Directory map:

- `buffer/`: `AcquisitionRingBuffer`, `RingMatrix`, and `RingVector` storage,
  wraparound, logical order, timestamp/sample alignment, and read/write head
  movement.
- `dsp/`: temporal extension, extension delay ordering, wrapped logical input,
  initial rolling sums, row-wise demeaning, and combined extend + demean cases.
- `decomposition/`: pulse-train projection, signed-square normalization,
  wrapped ring output, incremental local maxima, centroid classification, ties,
  and onset adjustment.
- `config/`: temporary YAML plus tiny binary float files for single-grid,
  multi-grid, missing-field, and invalid-dimension loading checks.
- `integration/`: small synthetic workflows through buffer copy, extension,
  demeaning, filter projection, peak detection, and discharge classification.
- `support/`: deterministic matrix builders, ADC conversion helpers, and
  temporary file helpers.

Intentional exclusions:

- No multi-threading or concurrency tests.
- No hardware timing assumptions.
- No tests that require production-only private access or changes to
  `src/`/`include/`.
- No NaN/Inf behavior tests; the current public comments do not specify those
  semantics.

See `KNOWN_GAPS.md` for behavior that should be tested once public seams or
semantic decisions are clarified.
