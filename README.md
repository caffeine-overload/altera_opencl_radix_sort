# altera_opencl_radix_sort
Accelerating radix sort on the Intel MCP

'runsort.cpp' contains the benchmark. 

The kernel named 'histogram_swi_nestedloop_6_extra' is the final optimised kernel. The other kernels have various levels of 
optimisation. 

If compiling it with icc, one can link it with Intel Integrated Performance Primitives and define 'icpc' in the build options, 
this will add Intel's radix sort to the benchmark. Refer to the makefile for an example. 

Arguments to runsort are positional and are as follows:

 1 .  Path to the aocx file. 
 
 2 .  Whether the OpenCL kernel calculates a cumulative sum of the histogram or not. Single character, 'y' or 'n'. Put 'y' for 
 the final kernel 'histogram_swi_nestedloop_6_extra'.
 
 3 .  The length of the array to sort. Integer. 
 
 4 .  Number of times to repeat the benchmark. Integer.
 
 5 .  The OpenCL platform ID to target.
 
 6 .  The OpenCL device ID to target.
 
 
 'app.py' Is obsolete in this repository. 
