#!/bin/bash

HASH_COUNTS=(1 2 3 4 6 8 14)

for h in "${HASH_COUNTS[@]}"; do
    make run STEP=single_buffer ITER=8 PF=8 TARGET=hw_emu HASH_UNIT_COUNT=$h
done
