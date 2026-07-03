## Pipeline overview

The project is structured around the acquisition buffer boundary.

Raw sensor samples enter the system through `AcquisitionRingBuffer`. One
`AcquisitionRingBuffer` stores a fixed-duration circular buffer for one sensor
type, while `AcquisitionFrameBuffer` owns the parallel EMG, IMU, and ADC
buffers. Today, EMG samples come from an offline binary file for replay. Later,
they should come from a live acquisition thread on the Jetson. Once samples are
in the acquisition buffer, the decomposition pipeline is intended to be the same.

The online decomposition path is:

1. Write timestamped raw samples into an `AcquisitionRingBuffer`.
2. Convert raw `uint16_t` values to stored floating-point samples while writing;
   the current EMG replay path uses the Intan/RHD microvolt conversion and this
   should not be assumed valid for every sensor type.
3. Use each grid's `AcquisitionMask` to gather selected source stream indices
   into that grid's dense working buffer.
4. Temporally extend the grid signal by stacking delayed copies of active
   channels.
5. Demean each extended row using rolling historical sums.
6. Project trained motor-unit filters to produce pulse trains.
7. Detect candidate local maxima in each pulse train.
8. Classify candidates as discharges using trained spike/noise centroids.

`AcquisitionRingBuffer` stores acquisition-facing data for one sensor type in
sample-major order, with sample indices and timestamps alongside signal values.

`GridWorkspace` stores per-grid decomposition working memory, including the
gathered raw grid window, acquisition indices, timestamps, extended signals,
pulse trains, and classification masks.

The offline replay code is therefore not a separate algorithm. It is only a
temporary producer that lets the online decomposition pipeline be tested before
live sensor integration. The current code does not claim full thread safety or
multi-rate sensor synchronization; ownership and synchronization rules still
need to be made explicit before using the acquisition buffers as live
producer/consumer queues.
