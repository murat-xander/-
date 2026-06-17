#ifndef TABLE_HPP
#define TABLE_HPP

#include "../common/container/string.hpp"
#include "../common/container/vector.hpp"
#include "column.hpp"
#include "row.hpp"
#include "index.hpp"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

class Table {
private:
    String name_;
    String db_path_;
    Vector<Column> columns_;
    Index* index_;
    size_t row_count_;

    void load_schema() {
        String schema_file = db_path_ + "/" + name_ + ".schema";
        std::ifstream in(schema_file.c_str());
        if (!in) throw std::runtime_error(("Cannot open schema file: " + schema_file).c_str());

        String line;
        char buf[1024];
        while (in.getline(buf, sizeof(buf))) {
            line = String(buf).trim();
            if (line.empty()) continue;
            columns_.push_back(Column::deserialize(line));
        }

        index_ = nullptr;
        for (size_t i = 0; i < columns_.size(); ++i) {
            if (columns_[i].is_primary()) {
                index_ = new Index(db_path_ + "/" + name_, columns_[i].get_name(), 
                                   columns_[i].get_type(), i);
                break;
            }
        }

        String data_file = db_path_ + "/" + name_ + ".dat";
        std::ifstream data_in(data_file.c_str());
        row_count_ = 0;
        while (data_in.getline(buf, sizeof(buf))) {
            if (!String(buf).trim().empty()) row_count_++;
        }
    }

    void save_schema() const {
        String schema_file = db_path_ + "/" + name_ + ".schema";
        std::ofstream out(schema_file.c_str());
        if (!out) throw std::runtime_error(("Cannot write schema file: " + schema_file).c_str());

        for (const auto& col : columns_) {
            out << col.serialize().c_str() << std::endl;
        }
    }

public:
    Table(const String& db_path, const String& name) 
        : name_(name), db_path_(db_path), index_(nullptr), row_count_(0) {
        load_schema();
    }

    Table(const String& db_path, const String& name, const Vector<Column>& columns)
        : name_(name), db_path_(db_path), columns_(columns), index_(nullptr), row_count_(0) {
        String data_file = db_path + "/" + name + ".dat";
        std::ofstream out(data_file.c_str());
        if (!out) throw std::runtime_error(("Cannot create data file: " + data_file).c_str());

        save_schema();

        for (size_t i = 0; i < columns_.size(); ++i) {
            if (columns_[i].is_primary()) {
                index_ = new Index(db_path + "/" + name, columns_[i].get_name(), 
                                   columns_[i].get_type(), i);
                break;
            }
        }
    }

    ~Table() { delete index_; }

    const String& get_name() const { return name_; }
    const Vector<Column>& get_columns() const { return columns_; }
    size_t get_row_count() const { return row_count_; }

