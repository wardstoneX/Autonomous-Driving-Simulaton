#include <string.h>
#include "stdint.h"
#include "lib_debug/Debug.h"
#include "sc_helpers.h"


uint32_t formatForPython(uint32_t e, uint32_t nLen, uint8_t* n, uint32_t bufLen, uint8_t* buf){
    uint8_t* pointer = buf;

    if(bufLen < 4+4+nLen) {
        Debug_LOG_ERROR("The buffer is too small!");
    }

    memcpy(pointer, &e, 4);
    pointer += 4;
    memcpy(pointer, &nLen, 4);
    pointer += 4;
    memcpy(pointer, n, nLen);
    pointer += nLen;

    return pointer - buf;
}

