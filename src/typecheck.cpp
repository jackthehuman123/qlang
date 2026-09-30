// typecheck.cpp: rejects queries that don't make sense for the data.
//
// Purpose
//   Third stage of the pipeline. Walks the tree and checks every comparison
//   against the schema, so a bad query fails before any row is read, not
//   partway through a scan. Checks meaning (semantics); the parser has
//   already checked grammar (syntax).
//       levl:error     →  unknown field 'levl' / did you mean 'level'?
//       message:>10    →  operator ':>' needs a numeric column
//       count:"hello"  →  field 'count' is a whole number, value is text
//
// Contract
//   - Returns every error found; an empty list means the query is valid.
//   - User mistakes are collected, never thrown, so one run reports them all.
//   - An unknown node kind throws std::logic_error: that's a bug in our own
//     code, not bad input.
//   - Errors come out in the order the user typed them (left child first).
//
// Per predicate, three checks in order, stopping at the first failure:
//   1. Field exists. If not, suggest the closest names (see below).
//   2. Operator fits the column type:
//        equality  (: :!)          any column
//        ordering  (:> :>= :< :<=) whole number or decimal columns
//        contains  (~)             text columns
//   3. Value fits the column type. A whole number is accepted in a decimal
//      column (no information lost); a decimal in a whole-number column is not.
//   Stopping matters because each check needs the one before it: no field
//   means no type to check against; a wrong operator makes a value error
//   just noise. Sibling predicates are still checked independently.
//
// Public
//   typecheck(ast, schema)   Entry point. Takes the tree by reference, so
//                            the derived node types aren't cut down to the
//                            base (object slicing).
//
// File-private helpers (anonymous namespace)
//   check_expr        Tells node kinds apart with dynamic_cast; recurses.
//   check_pred        The three checks above.
//   classify, family_accepts, family_requirement
//                     Group operators into families and match them to types.
//   column_accepts    Value type vs. column type, including the widening rule.
//   lit_to_col_type   Maps a value to its column type (std::visit + overloaded).
//   cmp_to_string, col_type_to_string, join_suggestions
//                     Text for error messages.
//   edit_dis          Edit distance: the fewest single-letter inserts,
//                     deletes, or substitutions turning one word into another.
//   suggest_names     Names within 2 edits, keeping only the closest ones,
//                     sorted so output doesn't depend on the map's order.
//
// Known limits
//   - Every error's position is 0 until Predicate records where it came from.
//   - The 2-edit suggestion limit is loose for very short names.
//   - A bare true/false value is typed true/false by the parser, so a text
//     column can only match the word "true" when it's quoted.

#include "qlang/typecheck.hpp"
#include "qlang/schema.hpp"
#include "qlang/ast.hpp"
#include <string_view>
#include <algorithm>
#include <stdexcept>
#include <format>
#include <string>
#include <vector>

namespace qlang {

// --- Hidden helper functions---
namespace {

    enum class OpFamily {Equality, Ordering, Substring};

    // Classify the operator to it's according family
    [[nodiscard]] OpFamily classify(CmpOp op) {
        switch (op) {
            case CmpOp::Eq:
            case CmpOp::NotEq:      return OpFamily::Equality;
            case CmpOp::Gt:
            case CmpOp::GtEq:
            case CmpOp::Lt:
            case CmpOp::LtEq:       return OpFamily::Ordering;
            case CmpOp::Contains:   return OpFamily::Substring;
        }
        return OpFamily::Equality;
    }

    // Validify operators with matching column
    [[nodiscard]] bool family_accepts(OpFamily family, ColumnType type) {
        switch (family) {
            case OpFamily::Equality:  return true;
            case OpFamily::Ordering:  return type == ColumnType::Int
                                          || type == ColumnType::Double;
            case OpFamily::Substring: return type == ColumnType::String;
        }
        return false;
    }

    // Phrased for the error message: what the operator needs.
    [[nodiscard]] std::string_view family_requirement(OpFamily family) {
        switch (family) {
            case OpFamily::Equality:  return "any column type";
            case OpFamily::Ordering:  return "a numeric column";
            case OpFamily::Substring: return "a string column";
        }
        return "";
    }

    // Return string represenation of the op
    std::string cmp_to_string(CmpOp op) {
        switch (op) {
            case CmpOp::Eq:         {return std::string{":"};}
            case CmpOp::NotEq:      {return std::string{":!"};}
            case CmpOp::Gt:         {return std::string{":>"};}
            case CmpOp::GtEq:       {return std::string{":>="};}
            case CmpOp::Lt:         {return std::string{":<"};}
            case CmpOp::LtEq:       {return std::string{":<="};}
            case CmpOp::Contains:   {return std::string{"~"};}
        }
        return "unknown operator";
    }

    // Compare field type and value type
    [[nodiscard]] bool column_accepts(ColumnType field, ColumnType value) {
        if (field == ColumnType::Double) {
            if (value == ColumnType::Int) {
                return true;
            }
        }
        return (field == value);
    }

    template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

    // Converting a literal into the corresponding column type
    ColumnType lit_to_col_type(const Literal& lit) {
        return (std::visit(overloaded{
            [](int64_t) { return ColumnType::Int; },
            [](double) { return ColumnType::Double; },
            [](const std::string&) { return ColumnType::String; },
            [](bool) { return  ColumnType::Bool; }, // C-literals decays to const char* not std::string
        }, lit));
    }

