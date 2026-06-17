#ifndef COLUMN_HPP
#define COLUMN_HPP

#include "../common/container/string.hpp"

enum class DataType { INT, STRING };

class Column {
private:
    String name_;
    DataType type_;
    bool is_primary_;

public:
    Column(const String& name, DataType type, bool is_primary = false)
        : name_(name), type_(type), is_primary_(is_primary) {}

    const String& get_name() const { return name_; }
    DataType get_type() const { return type_; }
    bool is_primary() const { return is_primary_; }

    String serialize() const {
        String type_str = (type_ == DataType::INT) ? "int" : "string";
        String pri_str = is_primary_ ? " primary" : "";
        return name_ + " " + type_str + pri_str;
    }

    static Column deserialize(const String& str) {
        Vector<String> parts = str.split(' ');
        Vector<String> filtered;
        for (const auto& p : parts) {
            if (!p.empty()) filtered.push_back(p);
        }
        if (filtered.size() < 2) 
            throw std::invalid_argument(("Invalid column format: " + str).c_str());
        
        DataType type;
        if (filtered[1] == "int") type = DataType::INT;
        else if (filtered[1] == "string") type = DataType::STRING;
        else throw std::invalid_argument(("Unknown data type: " + filtered[1]).c_str());
        
        bool is_pri = (filtered.size() >= 3 && filtered[2] == "primary");
        return Column(filtered[0], type, is_pri);
    }
};

#endif // COLUMN_HPPs