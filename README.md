# nanorq for Visual C

nanorq is a compact, performant implementation of the RaptorQ fountain codes, which is defined in RFC 6330.

# Usable on Microsoft Visual Studio

This is a modified version of [nanorq](https://github.com/sleepybishop/nanorq). 
Because the original nanorq cannot be compiled on Microsoft Visual Studio, 
I changed some points for compatibility with Microsoft Visual C. 
I removed mmap feature, as C-runtime library doesn't support it on Windows OS. 
I added detecting CPU feature (SSSE3 or AVX2) to switch functions automatically. 
When SSSE3 ia available on CPU, the speed becomes 3 times faster. 
When AVX2 ia available on CPU, the speed becomes 5 times faster. 
Though the API usage may be strange for Windows programers, this is worth to use.

# Notes

As I changed some, gcc may not compile this modified version. 
If you use nanorq with gcc on Linux or Unix, you would better use original version.