    int get_column_index(const String& col_name) const {
        for (size_t i = 0; i < columns_.size(); ++i) {
            if (columns_[i].get_name() == col_name) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    void insert_row(const Row& row) {
        if (row.size() != columns_.size()) 
            throw std::runtime_error("Row size mismatch");

        for (size_t i = 0; i < columns_.size(); ++i) {
            if (row.get_value(i).get_type() != columns_[i].get_type()) {
                throw std::runtime_error(("Data type mismatch for column: " + columns_[i].get_name()).c_str());
            }
        }

        if (index_ != nullptr) {
            const Value& key = row.get_value(index_->get_column_index());
            index_->insert(key, row_count_);
        }

        String data_file = db_path_ + "/" + name_ + ".dat";
        std::ofstream out(data_file.c_str(), std::ios::app);
        if (!out) throw std::runtime_error(("Cannot open data file for writing: " + data_file).c_str());
        out << row.serialize().c_str() << std::endl;

        row_count_++;
    }

Vector<Row> select(const String& col_name, const String& op, const Value& value) const {
    Vector<Row> res;
    int col_idx = get_column_index(col_name);
    if (col_idx == -1) throw std::runtime_error(("Column not found: " + col_name).c_str());

    // 注释索引分支，强制全表扫描，绕过B+树磁盘bug
    /*
    if (index_ != nullptr && index_->get_column_name() == col_name) {
        Vector<size_t> row_nums = index_->find_range(value, op);
        String data_file = db_path_ + "/" + name_ + ".dat";
        std::ifstream in(data_file.c_str());
        if (!in) throw std::runtime_error(("Cannot open data file: " + data_file).c_str());

        char buf[1024];
        size_t current_row = 0;
        size_t idx = 0;
        while (in.getline(buf, sizeof(buf)) && idx < row_nums.size()) {
            String line(buf);
            line.trim();
            if (line.empty()) continue;

            if (current_row == row_nums[idx]) {
                res.push_back(Row::deserialize(line, columns_));
                idx++;
            }
            current_row++;
        }
    } else {
    */
        String data_file = db_path_ + "/" + name_ + ".dat";
        std::ifstream in(data_file.c_str());
        if (!in) throw std::runtime_error(("Cannot open data file: " + data_file).c_str());

        char buf[1024];
        while (in.getline(buf, sizeof(buf))) {
            String line(buf);
            line.trim();
            if (line.empty()) continue;
            Row row = Row::deserialize(line, columns_);
            const Value& row_val = row.get_value(col_idx);

            bool match = false;
            if (op == "=") match = (row_val == value);
            else if (op == "<") match = (row_val < value);
            else if (op == ">") match = (row_val > value);
            else throw std::runtime_error(("Unsupported operator: " + op).c_str());

            if (match) res.push_back(row);
        }
    // }
    return res;
}

    Vector<Row> select_all() const {
        Vector<Row> res;
        String data_file = db_path_ + "/" + name_ + ".dat";
        std::ifstream in(data_file.c_str());
        if (!in) throw std::runtime_error(("Cannot open data file: " + data_file).c_str());

        char buf[1024];
        while (in.getline(buf, sizeof(buf))) {
            String line(buf);
            if (line.empty()) continue;
            res.push_back(Row::deserialize(line, columns_));
        }

        return res;
    }

size_t delete_rows(const String& col_name, const String& op, const Value& value) {
    int cond_idx = get_column_index(col_name);
    if (cond_idx == -1)
     throw std::runtime_error(("Column not found: " + col_name).c_str());

    String data_file = db_path_ + "/" + name_ + ".dat";
    // 临时文件，存放保留的数据
    String tmp_file = db_path_ + "/" + name_ + ".tmp";

    std::ifstream in(data_file.c_str());
    std::ofstream out(tmp_file.c_str());
    if (!in || !out)
        throw std::runtime_error("File open failed when delete");

    char buf[1024];
    size_t del_cnt = 0;
    size_t new_row_num = 0;

    // 遍历原文件，只写入不满足删除条件的行
    while (in.getline(buf, sizeof(buf)))
    {
        String line(buf);
        line.trim();
        if (line.empty()) continue;

        Row row = Row::deserialize(line, columns_);
        const Value& row_val = row.get_value(cond_idx);

        // 判断是否需要删除
        bool need_del = false;
        if (op == "=")    need_del = (row_val == value);
        else if (op == "<") need_del = (row_val < value);
        else if (op == ">") need_del = (row_val > value);

        if (need_del)
        {
            del_cnt++;
            // 如果删除的是主键，B+树索引这里简易处理：重建索引（作业最简方案）
            continue;
        }

        // 保留该行，写入临时文件
        out << row.serialize().c_str() << std::endl;
        // 新行写入索引
        if (index_ != nullptr)
        {
            Value key = row.get_value(index_->get_column_index());
            index_->insert(key, new_row_num);
        }
        new_row_num++;
    }

    in.close();
    out.close();

    // 替换原数据文件
    fs::remove(data_file.c_str());
    fs::rename(tmp_file.c_str(), data_file.c_str());

    // 更新总行数
    row_count_ = new_row_num;
    return del_cnt;
}

size_t update_rows(const String& col_name, const Value& new_val, 
                  const String& cond_col, const String& op, const Value& cond_val) {
    // 待更新列下标
    int update_idx = get_column_index(col_name);
    // 条件列下标
    int cond_idx = get_column_index(cond_col);

    if (update_idx == -1 || cond_idx == -1)
        throw std::runtime_error("Column not found");

    String data_file = db_path_ + "/" + name_ + ".dat";
    String tmp_file = db_path_ + "/" + name_ + ".tmp";

    std::ifstream in(data_file.c_str());
    std::ofstream out(tmp_file.c_str());
    if (!in || !out)
        throw std::runtime_error("File open failed when update");

    char buf[1024];
    size_t update_cnt = 0;
    size_t new_row_num = 0;
    bool is_primary_col = false;

    // 判断更新列是否为主键
    for (auto& col : columns_)
    {
        if (col.get_name() == col_name && col.is_primary())
        {
            is_primary_col = true;
            break;
        }
    }

    while (in.getline(buf, sizeof(buf)))
    {
        String line(buf);
        line.trim();
        if (line.empty()) continue;

        Row row = Row::deserialize(line, columns_);
        const Value& cond_row_val = row.get_value(cond_idx);

        // 判断是否命中更新条件
        bool need_update = false;
        if (op == "=")    need_update = (cond_row_val == cond_val);
        else if (op == "<") need_update = (cond_row_val < cond_val);
        else if (op == ">") need_update = (cond_row_val > cond_val);

        if (need_update)
        {
            row.set_value(update_idx, new_val);
            update_cnt++;
        }

        // 写入新行
        out << row.serialize().c_str() << std::endl;

        // 主键更新：重新插入索引
        if (index_ != nullptr)
        {
            Value key = row.get_value(index_->get_column_index());
            index_->insert(key, new_row_num);
        }
        new_row_num++;
    }

    in.close();
    out.close();

    // 替换原文件
    fs::remove(data_file.c_str());
    fs::rename(tmp_file.c_str(), data_file.c_str());
    row_count_ = new_row_num;

    return update_cnt;
}
    static void drop(const String& db_path, const String& name) {
        String data_file = db_path + "/" + name + ".dat";
        String schema_file = db_path + "/" + name + ".schema";
        String index_file = db_path + "/" + name + ".idx";

        if (fs::exists(data_file.c_str())) fs::remove(data_file.c_str());
        if (fs::exists(schema_file.c_str())) fs::remove(schema_file.c_str());
        if (fs::exists(index_file.c_str())) fs::remove(index_file.c_str());
    }
};

#endif // TABLE_HPP