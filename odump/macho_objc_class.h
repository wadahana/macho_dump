//
//  macho_objc_class.hpp
//  odump
//
//  Created by wadahana on 22/06/2017.
//  Copyright © 2017 . All rights reserved.
//

#ifndef macho_objc_class_h
#define macho_objc_class_h

#include <stdio.h>
#include <string>
#include <vector>
#include <memory>

#include "macho_objc_method.h"

class macho_objc_class {

public:

    macho_objc_class();
    ~macho_objc_class();
    
    bool add_class_method(macho_objc_method & method);
    bool add_instance_method(macho_objc_method & method);

protected:

    std::string m_class_name;
    std::string m_super_class_name;
    
    std::vector<std::shared_ptr<macho_objc_method>> m_class_method;
    std::vector<std::shared_ptr<macho_objc_method>> m_instance_method;

};

#endif /* macho_objc_class_h */
