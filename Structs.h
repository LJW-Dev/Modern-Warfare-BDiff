#pragma once

#include <stdio.h>
#include "Instructions.h"

struct VcdState
{
	unsigned int next_slot;
	size_t anear[4];
	size_t asame[768];
	unsigned char* pAddr;
};

struct diffInfo
{
	VcdState vcd;
	bool headerRead;

	unsigned char* patchBuffer;
	size_t patchBufferSize;
	size_t patchBufferOffset;

	unsigned char* sourceBuffer;
	size_t sourceBufferSize;
	size_t sourceBufferOffset;

	unsigned char* destBuffer;
	size_t destBufferSize;
	size_t destBufferOffset;
};

DoubleInstruction* instructions = (DoubleInstruction*)instructions_arr;