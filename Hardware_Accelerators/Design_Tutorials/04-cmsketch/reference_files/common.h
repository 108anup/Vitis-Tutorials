#pragma once

#include "sizes.h"

unsigned int MurmurHash2(const void* key, int len, unsigned int seed);

void runOnCPU (unsigned int*  doc_sizes,
               unsigned int*  input_doc_words,
               unsigned int   cm_sketch[cm_rows][cm_col_count],
               unsigned int   total_num_docs,
               unsigned int   total_size);

void runOnFPGA(
               unsigned int*  doc_sizes,
               unsigned int*  input_doc_words,
               unsigned int   cm_sketch[cm_rows][cm_col_count],
               unsigned int   total_num_docs,
               unsigned int   total_doc_size,
               int            num_iter);
