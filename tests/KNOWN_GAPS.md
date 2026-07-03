# Known Gaps

- Concurrency and thread-safety are intentionally untested. The acquisition
  buffer documentation says producer/consumer synchronization has not been
  implemented yet.
- Invalid acquisition read requests are not tested as runtime errors. The
  current code documents index-based reads but uses `assert` for several
  preconditions, while the library target defines `NDEBUG`, so normal builds do
  not provide deterministic error handling for those cases.
- Zero-sized buffers, zero stream counts, out-of-range acquisition masks, and
  onset shifts that underflow destination indices are not tested because the
  public docs do not define exception or rejection semantics for them.
- Full multi-rate EMG/IMU/ADC synchronization is not tested. The public
  acquisition API has separate buffers for sensor types, but no documented
  synchronization policy between them.
- The whole `MultiGridDecomposer::decompose` loop is only covered through
  adjacent-stage integration tests. The current workspace/head conventions for
  where newly projected pulse-train samples should land in retained output
  rings are not documented clearly enough to assert an end-to-end discharge
  index without first clarifying that contract.
