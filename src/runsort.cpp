
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY
#define CL_HPP_ENABLE_EXCEPTIONS

#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <utility>
#include <string>
#include <vector>
#include <CL/cl.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include <sstream>

#ifdef icpc
#include "ipp.h"
#include "ipps.h"
#endif

#define err std::cerr
#define puts std::cout
#define br std::endl
#define ti(x) std::stoi(argv[x])

typedef std::chrono::high_resolution_clock::time_point TimeVar;

#define duration(a) std::chrono::duration_cast<std::chrono::nanoseconds>(a).count()
#define timeNow() std::chrono::high_resolution_clock::now()


template<typename F, typename... Args>
double funcTime(F func, Args&&... args){
    TimeVar t1=timeNow();
    func(std::forward<Args>(args)...);
    return duration(timeNow()-t1);
}



cl::Context context;
cl::CommandQueue queue;
cl::Kernel kern;


cl::Program get_prog(const char* kernel_path, cl::Context context, bool src, std::vector<cl::Device> devicev){
    cl_int cl_status;
    std::ifstream file(kernel_path, std::ios::in | std::ios::binary | std::ios::ate);
    char *bins;
    int size;

    if (file.is_open()) {
        size = file.tellg();
        bins = new char[size];
        file.seekg(0, file.beg);
        file.read(bins, size);
        file.close();
    } else {
        std::cerr << "Source not found or failed to open [" << kernel_path << "]" << std::endl;
        exit(-1);
    }

    cl::Program program;
    if(src){
        cl::Program::Sources sources;

        std::pair<const char *, size_t> pair =
                std::make_pair((const char *) bins, size);
        sources.push_back(pair);

        program = cl::Program(context, sources);
    }else {

        cl::Program::Binaries binaries;

        std::pair<const char *, size_t> pair =
                std::make_pair((const char *) bins, size);
        binaries.push_back(pair);

        program = cl::Program(context, devicev, binaries);

    }

    cl_status = program.build(devicev);

    if (cl_status != CL_SUCCESS) {
        std::cerr << "Build fail: " << cl_status << std::endl;

        std::string name     = devicev[0].getInfo<CL_DEVICE_NAME>();
        std::string buildlog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devicev[0]);
        std::cerr << "Build log" << ":" << std::endl
                  << buildlog << std::endl;

        exit(-1);
    }

    return program;
}

void sort_single_radix(unsigned int* src, unsigned int* dest, unsigned int* count, int shift, int length){
    for(int i = length - 1; i > -1; i--){
        int key = (src[i] >> shift) & 0xF;
        count[key]--;
        dest[count[key]] = src[i];
    }
}

void fpgasort(unsigned int* src, unsigned int* tmp, cl::Buffer array, int length, bool cumsum){
    cl_int s;
    cl::Buffer histogram(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, sizeof(int) * 128,  NULL, &s);

    if (s != CL_SUCCESS) {
        std::cerr << "Opencl Create Buffer Error: " << s << br;
        exit(-1);
    }



    unsigned int* kernelresult = (unsigned int*)queue.enqueueMapBuffer(histogram, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, sizeof(int) * 128);
    std::fill(kernelresult, kernelresult + 128, 0);



    kern.setArg(0, array);
    kern.setArg(1, histogram);
    kern.setArg(2, length);
    cl::Event evt;
    s = queue.enqueueNDRangeKernel(kern, cl::NullRange, cl::NDRange(1,1,1), cl::NDRange(1,1,1), NULL, &evt);
    evt.wait();


    if(!cumsum){
        for(int i = 0; i < 8; i++){
            int offset = i * 16;
            for(int j = 1; j < 16; j++){
                kernelresult[j + offset] += kernelresult[j + offset - 1];
            }
        }
    }

    for(int i = 0; i < 4; i++){
        int r = i * 2;
        sort_single_radix(src, tmp, kernelresult + (r * 16), r * 4, length);
        r = r + 1;
        sort_single_radix(tmp, src, kernelresult + (r * 16), r * 4, length);
    }

}



/*
 * aocx path
 * with_cumsum?
 * length
 * reps
 * platform
 * dev
 */
int main(int argc, char* argv[]){
    const char* kernel_path = argv[1];
    bool cumsum = argv[2][0] == 'y';
    int length = ti(3);
    int reps = ti(4);
    int platform_id = ti(5);
    int device_id = ti(6);



    size_t dsize = sizeof(int) * length;



    //Get platform and device
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    cl::Platform platform = platforms[platform_id];
    std::vector<cl::Device> devices;

    cl_int cl_status;
    cl_status = platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
    if (cl_status != CL_SUCCESS)
    {
        std::cout << "Problem with getting devices from platform"
                  << " [" << platform_id << "] " << platform.getInfo<CL_PLATFORM_NAME>()
                  << " error: [" << cl_status << "]" << std::endl;
    }
    cl::Device device = devices[device_id];
    std::vector<cl::Device> devicev;
    devicev.push_back(device);

    //Create command queue
    context = cl::Context(devices);

    queue = cl::CommandQueue(context, device);


    cl::Program program = get_prog(kernel_path, context, false, devicev);

    kern = cl::Kernel(program, "kern_histogram");

    for(int rep = 0; rep < reps; rep++) {

        cl_int s;
        cl::Buffer databuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, dsize, NULL, &s);
        if (s != CL_SUCCESS) {
            std::cerr << "Opencl Create Buffer Error: " << s << br;
            exit(-1);
        }

        unsigned int *arr_src = (unsigned int *) queue.enqueueMapBuffer(databuffer, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE,
                                                                        0, dsize);

        for (int j = 0; j < length; j++) {
            arr_src[j] = rand();
        }

        unsigned int arr_src_copy1[length];
        unsigned int arr_src_copy2[length];
        memcpy(arr_src_copy1, arr_src, dsize);
        memcpy(arr_src_copy2, arr_src, dsize);

        unsigned int tmp[length];
        TimeVar t1 = timeNow();
        fpgasort(arr_src, tmp, databuffer, length, cumsum);
        double fpga = duration(timeNow() - t1);
        t1 = timeNow();
        std::sort(arr_src_copy1, arr_src_copy1 + length);
        double stdlib = duration(timeNow() - t1);
        bool correct = std::equal(arr_src, arr_src + length, arr_src_copy1);
#ifdef icpc
        ippInit();
        int pbufsize;
        ippsSortRadixGetBufferSize(length, ipp32u, &pbufsize);
        Ipp8u pBuffer[pbufsize];
        t1 = timeNow();
        ippsSortRadixAscend_32u_I((Ipp32u*)arr_src_copy2, length, pBuffer);
        double ipp =  duration(timeNow()-t1);
        puts << kernel_path << "\t" << length << "\t" << stdlib << "\t" << ipp << "\t" << fpga << "\t" << correct << br;
#else
        puts << kernel_path << "\t" << length << "\t" << stdlib << "\t" << fpga << "\t" << correct << br;
#endif
    }
}
