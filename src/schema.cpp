// schema.cpp: the list of columns a query can refer to, and their types.
//
// Purpose
//   Answers one question for the rest of the engine: "does this column
//   exist, and what type is it?" Filled in once when data is loaded, then
//   only read. The type checker holds it as a read-only reference.
//
// Public
//   add(name, type)     Registers a column. Returns false, and changes
//                       nothing, if the name already exists. The caller must
//                       use the result ([[nodiscard]] in the header).
//                       Takes the name by value so a caller can hand it over
//                       with std::move instead of copying.
//   lookup(name)        Returns the column's type, or "nothing"
//                       (std::optional) if it doesn't exist. Takes a view,
//                       since it only reads. Uses find(), never [], because
//                       [] would insert a missing name.
//   column_names()      Returns a copy of every name, for "did you mean"
//                       suggestions. Only runs when a field is unknown
//                       (a user typo), so the copy's cost doesn't matter.
//
// Known limits
//   - lookup() builds a temporary std::string to search the map. Accepted
//     short-term; the long-term fix is letting the map search by a view
//     directly (transparent hashing).
//   - Names are case-sensitive: "Level" is not "level".
//   - No particular order: column_names() comes back in whatever order the
//     map holds, and the schema doesn't remember insertion order.
//   - Columns can be added but never removed or retyped.
#include "qlang/schema.hpp"

namespace qlang {
// Insert a new column into the schema.
// Returns true for success
bool Schema::add(std::string name, ColumnType type) {
    auto inserted = cols_.try_emplace(std::move(name), type);
    return inserted.second;
}

// Returns the type of an existing column, otherwise nullopt
std::optional<ColumnType> Schema::lookup(std::string_view name) const {
    const auto it = cols_.find(std::string(name));
    if (it != cols_.end()) {
        return it->second; // second is the value, first is the key
    }
    return std::nullopt;
}

//TODO: Test copying on a variety of cols_
// Returns the list of column names
std::vector<std::string> Schema::column_names() const {
    std::vector<std::string> names;
    //? Preallocation to reduce overhead
    names.reserve(cols_.size());
    for (const auto& pair : cols_) {
        names.push_back(pair.first);
    }
    return names;
}

} // qlang
