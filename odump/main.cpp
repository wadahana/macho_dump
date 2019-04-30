//
//  main.cpp
//  odump
//
//  Created by 吴昕 on 20/06/2017.
//  Copyright © 2017 ChinaNetCenter. All rights reserved.
//

#include <iostream>
#include <assert.h>
#include "macho_module.h"

int main(int argc, const char * argv[]) {

    FILE * din = fopen("/Users/wuxin/workspace/svn/ios_tools/sdk_3.5.3/BUILD/SDK/WSPX/build/WSPX.build/Release-iphoneos/WSPX.build/Objects-normal/arm64/WSPXManager.o","rb");
    assert(din != NULL);
    
    if (fseek(din, 0, SEEK_END) < 0) {
        fclose(din);
        return -1;
    }
    size_t length = ftell(din);
    
    if (fseek(din, 0, SEEK_SET) < 0) {
        fclose(din);
        return -1;
    }
    
    uint8_t * buffer = (uint8_t *)malloc(sizeof(uint8_t) * (length + 10));
    if (!buffer) {
        fclose(din);
        return -1;
    }
    size_t szread = fread(buffer, 1, length, din);
    if (szread < length) {
        free(buffer);
        fclose(din);
        return -1;
    }
    
    macho_module module;
    module.load((intptr_t)buffer);
    module.load_symbol_table();

    free(buffer);
    fclose(din);
    
    return 0;
}
