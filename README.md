## Pipeline overview

The project is structured around one boundary: `SignalRingBuffer`.

Raw EMG samples enter the system through this buffer. Today, those samples come
from an offline binary file for replay. Later, they should come from a live
acquisition thread on the Jetson. Once samples are in the ring buffer, the rest
of the decomposition pipeline is intended to be the same.

The online decomposition path is:

1. Write timestamped raw samples into `SignalRingBuffer`.
2. Copy the newest samples for each grid into that grid's working buffer.
3. Temporally extend the grid signal by stacking delayed copies of active
   channels.
4. Demean each extended row using rolling historical sums.
5. Project trained motor-unit filters to produce pulse trains.
6. Detect candidate local maxima in each pulse train.
7. Classify candidates as discharges using trained spike/noise centroids.

`SignalRingBuffer` stores raw acquisition-facing data.

`GridBuffers` / `GridWorkspace` stores per-grid decomposition working memory.

The offline replay code is therefore not a separate algorithm. It is only a
temporary producer that lets the online decomposition pipeline be tested before
live sensor integration.
