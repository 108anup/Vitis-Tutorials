#include <vector>
#include <cstdio>
#include <ctime>

#include "xcl2.hpp"
#include "sizes.h"
#include "common.h"

using namespace std;
using namespace std::chrono;

string kernel_name = "runOnfpga";
const char* kernel_name_charptr = kernel_name.c_str();
unsigned size_per_iter_const=512*1024;
unsigned size_per_iter;


void runOnFPGA(
               unsigned int*  doc_sizes,
               unsigned int*  input_doc_words,
               unsigned int*  cm_sketch,
               unsigned int   total_num_docs,
               unsigned int   total_doc_size,
               int            num_iter)
{
  if ((total_doc_size)%64!=0) {
    printf("--------------------------------------------------------------------\n");
    printf("ERROR: The number of word per iterations must be a multiple of 64\n");
    printf("       Total words = %d, Number of iterations = 1, Word per iterations = %d\n", total_doc_size, total_doc_size);
    printf("       Skipping FPGA kernel execution\n");
    exit(-1);
  }
  // Boilerplate code to load the FPGA binary, create the kernel and command queue
  vector<cl::Device> devices = xcl::get_xil_devices();
  cl::Device device = devices[0];
  cl::Context context(device);
  cl::CommandQueue q(context,device, CL_QUEUE_PROFILING_ENABLE|CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);

  string run_type = xcl::is_emulation()?(xcl::is_hw_emulation()?"hw_emu":"sw_emu"):"hw";
  string binary_file = kernel_name + "_" + run_type + ".xclbin";
  cl::Program::Binaries bins = xcl::import_binary_file(binary_file);
  cl::Program program(context, devices, bins);
  cl::Kernel kernel(program,kernel_name_charptr,NULL);

  unsigned int total_size = total_doc_size;
  bool download_sketch = false;

  // Create buffers
  cl::Buffer buffer_input_doc_words(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                    total_size*sizeof(uint),input_doc_words);
  cl::Buffer buffer_sketch(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                           (cm_rows)*(cm_col_count)*sizeof(uint), cm_sketch);

  // Set buffer kernel arguments (needed to migrate the buffers in the correct memory)
  kernel.setArg(0, buffer_input_doc_words);
  kernel.setArg(1, buffer_sketch);

  double mbytes_total  = (double)(total_doc_size * sizeof(int)) / (double)(1000*1000);
  printf(" Processing %.3f MBytes of data\n", mbytes_total);
  printf(" Single_Buffer: Running with a single buffer of %.3f MBytes for FPGA processing\n",mbytes_total);

  // Create events for read,compute and write

  vector<cl::Event> wordWait;
  vector<cl::Event> krnlWait;
  vector<cl::Event> flagWait;
  cl::Event buffDone, krnlDone, flagDone;

  printf("--------------------------------------------------------------------\n");


  chrono::high_resolution_clock::time_point t1, t2;
  t1 = chrono::high_resolution_clock::now();


  // Load the bloom filter and input document words buffers
  q.enqueueMigrateMemObjects({buffer_input_doc_words}, 0,NULL,&buffDone);
  wordWait.push_back(buffDone);

  // Start the FPGA compute
  download_sketch = true;
  kernel.setArg(2, total_size);
  kernel.setArg(3, download_sketch);
  q.enqueueTask(kernel,&wordWait,&krnlDone);
  krnlWait.push_back(krnlDone);

  // Read back the results from FPGA to host
  q.enqueueMigrateMemObjects({buffer_sketch}, CL_MIGRATE_MEM_OBJECT_HOST,&krnlWait,&flagDone);
  flagWait.push_back(flagDone);
  flagWait[0].wait();

  t2 = chrono::high_resolution_clock::now();
  chrono::duration<double> perf_all_sec  = chrono::duration_cast<duration<double>>(t2-t1);

  cl_ulong f1 = 0;
  cl_ulong f2 = 0;
  wordWait.front().getProfilingInfo(CL_PROFILING_COMMAND_QUEUED, &f1);
  flagWait.back().getProfilingInfo(CL_PROFILING_COMMAND_END, &f2);
  double perf_hw_ms = (f2 - f1)/1000000.0;

  if (xcl::is_emulation()) {
    if (xcl::is_hw_emulation()) {
      printf(" Emulated FPGA accelerated version  | run 'vitis_analyzer xclbin.run_summary' for performance estimates");
    } else {
      printf(" Emulated FPGA accelerated version  | (performance not relevant in SW emulation)");
    }
  } else {
    printf(" Executed FPGA accelerated version  | %10.4f ms   ( FPGA %.3f ms )", 1000*perf_all_sec.count(), perf_hw_ms);
  }
  printf("\n");
}
