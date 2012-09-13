/*
 * bitops.h -- Bit manipulation operations.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#ifndef __bitops_h
#define __bitops_h

#include <cat/cattypes.h>
#include <cat/archops.h>

uint32_t rup2_32(uint32_t x, uint lg2p);
uint32_t rdp2_32(uint32_t x, uint lg2p);
uint32_t compress_l32(uint32_t x, uint32_t mask);
uint32_t compress_r32(uint32_t x, uint32_t mask);
uint32_t SAG32(uint32_t x, uint32_t maskleft);
int arr_to_SAG_permvec32(uint8_t arr[32], uint32_t permvec[5]);
uint32_t permute32_SAG(uint32_t bits, uint32_t permvec[5]);
uint32_t bitgather32(uint32_t bits, uint8_t pos[32]);

#if CAT_64BIT
uint64_t rup2_64(uint64_t x, uint lg2p);
uint64_t rdp2_64(uint64_t x, uint lg2p);
uint64_t compress_l64(uint64_t x, uint64_t mask);
uint64_t compress_r64(uint64_t x, uint64_t mask);
uint64_t SAG64(uint64_t x, uint64_t maskleft);
int arr_to_SAG_permvec64(uint8_t arr[64], uint64_t permvec[6]);
uint64_t permute64_SAG(uint64_t bits, uint64_t permvec[6]);
uint64_t bitgather64(uint64_t bits, uint8_t pos[64]);
#endif

#endif /* __bitops_h */
