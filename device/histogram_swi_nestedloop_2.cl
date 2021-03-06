

__attribute__((reqd_work_group_size(1,1,1)))
__kernel void kern_histogram(
#ifdef CLCONST
        __constant
#else
__global const
#endif
        unsigned int* restrict src, __global unsigned int* restrict histogram, const unsigned int length){

    unsigned int loc_hist[128] = {0};


    for(int i = 0; i < length; i++){
        unsigned int curr = src[i];

#ifndef static_keys
#pragma unroll
        for(int r = 0; r < 8; r++){
            int keys = ((curr >> (r * 4)) & 0xf) + (r * 16);
            loc_hist[keys]++;
        }
#else
        int keys[8];
        keys[0] =         curr & 0xF;
        keys[1] =        ((curr >> 4) & 0xF) + 16;
        keys[2] =        ((curr >> 8) & 0xF) + 32;
        keys[3] =        ((curr >> 12) & 0xF) + 48;
        keys[4] =        ((curr >> 16) & 0xF) + 64;
        keys[5] =        ((curr >> 20) & 0xF) + 80;
        keys[6] =        ((curr >> 24) & 0xF) + 96;
        keys[7] =        ((curr >> 28) & 0xF) + 112;
        #pragma unroll
        for(int r = 0; r < 8; r++){
            loc_hist[keys[r]]++;
        }
#endif
    }
#pragma unroll
    for(int i = 0; i < 128; i++){
        histogram[i] = loc_hist[i];
    }


}




