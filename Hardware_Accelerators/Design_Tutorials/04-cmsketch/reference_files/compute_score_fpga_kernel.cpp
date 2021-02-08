#include <iostream>
#include <ctime>
#include <utility>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "hls_stream_utils.h"
#include "sizes.h"

#define TOTAL_SIZE 3500000

#ifndef PARALLELISATION
#define PARALLELISATION 8
#endif

#ifndef HASH_UNITS
#define HASH_UNITS (PARALLELISATION * cm_rows)
#endif

// To be able to use macros within pragmas
// From https://www.xilinx.com/support/answers/46111.html
#define PRAGMA_SUB(x) _Pragma (#x)
#define DO_PRAGMA(x) PRAGMA_SUB(x)

//TRIPCOUNT identifiers
const unsigned int t_size = TOTAL_SIZE;
const unsigned int pf = PARALLELISATION;

typedef ap_uint<sizeof(int)*8*PARALLELISATION> parallel_words_t;

unsigned int MurmurHash2(unsigned int key, int len, unsigned int seed)
{
  const unsigned char* data = (const unsigned char *)&key;
  const unsigned int m = 0x5bd1e995;
  unsigned int h = seed ^ len;
  switch(len) {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0];
            h *= m;
  };
  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;
  return h;
}

void update_sketch (hls::stream<parallel_words_t>& word_stream,
                    unsigned int                   cm_sketch_local[cm_rows][cm_col_count],
                    unsigned int                   total_size)
{
  // std::cout<<"FPGA total_size: "<< total_size<<"\n";
  // std::cout<<"FPGA words start \n";

  DO_PRAGMA(HLS ALLOCATION function instances=MurmurHash2 limit=HASH_UNITS)
  update_sketch_loop: for(int i=0; i<total_size/PARALLELISATION; i++)
  {
    #pragma HLS LOOP_TRIPCOUNT min=1 max=t_size/pf
    parallel_words_t parallel_entries = word_stream.read();

    update_rows: for (unsigned int j=0; j<PARALLELISATION; j++)
    {
#pragma HLS UNROLL

      unsigned int curr_entry = parallel_entries(31+j*32, j*32);
      // unsigned int frequency = curr_entry & 0x00ff; // not used
      unsigned int word_id = curr_entry >> 8; // that is why max length 3
      // std::cout<<word_id<<"\n";

      update_rows_loop: for(int row = 0; row < cm_rows; row++) {
#pragma HLS UNROLL
        unsigned hash = MurmurHash2(word_id, 3, cm_seeds[row]);
        unsigned index = hash % cm_col_count;
        // Possible collisions here (see how does vitis handle these)
        cm_sketch_local[row][index]++;
      }
    }
  }
  // std::cout<<"FPGA words end\n";

}

void update_sketch_dataflow(
        ap_uint<512>*   input_words,
        unsigned int    cm_sketch[cm_rows][cm_col_count],
        unsigned int    total_size)
{
#pragma HLS DATAFLOW

    hls::stream<ap_uint<512> >    data_from_gmem;
    hls::stream<parallel_words_t> word_stream;

  // Burst read 512-bit values from global memory over AXI interface
  hls_stream::buffer(data_from_gmem, input_words, total_size/(512/32));

  // Form a stream of parallel words from stream of 512-bit values
  // Going from Wi=512 to Wo= 256
  hls_stream::resize(word_stream, data_from_gmem, total_size/(512/32));

  // Process stream of parallel word : word_stream is of 2k (32*64)
  update_sketch(word_stream, cm_sketch, total_size);
}

extern "C"
{

  // TODO(108anup) Check if one can pass arrays like this
  // Why did the tutorial not pass the array like this then
  // (Maybe for 1d array not much difference?)
  void runOnfpga (
          ap_uint<512>*  input_words,
          unsigned int   cm_sketch[cm_rows][cm_col_count],
          unsigned int   total_size,
          bool           download_sketch)
  {
  #pragma HLS INTERFACE ap_ctrl_chain port=return            bundle=control
  #pragma HLS INTERFACE m_axi         port=input_words       bundle=maxiport0   offset=slave
  #pragma HLS INTERFACE m_axi         port=cm_sketch         bundle=maxiport1   offset=slave

    // Static arrays are by default 0 initialized in CPP,
    // don't know if same is done with v++ compiler
    // https://stackoverflow.com/questions/201101/how-to-initialize-all-members-of-an-array-to-the-same-value
    static unsigned int cm_sketch_local[cm_rows][cm_col_count];
    #pragma HLS ARRAY_PARTITION variable=cm_sketch_local complete dim=1
    printf("From runOnfpga : Total_size = %d\n", total_size);

    update_sketch_dataflow(input_words,
                           cm_sketch_local,
                           total_size);

    if(download_sketch==true)
    {
    write_cm_sketch: for(int row = 0; row<cm_rows; row++) {
#pragma HLS PIPELINE II=1
        for(int index = 0; index<cm_col_count; index++) {
          cm_sketch[row][index] = cm_sketch_local[row][index];
        }
      }
    }
  }
}
