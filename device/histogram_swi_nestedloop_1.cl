

__attribute__((reqd_work_group_size(1,1,1)))
__kernel void kern_histogram(__global unsigned int* restrict src, __global unsigned int* restrict histogram, const unsigned int length){

#ifdef LOCAL_HIST
    __local unsigned int loc_hist[128];
#pragma unroll
    for(int i = 0; i < 128; i++){
        loc_hist[i] = 0;
    }
#endif

#ifdef LOOP_COALESCE
#pragma loop_coalesce 2
#endif
for (int i = 0; i < length; i++) {
        unsigned int curr;
        curr = src[i];

#ifdef UNROLL
#pragma unroll 8
#endif
        for(int r = 0; r < 8; r++){
            //printf("%d %d \n", i, r);
            //printf("Current: %#010x\n", curr);
            unsigned int key = (curr >> (r * 4)) & 0xF;
            //printf("Key: %#010x\n", key);
            //printf("Access: %d  Offset: %d\n", key + (r * 16), r * 16);
#ifdef LOCAL_HIST
            loc_hist[key + (r * 16)]++;
#else
            histogram[key + (r * 16)]++;
#endif
        }
    }

#ifdef LOCAL_HIST
    #pragma unroll
    for(int i = 0; i < 128; i++){
        histogram[i] = loc_hist[i];
    }
#endif
}




