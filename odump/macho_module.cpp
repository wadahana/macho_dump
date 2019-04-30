//
//  macho_module.cpp
//  AnyDemo
//
//  Created by 吴昕 on 21/05/2017.
//  Copyright © 2017 chinanetcenter. All rights reserved.
//
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dlfcn.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <uuid/uuid.h>

#include <objc/objc.h>
#include <objc/runtime.h>
#include <mach/mach.h>
#include <mach-o/dyld_images.h>
#include <mach/vm_map.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/fat.h>
#include <mach-o/loader.h>
#include <mach-o/dyld.h>
#include <mach-o/reloc.h>

#include "macho_module.h"
#include "objc_utils.h"

macho_module::macho_module() {
    
    this->m_base_addr = NULL;
    this->m_module_slide = NULL;
    this->m_module_name = "";
    
    this->m_mach_header = NULL;
    this->m_segment_base = NULL;
    
    this->m_linkedit_segment = NULL;
    this->m_symtab_segment = NULL;
    this->m_dysymtab_segment = NULL;
    
    this->m_objc_classlist_section = NULL;
    this->m_objc_classname_section = NULL;
    this->m_objc_methodname_section = NULL;
    this->m_objc_protolist_section = NULL;
    this->m_objc_catlist_section = NULL;
}

//macho_module::macho_module(intptr_t base_addr, intptr_t slide, const char* module_name) {
//    this->m_base_addr = base_addr;
//    this->m_module_slide = slide;
//    this->m_module_name = module_name;
//    this->m_mach_header = (uint64_t)base_addr;
//}

macho_module::~macho_module() {

}

bool macho_module::load(intptr_t base_addr) {
    this->m_module_name = "memory module";
    this->m_base_addr = base_addr;
    this->m_module_slide = 0;
    this->m_load_from_file = true;
    
    return this->do_load();
}

bool macho_module::load(const char * dylib_name) {
    
    uint32_t c = _dyld_image_count();
    for (uint32_t i = 0; i < c; i++) {
        const char * name = _dyld_get_image_name(i);
        fprintf(stderr, "module_name(%s), image_header(%p), vmaddr_slide(%p)\n",
                name, (intptr_t)_dyld_get_image_header(i), _dyld_get_image_vmaddr_slide(i));
        if (name && strcmp(name, dylib_name) == 0) {
            this->m_module_name = dylib_name;
            this->m_base_addr = (intptr_t)_dyld_get_image_header(i);
            this->m_module_slide = _dyld_get_image_vmaddr_slide(i);
            this->m_load_from_file = false;
            return this->do_load();
        }
    }
    return false;
}

uint32_t macho_module::get_macho_file_type(){
    struct mach_header * hdr = (struct mach_header*)this->m_base_addr;
    return hdr->filetype;
}

bool macho_module::do_load() {
    
    struct mach_header * mach_hdr = (struct mach_header*)this->m_base_addr;
    
    if (mach_hdr->cputype == CPU_TYPE_ARM64) {
        this->m_64bit_abi = false;
        return this->do_load_32();
    } else if (mach_hdr->cputype == CPU_TYPE_ARM64) {
        this->m_64bit_abi = true;
        return this->do_load_64();
    } else {
        return false;
    }
    
}

bool macho_module::do_load_32() {
    
    intptr_t current_addr = (intptr_t)this->m_segment_base;
    struct mach_header * mach_hdr = (struct mach_header*)this->m_base_addr;
    struct segment_command * current_cmd = (struct segment_command *)current_addr;
    
    for (uint i = 0; i < mach_hdr->ncmds; i++) {
        current_cmd = (struct segment_command *)current_addr;
        if (current_cmd->cmd == LC_SEGMENT) {
            if (strcmp(current_cmd->segname, SEG_LINKEDIT) == 0) {
                this->m_linkedit_segment = reinterpret_cast<uint64_t>(current_cmd);
            }
            load_lc_segment(reinterpret_cast<uint64_t>(current_cmd));
        } else if (current_cmd->cmd == LC_SYMTAB) {
            this->m_symtab_segment = reinterpret_cast<uint64_t>(current_cmd);
        } else if (current_cmd->cmd == LC_DYSYMTAB) {
            this->m_dysymtab_segment = reinterpret_cast<uint64_t>(current_cmd);
        }
        current_addr += current_cmd->cmdsize;
    }
    
    if (!this->m_symtab_segment ||
        !this->m_dysymtab_segment ||
        !this->m_linkedit_segment) {  //this->m_dysymtab_cmd_segment->nindirectsyms == 0
        return false;
    }

    return true;
}

