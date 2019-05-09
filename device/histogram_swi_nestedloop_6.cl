#ifndef UNROLL
#error "no UNROLL defined"
#endif


__attribute__((reqd_work_group_size(1,1,1)))
__kernel void kern_histogram(
        __global
#ifdef VOLATILE
volatile
#else
        const
#endif
        unsigned int* restrict src, __global unsigned int* restrict histogram, const unsigned int length) {
    __local unsigned int loc_hist[UNROLL][128];
#pragma unroll
    for(int i = 0; i < UNROLL; i++){
#pragma unroll
        for(int j = 0; j < 128; j++){
            loc_hist[i][j] = 0;
        }
    }
#ifdef MEMFENCE
    mem_fence(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
#endif
    int exit = (length % UNROLL == 0) ?  (length / UNROLL) : (length / UNROLL) + 1;
#pragma unroll 1
    for (int i = 0; i < exit; i++) {
#ifdef PART_J_UNROLL
#pragma unroll 4
#else
    #pragma unroll
#endif
        for (int j = 0; j < UNROLL; j++){
            int index = i * UNROLL + j;
            bool valid = index < length;
            unsigned int current = valid ? src[index] : 0;
#pragma unroll
            for(int radix = 0; radix < 8; radix++){
                int shift = radix * 4;
                int offset = radix * 16;
                unsigned int hist_key = ((current >> shift) & 0xF) + offset;
                loc_hist[j][hist_key] += valid ? 1 : 0;
            }
        }
#ifdef MEMFENCE
        mem_fence(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
#endif
    }

#pragma unroll
    for(int i = 0; i < 128; i++){
        int sum = 0;
#pragma unroll
        for(int j = 0; j < UNROLL; j++){
            sum += loc_hist[j][i];
        }
        histogram[i] = sum;
    }
}