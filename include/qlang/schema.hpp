#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace qlang {

enum class ColumnType {Int, Double, String, Bool};

class Schema {
public:
    bool add(std::string name, ColumnType type);
    std::optional<ColumnType> lookup(std::string_view name) const;

    //* For "did you mean...?" suggestions.
    std::vector<std::string> column_names() const;
    //? Since we look name by name, returning a data structure with O(1) membership checking is pointless. std::vector does the job better (considering overall cost and overhead)

private:
    std::unordered_map<std::string, ColumnType> cols_;
};

}
