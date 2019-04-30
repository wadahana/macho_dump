//
//  macho_module.hpp
//  AnyDemo
//
//  Created by 吴昕 on 21/05/2017.
//  Copyright © 2017 chinanetcenter. All rights reserved.
//

#ifndef macho_module_hpp
#define macho_module_hpp

#include <unistd.h>
#include <stdio.h>
#include <string>
#include <list>

#include <mach-o/nlist.h>

class macho_module {

public:
    macho_module();

//    macho_module(intptr_t base_addr, intptr_t slide, const char* module_name);
    
    ~macho_module();
    
    bool load(intptr_t base_addr);
    bool load(const char * dylib_name);
    
    struct nlist * lookup_symbol_32(const char * name);
    struct nlist_64 * lookup_symbol_64(const char * name);
    
    static bool is_macho_module(intptr_t base_addr);
    
    bool load_objc_class();
    
protected:
    
    uint32_t get_macho_file_type();
    
    bool do_load();
    bool do_load_32();
    bool do_load_64();
    void load_lc_segment(uint64_t lc_segment);
    
    void dump_symbol_32();
    
    void dump_symbol_64();
    
    void dump_objc_symbols_32();
    
    void dump_objc_symbols_64();
    
    uint64_t lookup_section_by_address(uint64_t addr);
    
    uint64_t lookup_section_by_name(const char * segname, const char * sectname);

protected:
    
    intptr_t m_base_addr;
    
    intptr_t m_module_slide;
    
    std::string m_module_name;
    
    bool m_load_from_file;
    
    bool m_64bit_abi;
    
    std::list<uint64_t> m_section_array;
    
protected:
    
    uint64_t m_mach_header;
    
    uint64_t m_segment_base;
    
    uint64_t  m_linkedit_segment;
    
    uint64_t m_symtab_segment;
    
    uint64_t  m_dysymtab_segment;
    
//    uint64_t m_objc_data_section;
    
    uint64_t m_objc_methodname_section;
    
    uint64_t m_objc_classname_section;
    
    uint64_t m_objc_classlist_section;
    
    uint64_t m_objc_protolist_section;
    
    uint64_t m_objc_catlist_section;
    
#if 0
    
    const char * m_string_table;
    
    struct symtab_command * m_symtab_segment;
    
    struct dysymtab_command * m_dysymtab_segment;
    
    // for 32bits cpu
    struct mach_header * m_mach_header_32;
    
    struct segment_command * m_base_cmd_segment_32;
    
    struct segment_command * m_linkedit_cmd_segment_32;
    
    struct nlist * m_symbol_table_32;

    struct section * m_objc_methodname_32;
    
    struct section * m_objc_classname_32;
    
    struct section * m_objc_classlist_32;
    
    struct section * m_objc_protolist_32;
    
    struct section * m_objc_catlist_32;
    
    // for 64bits cpu
    struct mach_header_64 * m_mach_header_64;
    
    struct segment_command_64 * m_base_cmd_segment_64;
    
    struct segment_command_64 * m_linkedit_cmd_segment_64;
    
    struct nlist_64 * m_symbol_table_64;
    
    struct section_64 * m_objc_methodname_64;
    
    struct section_64 * m_objc_classname_64;
    
    struct section_64 * m_objc_classlist_64;
    
    struct section_64 * m_objc_protolist_64;
    
    struct section_64 * m_objc_catlist_64;
    
#endif


};

#endif /* macho_module_hpp */
