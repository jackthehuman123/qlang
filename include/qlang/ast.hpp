#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <variant>

namespace qlang {

using Literal = std::variant<int64_t, double, std::string, bool>; //* type alias

enum class CmpOp {Eq, NotEq, Gt, GtEq, Lt, LtEq, Contains};
enum class BoolOp {And, Or};

struct Node {
    virtual ~Node() = default; //* Allow complete deallocation of members within derived classes
};

//* Resource Acquisition Is Initialization
using NodePtr = std::unique_ptr<Node>; 

struct Predicate : Node {
    std::string field;
    CmpOp       op;
    Literal     value;
};

struct BinaryExpr : Node {
    BoolOp  op;
    NodePtr lhs;
    NodePtr rhs;
};

struct NotExpr : Node {
    NodePtr operand;
};

} //* namespace qlang


//* a:1 OR b:2 AND c:3