bool macho_module::do_load_64() {
    
    intptr_t current_addr = (intptr_t)this->m_segment_base;
    struct mach_header_64 * mach_hdr = (struct mach_header_64*)this->m_base_addr;
    
    struct segment_command_64 * current_cmd = (struct segment_command_64 *)current_addr;
    for (uint i = 0; i < mach_hdr->ncmds; i++) {
        current_cmd = (struct segment_command_64 *)current_addr;
        if (current_cmd->cmd == LC_SEGMENT_64) {
            if (strcmp(current_cmd->segname, SEG_LINKEDIT) == 0) {
                this->m_linkedit_segment = reinterpret_cast<uint64_t>(current_cmd);
            }
            load_lc_segment(reinterpret_cast<uint64_t>(current_cmd));
        } else if (current_cmd->cmd == LC_SYMTAB) {
            this->m_symtab_segment = reinterpret_cast<uint64_t>(current_cmd);
        } else if (current_cmd->cmd == LC_DYSYMTAB) {
            this->m_dysymtab_segment = reinterpret_cast<uint64_t>(current_cmd);
        }
        current_addr += current_cmd->cmdsize;
    }
    
    if (!this->m_symtab_segment ||
        !this->m_dysymtab_segment ||
        !this->m_linkedit_segment) {  //this->m_dysymtab_cmd_segment->nindirectsyms == 0
        return false;
    }
    return true;
}

void macho_module::load_lc_segment(uint64_t lc_segment) {
    int segment_command_size = this->m_64bit_abi ? sizeof(struct segment_command_64) : sizeof(struct segment_command);
    int section_size = this->m_64bit_abi ? sizeof(struct section_64) : sizeof(struct section);
    int section_nums = 0;
    if (this->m_64bit_abi) {
        section_nums = reinterpret_cast<struct segment_command_64 *>(lc_segment)->nsects;
    } else {
        section_nums = reinterpret_cast<struct segment_command *>(lc_segment)->nsects;
    }
    uint64_t section_offset = (uint64_t)(lc_segment + segment_command_size);
    for (int i = 0; i < section_nums; i++) {
        uint64_t current_section = static_cast<uint64_t>(section_offset + i * section_size);
        char * section_name = reinterpret_cast<char *>(current_section);
        if (strncmp(section_name, "__objc_methname", 16) == 0) {
            this->m_objc_methodname_section = reinterpret_cast<uint64_t>(current_section);
        } else if (strncmp(section_name, "__objc_classname", 16) == 0) {
            this->m_objc_classname_section = reinterpret_cast<uint64_t>(current_section);
        } else if (strncmp(section_name, "__objc_classlist", 16) == 0) {
            this->m_objc_classlist_section = reinterpret_cast<uint64_t>(current_section);
        } else if (strncmp(section_name, "__objc_catlist", 14) == 0) {
            this->m_objc_catlist_section = reinterpret_cast<uint64_t>(current_section);
        } else if (strncmp(section_name, "__objc_protolist", 16) == 0) {
            this->m_objc_protolist_section = reinterpret_cast<uint64_t>(current_section);
        }
        this->m_section_array.push_back(current_section);
    }
}


uint64_t macho_module::lookup_section_by_address(uint64_t addr) {
    for(std::list<uint64_t>::iterator itor = this->m_section_array.begin();
        itor != this->m_section_array.end();
        itor ++ ) {
        if (this->m_64bit_abi) {
            struct section_64 * section = reinterpret_cast<struct section_64 *>(*itor);
            if (section->addr <= addr && addr < (section->addr + section->size)) {
                return *itor;
            }
        } else {
            struct section * section = reinterpret_cast<struct section *>(*itor);
            if (section->addr <= addr && addr < (section->addr + section->size)) {
                return *itor;
            }
        }
    }
    return NULL;
}

uint64_t macho_module::lookup_section_by_name(const char * segname, const char * sectname) {
    for(std::list<uint64_t>::iterator itor = this->m_section_array.begin();
        itor != this->m_section_array.end();
        itor ++ ) {
        if (this->m_64bit_abi) {
            struct section_64 * section = reinterpret_cast<struct section_64 *>(*itor);
            if (strncmp(section->sectname, sectname, strlen(section->sectname)) == 0 &&
                strncmp(section->segname, segname, strlen(section->segname)) == 0) {
                return *itor;
            }
        } else {
            struct section * section = reinterpret_cast<struct section *>(*itor);
            if (strncmp(section->sectname, sectname, strlen(section->sectname)) == 0 &&
                strncmp(section->segname, segname, strlen(section->segname)) == 0) {
                return *itor;
            }
        }
    }
    return NULL;
}

