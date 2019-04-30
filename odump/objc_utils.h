//
//  objc_utils.h
//  odump
//
//  Created by 吴昕 on 21/06/2017.
//  Copyright © 2017 ChinaNetCenter. All rights reserved.
//

#ifndef objc_utils_h
#define objc_utils_h

struct _objc2_class {
    uint64_t isa;
    uint64_t superclass;
    uint64_t cache;
    uint64_t vtable;
    uint64_t data; // points to class_ro_t
    uint64_t reserved1;
    uint64_t reserved2;
    uint64_t reserved3;
};

struct _objc2_class_ro_t {
    uint32_t flags;
    uint32_t instanceStart;
    uint32_t instanceSize;
    uint32_t reserved; // *** this field does not exist in the 32-bit version ***
    uint64_t ivarLayout;
    uint64_t name;
    uint64_t baseMethods;
    uint64_t baseProtocols;
    uint64_t ivars;
    uint64_t weakIvarLayout;
    uint64_t baseProperties;
};

struct _objc2_method {
    uint64_t name;
    uint64_t types;
    uint64_t imp;
};


#endif /* objc_utils_h */
