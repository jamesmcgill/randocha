# randocha

Header only fast x86-64 AES-NI based Random Number Generator

<img src="randocha.png" width="640"></img>

Originally designed for graphics rendering and therefore prioritised for performance.
**Not** suitable for any cryptography workloads.  

### Executable Sources
Along with the header library, the source for three executables are provided:
+ Benchmark tool
+ Distribution visualization tool
+ CSV exporter
  
##### Benchmark Tool
The benchmark is RDTSC/RDTSCP based with Variance/Standard Deviation analysis based on the Intel Benchmarking Whitepaper (see references)  

The benchmark compares a baseline run against:
+ Radocha (this library)
+ Intel SSE based generator (see references)
+ Tiny Encryption Algorithm (TEA) implementation
+ Mersenne Twister (MT) implementation (from the C++ STL)

##### Distribution visualization Tool
Outputs statistics to the console comparing the distribution of the various random number generators (same generators listed in the benchmark tool)  
This is used to confirm a uniform distribution is achieved and that the spread reaches the full range [0 -> 1)
This tool also generates bmp images visualizing the white noise properties of each generator (image above)


##### CSV exporter
Simply generates a CSV file with 100k random values

### Requirements
+ As it's based on AES-NI, it requires an Intel/AMD CPU from 2010 onwards (no ARM support at the moment). Support for these instructions is checked at runtime. 
+ CMake (if you wish to use the provided build system)
+ Tested on MSVC 2017 and 2019 with Intel i5-2500k.
+ Tested on gcc v7.4 with Intel i7-8650U.

### Building
+ On Windows run the provided `build.bat` from within the MSVC command prompt (`vcvarsall.bat`)
+ On Linux run the provided `build.sh`

### References
+ Intel Whitepaper on benchmarking: https://www.intel.com/content/dam/www/public/us/en/documents/white-papers/ia-32-ia-64-benchmark-code-execution-paper.pdf  
+ Intel SSE based random number generator: https://software.intel.com/en-us/articles/fast-random-number-generator-on-the-intel-pentiumr-4-processor/  
+ Manny Ko's GDC presentation Parallel Random Number Generation: https://gdcvault.com/play/1022146/Math-for-Game-Programmers-Parallel  
+ TEA: https://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm  
+ Standard Deviation and Variance explanation: https://www.mathsisfun.com/data/standard-deviation.html  

