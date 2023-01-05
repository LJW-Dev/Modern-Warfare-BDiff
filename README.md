# MWBDiff
A modified implementation of VCDIFF (https://www.researchgate.net/publication/279639182_The_VCDIFF_Generic_Differencing_and_Compression_Data_Format) used by Modern Warfare 2 2022's FastFiles for data compression.

# Usage
There is only one export:

1. `bool bdiff(
    unsigned char* sourceBuffer, size_t sourceBufferSize, 
    unsigned char* patchBuffer, size_t patchBufferSize, 
    unsigned char* outBuffer, size_t outBufferSize);`
    
where sourceBuffer and patchBuffer are pointers to the source and patch files, and outBuffer is a pointer to memory to be filled with the resulting patched FastFile.

outBufferSize must be the EXACT size of the resulting patched FastFile.
