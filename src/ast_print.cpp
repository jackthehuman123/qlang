#include "qlang/ast_print.hpp"

namespace qlang {

namespace {
    std::string op_to_text(const CmpOp& op) {
        switch (op) {
            case CmpOp::Eq:         {return std::string{":"};}
            case CmpOp::NotEq:      {return std::string{":!"};}
            case CmpOp::Gt:         {return std::string{":>"};}
            case CmpOp::GtEq:       {return std::string{":>="};}
            case CmpOp::Lt:         {return std::string{":<"};}
            case CmpOp::LtEq:       {return std::string{":<="};}
            case CmpOp::Contains:   {return std::string{"~"};}
        }
        return std::string{"???"};
    }

    //! Inherits from n callables and merge their operator()s into one scope, producing a single object callable with any of their argument types.
    //! The 'using' is required: names from sibling bases don't form an overload set on their own. 
    template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };

    //! Deduction guide: overloaded{L1, l2} -> overloaded<L1, L2>
    //* Back in C++17, we had to write overloaded<T1, T2, T3>, and since lambdas are anonymous function, we can't really do that, hence the template for deduction guide.
    template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

    std::string lit_to_text(const Literal& lit) {
        return (std::visit(overloaded{
            [](int64_t n) { return std::to_string(n); },
            [](double d) { return std::to_string(d); },
            [](const std::string& s) { return s; },
            [](bool b) { return  b ? std::string{"true"} : std::string{"false"}; }, //! C-literals decays to const char* not std::string
        }, lit));
    } 

    std::string bool_to_text(const qlang::BoolOp& b) {
        switch(b) {
            case BoolOp::And: {return std::string{"AND"};}
            case BoolOp::Or: {return std::string{"OR"};}
        }
        return std::string{"???"};
    }
}

//! dereferencing a unique_ptr returns a reference to the object being pointed at 
std::string print(const Node& node) {
    if (auto* pred = dynamic_cast<const Predicate*>(&node)) {
        return (pred->field + op_to_text(pred->op) + lit_to_text(pred->value));
    } 
    if (auto* bi_ex = dynamic_cast<const BinaryExpr*>(&node)) {
        return (
            "(" + bool_to_text(bi_ex->op) + " " + print(*bi_ex->lhs) + " " + print(*bi_ex->rhs) + ")"
        );
    }
    if (auto* not_ex = dynamic_cast<const NotExpr*>(&node)) {
        return (std::string{"NOT "} + print(*not_ex->operand));
    }
    throw std::logic_error{"print: unknown node type"};
}

} // namespace qlang