# CRISPR todo

- rewrite the patcher
    - step 1: drop LIEF, we use it only for convenience
    - step 2: rewrite it in C++ and make CRISPR a single executable
- invoke lld in-process
- size optimizations
- support PE/COFF binaries
- support MachO binaries
- support codegen for other architectures
- create a comprehensive testsuite

## Size optimization ideas

Here are some ideas which we could implement to avoid increasing the memory footprint and/or file size of the binary.
TODO: document the other ideas we discussed.

### Extending segments

Often segments are not placed contiguously in memory due to alignment.
If the gap between a segment and the next is big enough, we could extend the first segment
and place our new data there, to avoid extending and moving the PHDRs.

### GNU_NOTES

The `GNU_NOTES` program header can be replaced with a different PHDR.
If we need to add a single PHDR we can replace `GNU_NOTES` instead and avoid moving PHDRs.

### Fitting the code over multiple functions/places

We could get a list of unneded ranges in the binary, and try to split the new data
to distribute it over them.
We can try to do this either:
- by solving a sort-of knapsack problem, or
- by splitting every function and placing every piece in a different spot, joining pieces with jumps