struct nlist * macho_module::lookup_symbol_32(const char *name) {

    struct symtab_command * symtab_segment = reinterpret_cast<struct symtab_command *>(this->m_symtab_segment);
    
    struct nlist * symbol_table = NULL;
    const char * string_table = NULL;
    int symbol_nums = symtab_segment->nsyms;
    
    if (!this->m_load_from_file) {
        struct segment_command * linkedit_segment = reinterpret_cast<struct segment_command *>(this->m_linkedit_segment);
        uint64_t linkedit_base = this->m_module_slide + linkedit_segment->vmaddr - linkedit_segment->fileoff;
        symbol_table = reinterpret_cast<struct nlist *>(linkedit_base + symtab_segment->symoff);
        string_table = reinterpret_cast<const char *>(linkedit_base + symtab_segment->stroff);
    } else {
//          FIX ME
//        symbol_table = reinterpret_cast<struct nlist *>(this->m_base_addr + symtab_segment->symoff);
//        string_table = reinterpret_cast<const char *>(this->m_base_addr + symtab_segment->stroff);
    }
    
    
    for (int i = 0; i < symbol_nums; i++) {
        struct nlist * symbol = &symbol_table[i];
        const char * symbol_name = string_table + symbol->n_un.n_strx;
        if (strcmp(symbol_name, name) == 0) {
            return symbol;
        }
    }
    return NULL;
}

struct nlist_64 * macho_module::lookup_symbol_64(const char *name) {
    struct symtab_command * symtab_segment = reinterpret_cast<struct symtab_command *>(this->m_symtab_segment);
    
    struct nlist_64 * symbol_table = NULL;
    const char * string_table = NULL;
    int symbol_nums = symtab_segment->nsyms;
    
    if (!this->m_load_from_file) {
        struct segment_command_64 * linkedit_segment = (struct segment_command_64 *)this->m_linkedit_segment;
        uint64_t linkedit_base = this->m_module_slide + linkedit_segment->vmaddr - linkedit_segment->fileoff;
        symbol_table = reinterpret_cast<nlist_64 *>(linkedit_base + symtab_segment->symoff);
        string_table = reinterpret_cast<const char *>(linkedit_base + symtab_segment->stroff);
    } else {
//          FIX ME
//        symbol_table = reinterpret_cast<nlist_64 *>(this->m_base_addr + symtab_segment->symoff);
//        string_table = reinterpret_cast<const char *>(this->m_base_addr + symtab_segment->stroff);
    }
    
    for (int i = 0; i < symbol_nums; i++) {
        struct nlist_64 * symbol = &symbol_table[i];
        const char * symbol_name = string_table + symbol->n_un.n_strx;
//        fprintf(stderr, "index(%d), n_strx(%d), type(%02X), value(%p),symbol_name(%s)\n",
//                i, symbol->n_un.n_strx, symbol->n_type, symbol->n_value, symbol_name);
        if (strcmp(symbol_name, name) == 0) {
            return symbol;
        }
    }
    return NULL;
}

//bool macho_module::load_obj_catagory() {
//
//}

