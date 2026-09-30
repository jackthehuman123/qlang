#pragma once 

#include "qlang/ast.hpp"
#include "qlang/schema.hpp"
#include <string>
#include <vector>

namespace qlang {

struct TypeError {
    std::string message;
    size_t pos;
};

//* Return all errors found, empty vector means the query is valid 
std::vector<TypeError> typecheck(const Node& ast, const Schema& schema);
 
//* Example usage = typecheck(*root) where NodePtr root

}