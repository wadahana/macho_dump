//
//  macho_utils.cpp
//  AnyDemo
//
//  Created by 吴昕 on 21/05/2017.
//  Copyright © 2017 chinanetcenter. All rights reserved.
//

#include "macho_utils.hpp"

const struct mach_header* lookup_dylib(const char * dylib_path) {
    struct task_dyld_info dyld_info;
    mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
    
    if (task_info(mach_task_self(), TASK_DYLD_INFO, (task_info_t) &dyld_info, &count) == KERN_SUCCESS) {
        struct dyld_all_image_infos* infos = (struct dyld_all_image_infos *) dyld_info.all_image_info_addr;
        struct dyld_image_info* info = (struct dyld_image_info*) infos->infoArray;
        
        for (int i=0; i < infos->infoArrayCount; i++) {
            if(strcmp(info[i].imageFilePath, dylib_path) == 0) {
                printf("path: %p %s\n", info[i].imageLoadAddress, info[i].imageFilePath);
                
                return info[i].imageLoadAddress;
            }
        }
    } else {
        printf("Not success!\n");
    }
    
    return NULL;
}

