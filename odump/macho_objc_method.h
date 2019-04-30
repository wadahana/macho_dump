//
//  macho_objc_method.h
//  odump
//
//  Created by wadahana on 22/06/2017.
//  Copyright © 2017 . All rights reserved.
//

#ifndef macho_objc_method_h
#define macho_objc_method_h

#include <stdio.h>
#include <string>


class macho_objc_method {
public:
    macho_objc_method();
    ~macho_objc_method();
    
    void set_name(const char * name) {this->m_name = name;}
    void set_type(const char * type) {this->m_type = type;}
    void set_addr(uint64_t addr) {this->m_address = addr;}
    
    std::string & get_name(void) {return this->m_name;}
    std::string & get_type(void) {return this->m_type;}
    uint64_t get_addr(void) {return this->m_address;}
    
protected:
    std::string m_name;
    std::string m_type;
    uint64_t m_imp_address;
};

#endif /* macho_objc_method_h */
