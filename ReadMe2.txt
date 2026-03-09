
nanorq
RaptorQ RFC 6330 library for gcc
https://github.com/sleepybishop/nanorq

MIT License

Copyright (c) 2020 Joseph Calderon

--------------------------------------------------------------------------------

 Modified by Yutaka Sawada at 2026.
I changed some points for compatibility with Microsoft Visual C.
I removed mmap feature, as C-runtime library doesn't support it on Windows OS.

I added detecting CPU feature (SSSE3 or AVX2) to switch functions automatically.
When SSSE3 ia available on CPU, the speed becomes 3 times faster.
When AVX2 ia available on CPU, the speed becomes 5 times faster.

I combined 2 projects for simple usage on Microsoft Visual Studio.
https://github.com/sleepybishop/nanorq
https://github.com/sleepybishop/oblas

I put a sample code (main.c) to test usage.
