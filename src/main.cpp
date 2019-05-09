//
// Created by philip on 3/6/19.
//


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

#define err std::cerr
#define puts std::cout
#define br std::endl
#define ti(x) std::stoi(argv[x])

typedef std::chrono::high_resolution_clock::time_point TimeVar;

#define duration(a) std::chrono::duration_cast<std::chrono::nanoseconds>(a).count()
#define timeNow() std::chrono::high_resolution_clock::now()


cl::Context context;
cl::CommandQueue queue;

void chistogram(int* src, int* hist, int l){
    for (int i = 0; i < l; i++) {
        for(int j = 0; j < 8; j++){
            int shift = j * 4;
            int ele = src[i];
            int key = (ele >> shift) & 0xF;
            hist[key + (j * 16)]++;
        }
    }
}

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

/*
 * program path
 * src or bin
 * length of array
 * reps
 * ndr1
 * ndr2
 * ndr3
 * ndrl1
 * ndrl2
 * ndrl3
 * dev or host mem
 * platform
 * dev
*/
int main (int argc, char* argv[]) {
    const char* kernel_path;
    bool src;
    int l;
    int reps;
    bool devmem;
    int platform_id, device_id;


    if(argc < 14){
        err << "Too few args" << br;
        return(-1);
    }
#ifdef debug
    puts << "Processing args" << br;
#endif

    kernel_path = argv[1];
    src = argv[2][0] == 's';
    l = ti(3);
    reps = ti(4);
    const cl::NDRange glob(ti(5), ti(6), ti(7));
    const cl::NDRange loc( ti(8), ti(9), ti(10));
    devmem = argv[11][0] == 'd';
    platform_id = ti(12);
    device_id = ti(13);
#ifdef debug
    puts << "Generating buffers" << br;
#endif

    size_t dsize = sizeof(int) * l;
#ifdef debug
    puts << dsize << br;
#endif
    size_t bufsize = sizeof(int) * 128;

    cl::Buffer buf_in;
    cl::Buffer buf_hist;

    cl_int cl_status;

    int* arr_src = new int[l];
#ifdef debug
    puts << "Platform and device" << br;
#endif
    //Get platform and device
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    cl::Platform platform = platforms[platform_id];
    std::vector<cl::Device> devices;

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
    #ifdef debug
    puts << "Create command queue" << br;
#endif
    //Create command queue
    context = cl::Context(devices);

    if(devmem){
        buf_in = cl::Buffer(context, CL_MEM_READ_WRITE, dsize, NULL, &cl_status);
#ifdef debug
        puts << cl_status << br;
#endif
        buf_hist = cl::Buffer(context, CL_MEM_READ_WRITE, bufsize);
    }else{
        buf_in = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, dsize);
        buf_hist = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, bufsize);
    }

    queue = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE);
#ifdef debug
    puts << "Get kernel" << br;
#endif

    cl::Program program = get_prog(kernel_path, context, src, devicev);

    cl::Kernel kern(program, "kern_histogram");
#ifdef debug
    puts << "Run test" << br;
#endif
    for(int i = 0; i < reps; i++){
        for(int j = 0; j < l; j++){
            arr_src[j] = rand();
        }
        int hist_truth[128] = {0};
        int hist_cl[128] = {0};
#ifdef debug
        puts << "Run c algo" << br;
#endif
        chistogram(arr_src, hist_truth, l);
#ifdef debug
        puts << "Write buffers" << br;
#endif
        queue.enqueueWriteBuffer(buf_in, CL_TRUE, 0, dsize, arr_src);
        queue.enqueueWriteBuffer(buf_hist, CL_TRUE, 0, bufsize, hist_cl);
#ifdef debug
        puts << "Set kernel args" << br;
#endif
        cl_int succ = kern.setArg(0, buf_in);
        kern.setArg(1, buf_hist);
        kern.setArg(2, (cl_int) l);

#ifdef debug
        puts << succ << br;
#endif

        cl::Event evt;
        #ifdef debug
        puts << "Run kernel" << br;
#endif
        cl_status = queue.enqueueNDRangeKernel(kern, cl::NullRange, glob, loc, NULL, &evt);
        evt.wait();
#ifdef debug
        puts << "Get time" << br;
#endif
        double elapsed = evt.getProfilingInfo<CL_PROFILING_COMMAND_END>() -
                         evt.getProfilingInfo<CL_PROFILING_COMMAND_START>();
        //std::cout << "Elapsed " << elapsed << std::endl;
        queue.enqueueReadBuffer(buf_hist, CL_TRUE, 0, bufsize, hist_cl);
        bool correct = std::equal(hist_truth, hist_truth + 128, hist_cl);
        if(!correct){
            for(int k = 0; k < 128; k++){
 //               printf("Truth: %#010x CLL %#010x Difference %d\n", hist_truth[k], hist_cl[k], hist_cl[k] - hist_truth[k]);
            }
        }
        puts << kernel_path << "\t" << l << "\t" << elapsed << "\t" << correct << br;
        if (cl_status != CL_SUCCESS) {
            std::cerr << "Run fail: " << cl_status << std::endl;
            exit(-1);
        }
        #ifdef debug
        puts << "Go around" << br;
#endif

    }
}
