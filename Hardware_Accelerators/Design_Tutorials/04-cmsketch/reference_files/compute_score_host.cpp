#include<iostream>
#include<ctime>
#include<chrono>
#include<vector>
#include<utility>
#include<cstdio>
#include<cstdlib>

// sizes.h automatically included by common.h
#include "common.h"
#include "sizes.h"

using namespace std;
using namespace std::chrono;

void runOnCPU (
    unsigned int*  doc_sizes,
    unsigned int*  input_doc_words,
    unsigned int*  cm_sketch,
    unsigned int   total_num_docs,
    unsigned int   total_size)
{
  // unsigned total_word_count = 0;
    unsigned int size_offset=0;
    chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();

    // std::cout<<"CPU words start"<<endl;

    for(unsigned int doc=0;doc<total_num_docs;doc++) 
    {
        unsigned int size = doc_sizes[doc];
        for (unsigned i = 0; i < size ; i++)
        { 
            unsigned curr_entry = input_doc_words[size_offset+i];
            unsigned word_id = curr_entry >> 8;
            // total_word_count++;
            for(int row = 0; row < cm_rows; row++) {
              unsigned hash = MurmurHash2(&word_id, 3, cm_seeds[row]);
              unsigned index = hash % cm_col_count;
              cm_sketch[row * cm_col_count + index]++;
            }
        }
        size_offset+=size;
    }
    // std::cout<<"CPU words count"<<total_word_count<<endl;

    // std::cout<<"CPU words end"<<endl;
    // printf("CPU bad_word count: %u", bad_count);
   
    chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
    chrono::duration<double> time_span_cpu   = (t2-t1);
    printf(" Executed Software-Only version     |  %10.4f ms\n", 1000*time_span_cpu.count());
}
