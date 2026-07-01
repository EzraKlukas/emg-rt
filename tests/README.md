# Tests

This directory keeps tests close to the public API area they exercise:

- `utils/`: storage and view helpers.
- `dsp/`: signal preparation helpers.
- `decomposition/`: pulse train, spike detection, and discharge-time logic.
- `support/`: small test-only helpers.

Each `.cpp` file should stay narrow. Prefer one file per function or one file per
small workflow. When a behavior spans multiple functions, add a new file beside
the isolated tests rather than making the isolated tests large.

Correctness notes for this project:

- Test tiny hand-computed matrices first. EMG pipeline bugs often hide in row,
  column, delay, and ring-buffer indexing.
- Include wrapped `RingMatrix` cases when testing anything that reads logical
  columns. Contiguous storage and logical order are different concerns.
- Keep golden expectations explicit in tests; avoid recomputing the expected
  value with the same algorithm under test.
- Separate offline file/config loading tests from realtime processing tests.
  Realtime-facing tests should make allocation and loop bounds obvious.
- Add integration tests only after the isolated function contract is pinned
  down. Pipeline failures are much easier to diagnose when each stage has small
  deterministic examples.
