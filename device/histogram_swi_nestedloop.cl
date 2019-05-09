
__attribute__((reqd_work_group_size(1,1,1)))
__kernel void kern_histogram(__global unsigned int* restrict src, __global unsigned int* restrict histogram, const unsigned int length){
    for(int j = 0; j < 8; j++){
        int shift = j * 4;
        for (int i = 0; i < length; i++) {
            unsigned int key = (src[i] >> shift) & 0xF;
            histogram[key + (j * 16)]++;
        }
    }
}




