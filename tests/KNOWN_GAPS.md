# Known Test Gaps

- `dsp::extend` is declared in `include/emg-rt/dsp/extend.h` but no
  non-incremental implementation exists in `src/dsp/extend.cpp`. The suite
  tests `incremental_extend`, which is the documented online path.
- There is no public API for reading the newest `N` samples from
  `AcquisitionRingBuffer` without going through `MultiGridDecomposer` or
  manually inspecting storage. The buffer tests verify storage/head behavior;
  orchestration tests cover the public `get_samples` path.
- `OnlineDecompositionConfig` documents `min_lookback_ms` and
  `min_lookahead_ms` as milliseconds, but the loader multiplies those YAML
  values directly by sampling frequency without dividing by 1000. Tests avoid
  asserting nonzero derived values until the intended units are confirmed.
- The local-maximum comments describe online lookahead behavior but do not
  fully specify endpoint handling at the beginning of a retained pulse window.
  The suite covers interior maxima, larger future values, ties, and row
  independence, but does not assert beginning-of-window endpoint semantics.
- The public discharge API returns a boolean mask, not timestamp/sample-index
  vectors. Tests assert mask positions and onset shifts; exact timestamp output
  would need a public API that carries timestamps into classification.
- `GridDecomposer::decompose` currently couples the full pipeline to production
  classification code. Integration tests include the public stages, but richer
  whole-pipeline history/lookahead alignment tests may need clarified buffer
  head ownership semantics.
