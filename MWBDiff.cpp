#include <stdbool.h>
#include <string.h>
#include "Structs.h"

void initSourceData(diffInfo* s_diffInfo, unsigned char* sourceBuffer, size_t sourceBufferSize)
{
    s_diffInfo->sourceBuffer = sourceBuffer;
    s_diffInfo->sourceBufferSize = sourceBufferSize;
    s_diffInfo->sourceBufferOffset = 0;
}

void initPatchData(diffInfo* s_diffInfo, unsigned char* patchBuffer, size_t patchBufferSize)
{
    s_diffInfo->patchBuffer = patchBuffer;
    s_diffInfo->patchBufferSize = patchBufferSize;
    s_diffInfo->patchBufferOffset = 0;
}

unsigned char* loadSourceData(diffInfo* s_diffInfo, size_t offset, size_t size)
{
    size_t sourceOffset = offset;

    if (offset)
        s_diffInfo->sourceBufferOffset = offset;
    else
        sourceOffset = s_diffInfo->sourceBufferOffset;

    return &s_diffInfo->sourceBuffer[sourceOffset];
}

unsigned char* loadPatchData(diffInfo* s_diffInfo, size_t offset, size_t size, size_t* pOffset)
{
    size_t patchOffset = offset;

    if (offset)
        s_diffInfo->patchBufferOffset = offset;
    else
        patchOffset = s_diffInfo->patchBufferOffset;

    if (pOffset)
        *pOffset = patchOffset;

    return &s_diffInfo->patchBuffer[patchOffset];
}

unsigned char* setupDestData(diffInfo* s_diffInfo, size_t size)
{
    s_diffInfo->destBufferSize = size;
    s_diffInfo->destBuffer = new unsigned char[size];
    memset(s_diffInfo->destBuffer, 0, size);
    
    return s_diffInfo->destBuffer;
}

size_t readULEB128(unsigned __int8** start)
{
    size_t output = 0;
    char currChar;

    do
    {
        currChar = **start;
        (*start)++;
        output = (output << 7) | (currChar & 0x7F);
    } 
    while (currChar < 0);

    return output;
}

size_t addr_decode(VcdState* vcd, size_t here, int mode)
{
    size_t address;

    if (mode == 0)
    {
        address = readULEB128(&vcd->pAddr);
    }
    else if (mode == 1)
    {
        address = here - readULEB128(&vcd->pAddr);
    }
    else if (mode >= 6)
    {
        int index = (mode - 6) << 8;
        index += *vcd->pAddr;
        vcd->pAddr++;
        address = vcd->asame[index];
    }
    else
    {
        address = vcd->anear[mode - 2] + readULEB128(&vcd->pAddr);
    }

    vcd->anear[vcd->next_slot] = address;
    vcd->next_slot = (vcd->next_slot + 1) & 3;
    vcd->asame[address % 0x300] = address;

    return address;
}