    // Returing a corresponding string to the column type
    std::string col_type_to_string(ColumnType type) {
        switch (type) {
            case ColumnType::Int: {return std::string{"Integer"};}
            case ColumnType::Double: {return std::string{"Double"};}
            case ColumnType::Bool: {return std::string{"Bool"};}
            case ColumnType::String: {return std::string{"String"};}
        }
        return "unknown column_type ";
    }

    std::vector<std::string> suggest_names(const std::string_view word1, const Schema& schema);

    // Build suggestion error string
    std::string join_suggestions(const std::vector<std::string>& names) {
        std::string out;
        for (size_t i = 0; i < names.size(); ++i) {
            if (i > 0) {
                if (names.size() == 2)          out += " or ";
                else if (i == names.size() - 1) out += ", or ";
                else                            out += ", ";
            }
            out += "'" + names[i] + "'";
        }
        return out;
    }

    // Validify a predicate
    void check_pred(const Predicate& pred, const Schema& schema, std::vector<TypeError>& errors) {
        const auto column_type = schema.lookup(pred.field);
        if (!column_type) {
            //TODO: position unavailable — nodes don't carry token offsets yet
            std::string message = std::format("unknown field '{}'", pred.field);

            // Add suggestions
            const auto suggestions = suggest_names(pred.field, schema);
            if (!suggestions.empty()) {
                message += std::format("\n  did you mean {}?", join_suggestions(suggestions));
            }

            errors.push_back(TypeError{std::move(message), 0});
            return;
        }

        const ColumnType type = *column_type;

        // Check compatible operators
        const OpFamily family = classify(pred.op);
        if (!family_accepts(family, type)) {
            errors.push_back(TypeError{
                std::format("operator '{}' requires '{}', but field '{}' is declared '{}'",
                    cmp_to_string(pred.op),
                    family_requirement(family),
                    pred.field,
                    col_type_to_string(type)),
            0});

            return; // Bad operator
        }

        // Check matching value and column type
        const ColumnType value_type = lit_to_col_type(pred.value);

        if (!column_accepts(type, value_type)) {
            errors.push_back(TypeError{
                std::format("field '{}' is declared '{}', but the value given is '{}'",
                    pred.field,
                    col_type_to_string(type),
                    col_type_to_string(value_type)),
                0});
        }
    }

    // Validify NotExpr or BinaryExpr
    void check_expr(const Node& expr, const Schema& schema, std::vector<TypeError>& errors) {
        if (auto* pred = dynamic_cast<const Predicate*>(&expr)) {
            check_pred(*pred, schema, errors);
            return;
        }
        if (auto* not_ex = dynamic_cast<const NotExpr*>(&expr)) {
            check_expr(*not_ex->operand, schema, errors);
            return;
        }
        if (auto* bi_ex = dynamic_cast<const BinaryExpr*>(&expr)) {
            check_expr(*bi_ex->lhs, schema, errors);
            check_expr(*bi_ex->rhs, schema, errors);
            return;
        }
        throw std::logic_error{"typecheck: unknown node type"};
    }

    // Returns the edit distance between word1 and word2
    int edit_dis(const std::string_view word1, const std::string_view word2) {
        int l_w1{static_cast<int>(word1.size())}; // ROWS
        int l_w2{static_cast<int>(word2.size())}; // COLS
        std::vector<std::vector<int>> grid(l_w1 + 1, std::vector<int>(l_w2 + 1, 0)); // ROWS x COLS grid of 0s
        for (int r{0}; r < l_w1 + 1; r++) {
            grid[r][0] = r; // Initialize leftmost column
        }
        for (int c{0}; c < l_w2 + 1; c++) {
            grid[0][c] = c; // Initialize topmost row
        }
        for (int r{1}; r < l_w1 + 1; r++) {
            for (int c{1}; c < l_w2 + 1; c++) {
                int left{grid[r][c-1] + 1};
                int top{grid[r-1][c] + 1};
                int diag{grid[r-1][c-1]};
                if (word1[r-1] != word2[c-1])
                    diag += 1;
                grid[r][c] = std::min({left, top, diag});
            }
        }
        return grid[l_w1][l_w2];
    }

    // Returns a list of suggested corrections for invalid fields
    std::vector<std::string> suggest_names(const std::string_view word1, const Schema& schema) {
        const int threshold{2};
        int best_so_far{threshold};
        std::vector<std::string> matches;
        for (const std::string& name : schema.column_names()) {
            int e_d{edit_dis(word1, name)};
            if (e_d == best_so_far) {
                matches.push_back(name);
            }
            else if (e_d < best_so_far) {
                matches.clear();
                best_so_far = e_d;
                matches.push_back(name);
            }
        }
        // Sort the results
        std::sort(matches.begin(), matches.end());
        return matches;
    }

    } // namespace

// const Node& to prevent object slicing
std::vector<TypeError> typecheck(const Node& ast, const Schema &schema) {
    std::vector<TypeError> errors;
    check_expr(ast, schema, errors);
    return errors;
}

} // namespace qlang
