#include<iostream>
#include<ctime>
#include<chrono>
#include<vector>
#include<utility>
#include<cstdio>
#include<cstdlib>

// sizes.h automatically included by common.h
#include "common.h"

using namespace std;
using namespace std::chrono;

void runOnCPU (
    unsigned int*  doc_sizes,
    unsigned int*  input_doc_words,
    unsigned int   cm_sketch[cm_rows][cm_col_count],
    unsigned int   total_num_docs,
    unsigned int   total_size)
{
    unsigned int size_offset=0;
    chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();

    for(unsigned int doc=0;doc<total_num_docs;doc++) 
    {
        unsigned int size = doc_sizes[doc];
        for (unsigned i = 0; i < size ; i++)
        { 
            unsigned curr_entry = input_doc_words[size_offset+i];
            unsigned word_id = curr_entry >> 8;
            for(int row = 0; row < cm_rows; row++) {
              unsigned hash = MurmurHash2(&word_id, 3, cm_seeds[row]);
              unsigned index = hash % cm_col_count;
              cm_sketch[row][index]++;
            }
        }
        size_offset+=size;
    }
   
    chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
    chrono::duration<double> time_span_cpu   = (t2-t1);
    printf(" Executed Software-Only version     |  %10.4f ms\n", 1000*time_span_cpu.count());
}