bool bdiff_internal(diffInfo* s_diffInfo, 
    unsigned char* (__fastcall* loadSourceData)(diffInfo* s_diffInfo, size_t offset, size_t size),
    unsigned char* (__fastcall* loadPatchData)(diffInfo* s_diffInfo, size_t offset, size_t size, size_t* pOffset),
    unsigned char* (__fastcall* setupDestData)(diffInfo* s_diffInfo, size_t size))
{
    if (!s_diffInfo->headerRead)
    {
        unsigned char* header = loadPatchData(s_diffInfo, 0, 0x405, 0);
        if (!header)
            return false;

        if (header[0] != 0xD6 || header[1] != 0xC3 || header[2] != 0xC4)
            return false;

        if ((header[4] & 0xFFFFFFF3) != 0)
            return false;
        
        loadPatchData(s_diffInfo, 5, 0, 0);
        s_diffInfo->headerRead = true;
    }

    unsigned __int64 diffFileStart;
    unsigned char* v13 = loadPatchData(s_diffInfo, 0, 0x400, &diffFileStart);

    auto headerStart = v13;
    char type = *v13;
    char v16 = type & 3;
    if (v16 == 3)
        return false;

    v13++;
    unsigned __int8* v83 = NULL;
    size_t v22 = 0;
    __int64 v25 = 0;
    __int64 v28 = 0;
    if (v16)
    {
        v22 = readULEB128(&v13);
        v25 = readULEB128(&v13);

        if (v16 != 1)
            return false;

        if ((type & 4) != 0)
            v28 = readULEB128(&v13);
        else
            v28 = v25;

        __int64 v27 = v25 - v28;
        auto v32 = loadSourceData(s_diffInfo, v28, v22 + v27);
        v83 = &v32[v27];
    }
    __int64 v33 = readULEB128(&v13);
    auto data = loadPatchData(s_diffInfo, diffFileStart + v13 - headerStart, v33 + ((type >> 1) & 4), &diffFileStart);
    auto dataStart = data;

    __int64 v40 = readULEB128(&data);
    auto v44 = setupDestData(s_diffInfo, v40);
    auto destDataStart = v44;

    if (*data)
        return false;
    data++;

    memset(&s_diffInfo->vcd, 0, sizeof(VcdState));

    __int64 v42 = readULEB128(&data);
    __int64 v45 = readULEB128(&data);
    __int64 v48 = readULEB128(&data);

    unsigned char* v59 = &data[v42];
    unsigned char* v87 = &data[v42 + v45];
    unsigned char* v89 = &v87[v48];
    s_diffInfo->vcd.pAddr = v87;

    while (v59 < v87)
    {
        auto v53 = *v59++;

        DoubleInstruction* v62 = &instructions[v53];
        DoubleInstruction* oldIns = v62;
        for (int i = 0; i < 2; i++)
        {
            Instruction instruction = v62->instruction[i];
            if (!instruction.op)
            {
                v62 = oldIns;
                continue;
            }

            size_t size = instruction.size;
            if (!size)
            {
                size = readULEB128(&v59);
            }
            if (instruction.op == 1)
            {
                unsigned char* v68 = data;
                data += size;
                memcpy(v44, v68, size);
                v44 += size;
                v62 = oldIns;
                continue;
            }
            if (instruction.op != 2)
            {
                auto v70 = destDataStart;
                size_t v71 = v22;
                size_t v72 = addr_decode(&s_diffInfo->vcd, v22 + v44 - destDataStart, instruction.mode);

                unsigned __int8* v68 = 0;
                if (v72 < v71)
                {
                    if (!v83)
                        return false;

                    v68 = &v83[v72];
                }
                else
                {
                    v68 = (v72 - v71 + v70);
                }
                memcpy(v44, v68, size);
                v44 += size;
                v62 = oldIns;
                continue;
            }
            char v69 = *data++;
            memset(v44, v69, size);
            v44 += size;
            v62 = oldIns;
        }
    }

    if ((type & 8) != 0)
        v89 += 4;

    loadPatchData(s_diffInfo, diffFileStart + v89 - dataStart, 0, 0);

    return true;
}

extern "C" __declspec(dllexport) bool bdiff(
    unsigned char* sourceBuffer, size_t sourceBufferSize, 
    unsigned char* patchBuffer, size_t patchBufferSize, 
    unsigned char* outBuffer, size_t outBufferSize)
{
    diffInfo s_diffInfo;
	memset(&s_diffInfo, 0, sizeof(diffInfo));

    initSourceData(&s_diffInfo, sourceBuffer, sourceBufferSize);
    initPatchData(&s_diffInfo, patchBuffer, patchBufferSize);

    if (outBufferSize > 0)
    {
        do
        {
            if (!bdiff_internal(&s_diffInfo, loadSourceData, loadPatchData, setupDestData))
                return false;

            memcpy(outBuffer, s_diffInfo.destBuffer, s_diffInfo.destBufferSize);

            outBuffer += s_diffInfo.destBufferSize;
            outBufferSize -= s_diffInfo.destBufferSize;
        } 
        while (outBufferSize > 0);
    }

    return true;
}

void eee()
{
    bdiff(NULL, 0, NULL, 0, NULL, 0);
}