bool macho_module::load_objc_class(uint64_t) {
    
    struct _objc2_class objc_class;
    struct _objc2_class_ro_t objc_class_data;
    
    uint64_t classlist_addr = NULL;
    if (m_64bit_abi) {
        struct section_64 * section = reinterpret_cast<struct section_64 *>(this->m_objc_classlist_section);
        if (section->reloff && section->nreloc > 0) { // using Relocations __DATA,__objc_classlist
            struct relocation_info * reloc = reinterpret_cast<struct relocation_info *>(this->m_base_addr + section->reloff);
            uint64_t addr = reinterpret_cast<uint64_t>(reloc->r_symbolnum);
            struct section_64 * data_section = this->lookup_section_by_address(addr);
            uint64_t * ptr = reinterpret_cast<uint64_t *>(addr - data_section->addr + data_section->offset);
            objc_class.isa        = ptr[0];
            objc_class.superclass = ptr[1];
            objc_class.cache      = ptr[2];
            objc_class.vtable     = ptr[3];
            objc_class.data       = ptr[4];
            objc_class.reserved1  = ptr[5];
            objc_class.reserved2  = ptr[6];
            objc_class.reserved3  = ptr[7];
            
        } else {
        
        }
  
        ptr = ptr + 
        objc2ClassData.flags         = [cursor readInt32];
        objc2ClassData.instanceStart = [cursor readInt32];
        objc2ClassData.instanceSize  = [cursor readInt32];
        if ([self.machOFile uses64BitABI])
            objc2ClassData.reserved  = [cursor readInt32];
        else
            objc2ClassData.reserved = 0;
        
        objc2ClassData.ivarLayout     = [cursor readPtr];
        objc2ClassData.name           = [cursor readPtr];
        objc2ClassData.baseMethods    = [cursor readPtr];
        objc2ClassData.baseProtocols  = [cursor readPtr];
        objc2ClassData.ivars          = [cursor readPtr];
        objc2ClassData.weakIvarLayout = [cursor readPtr];
        objc2ClassData.baseProperties = [cursor readPtr];
    } else {
        struct section * section = reinterpret_cast<struct section *>(this->m_objc_classlist_section);
        uint32_t * ptr = reinterpret_cast<uint32_t *>(this->m_base_addr + section->offset);
        objc_class.isa        = ptr[0];
        objc_class.superclass = ptr[1];
        objc_class.cache      = ptr[2];
        objc_class.vtable     = ptr[3];
        objc_class.data       = ptr[4];
        objc_class.reserved1  = ptr[5];
        objc_class.reserved2  = ptr[6];
        objc_class.reserved3  = ptr[7];
    }

    
    
    
}

void macho_module::dump_objc_symbols_64() {
    
}

void macho_module::dump_symbol_32() {
    
    struct symtab_command * symtab_segment = reinterpret_cast<struct symtab_command *>(this->m_symtab_segment);
    
    struct nlist * symbol_table = NULL;
    const char * string_table = NULL;
    int symbol_nums = symtab_segment->nsyms;
    
    if (!this->m_load_from_file) {
        struct segment_command * linkedit_segment = reinterpret_cast<struct segment_command *>(this->m_linkedit_segment);
        uint64_t linkedit_base = this->m_module_slide + linkedit_segment->vmaddr - linkedit_segment->fileoff;
        symbol_table = reinterpret_cast<struct nlist *>(linkedit_base + symtab_segment->symoff);
        string_table = reinterpret_cast<const char *>(linkedit_base + symtab_segment->stroff);
    } else {
        symbol_table = reinterpret_cast<struct nlist *>(this->m_base_addr + symtab_segment->symoff);
        string_table = reinterpret_cast<const char *>(this->m_base_addr + symtab_segment->stroff);
    }
    
    for (int i = 0; i < symbol_nums; i++) {
        struct nlist * symbol = &symbol_table[i];
        const char * symbol_name = string_table + symbol->n_un.n_strx;
        fprintf(stdout, "%s\n", symbol_name);
    }
}

void macho_module::dump_symbol_64() {
    
    struct symtab_command * symtab_segment = reinterpret_cast<struct symtab_command *>(this->m_symtab_segment);
    
    struct nlist_64 * symbol_table = NULL;
    const char * string_table = NULL;
    int symbol_nums = symtab_segment->nsyms;
    
    if (!this->m_load_from_file) {
        struct segment_command_64 * linkedit_segment = (struct segment_command_64 *)this->m_linkedit_segment;
        uint64_t linkedit_base = this->m_module_slide + linkedit_segment->vmaddr - linkedit_segment->fileoff;
        symbol_table = reinterpret_cast<nlist_64 *>(linkedit_base + symtab_segment->symoff);
        string_table = reinterpret_cast<const char *>(linkedit_base + symtab_segment->stroff);
    } else {
        symbol_table = reinterpret_cast<nlist_64 *>(this->m_base_addr + symtab_segment->symoff);
        string_table = reinterpret_cast<const char *>(this->m_base_addr + symtab_segment->stroff);
    }
    
    for (int i = 0; i < symbol_nums; i++) {
        struct nlist_64 * symbol = &symbol_table[i];
        const char * symbol_name = string_table + symbol->n_un.n_strx;
        fprintf(stdout, "%s\n", symbol_name);
    }
}


bool macho_module::is_macho_module(intptr_t base_addr) {
    struct mach_header * hdr = (struct mach_header *)base_addr;
    
    if (hdr->cputype == CPU_TYPE_ARM || hdr->cputype == CPU_TYPE_ARM64) {
        return true;
    }
    return false;
}
