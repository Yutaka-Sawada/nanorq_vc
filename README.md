# nanorq for Visual C

nanorq is a compact, performant implementation of the RaptorQ fountain codes, which is defined in RFC 6330.  
Thanks Joseph Calderon for publishing the useful library.

# Usable on Microsoft Visual Studio

This is a modified version of [nanorq](https://github.com/ursoid-tech/nanorq). 
Because the original nanorq cannot be compiled on Microsoft Visual Studio, 
I changed some points for compatibility with Microsoft Visual C.

To enable atomic functions, you must use Microsoft Visual Studio 2022 (version 17.5 or later).  
It requires compiler option "`/std:c11`" or "`/std:c17`".  
It requires "`/experimental:c11atomics`" as additional compiler option, too.

# Notes

As I changed some, gcc may not compile this modified version. 
If you use nanorq with gcc on Linux or Unix, you would better use [original version](https://github.com/ursoid-tech/nanorq).
