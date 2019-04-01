# randocha

Fast x86-64 AES-NI based Random Number Generator

Originally designed for graphics rendering and therefore prioritised for performance.
**Not** suitable for any cryptography workloads.  

  
Includes a RDTSC/RDTSCP based benchmark with Variance/Standard Deviation analysis based on the Intel Benchmarking Whitepaper:  
https://www.intel.com/content/dam/www/public/us/en/documents/white-papers/ia-32-ia-64-benchmark-code-execution-paper.pdf

### Requirements:
As it's based on AES-NI, it requires an Intel/AMD CPU from 2010 onwards (no ARM support at the moment).  

Currently only supports Microsoft VC++ (GCC/Clang requires only a few changes).  
Tested on MSVC 2017 and 2019 with Intel i5-2500k.

### Building:
+ On Windows run the provided `build.bat` from within the MSVC command prompt (`vcvarsall.bat`)
