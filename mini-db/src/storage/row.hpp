#ifndef ROW_HPP
#define ROW_HPP

#include "../common/container/vector.hpp"
#include "../common/container/string.hpp"
#include "column.hpp"

class Value {
private:
    enum class Type { INT, STRING };
    Type type_;
    int int_val_;
    String str_val_;

public:
    Value(int val) : type_(Type::INT), int_val_(val) {}
    Value(const String& val) : type_(Type::INT), int_val_(0)
    {
        constexpr int MAX_STR = 256;
        if (val.length() > MAX_STR)
        {
            str_val_ = String(val.c_str(), MAX_STR);
        }
        else
        {
            str_val_ = val;
        }
        type_ = Type::STRING;
    }

    DataType get_type() const { 
        return (type_ == Type::INT) ? DataType::INT : DataType::STRING; 
    }

    int get_int() const { 
        if (type_ != Type::INT) 
            throw std::runtime_error("Value is not an integer");
        return int_val_; 
    }

    const String& get_string() const { 
        if (type_ != Type::STRING) 
            throw std::runtime_error("Value is not a string");
        return str_val_; 
    }

    String to_string() const {
        if (type_ == Type::INT) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", int_val_);
            return String(buf);
        }
        return str_val_;
    }

    bool operator==(const Value& other) const {
        if (type_ != other.type_) return false;
        if (type_ == Type::INT) return int_val_ == other.int_val_;
        return str_val_ == other.str_val_;
    }

    bool operator!=(const Value& other) const {
        return !(*this == other);
    }

    bool operator<(const Value& other) const {
        if (type_ != other.type_) 
            throw std::runtime_error("Cannot compare values of different types");
        if (type_ == Type::INT) return int_val_ < other.int_val_;
        return str_val_ < other.str_val_;
    }

    bool operator>(const Value& other) const {
        return other < *this;
    }

    bool operator<=(const Value& other) const {
        return !(*this > other);
    }

    bool operator>=(const Value& other) const {
        return !(*this < other);
    }
};

class Row {
private:
    Vector<Value> values_;

public:
    Row() = default;
    Row(const Vector<Value>& vals) : values_(vals) {}

    void add_value(const Value& val) { values_.push_back(val); }

    // 新增：update 所需赋值方法
    void set_value(size_t idx, const Value& val)
    {
        if (idx < values_.size())
        {
            values_[idx] = val;
        }
    }

    const Value& get_value(size_t idx) const { 
        if (idx >= values_.size()) 
            throw std::out_of_range("Row index out of range");
        return values_[idx]; 
    }

    size_t size() const { return values_.size(); }

    String serialize() const {
        String res;
        for (size_t i = 0; i < values_.size(); ++i) {
            if (i > 0) res += ",";
            res += values_[i].to_string();
        }
        return res;
    }

    static Row deserialize(const String& str, const Vector<Column>& cols) {
        Vector<String> parts = str.split(',');
        if (parts.size() != cols.size()) 
            throw std::invalid_argument(("Row size mismatch: expected " + String(std::to_string(cols.size()).c_str()) + ", got " + String(std::to_string(parts.size()).c_str())).c_str());
        
        Row row;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (cols[i].get_type() == DataType::INT) {
                int val = atoi(parts[i].c_str());
                row.add_value(Value(val));
            } else {
                row.add_value(Value(parts[i]));
            }
        }
        return row;
    }
};

#endif // ROW_HPP