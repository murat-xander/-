#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "../common/container/string.hpp"
#include "../common/container/vector.hpp"
#include "table.hpp"
#include <filesystem>

namespace fs = std::filesystem;

class Database {
private:
    String name_;
    String path_;

public:
    Database(const String& name) : name_(name) {
        path_ = "data/" + name;
        if (!fs::exists(path_.c_str())) 
            throw std::runtime_error(("Database does not exist: " + name).c_str());
    }

    const String& get_name() const { return name_; }
    const String& get_path() const { return path_; }

    void create_table(String table_name, const Vector<Column>& columns) {
        table_name.trim(); // 去除首尾空格换行
        String table_path = path_ + "/" + table_name + ".dat";
        if (fs::exists(table_path.c_str())) 
            throw std::runtime_error(("Table already exists: " + table_name).c_str());
        Table(path_, table_name, columns);
    }

    Table get_table(String table_name) const {
        table_name.trim(); // 关键修复：清除空白字符
        String table_path = path_ + "/" + table_name + ".dat";
        if (!fs::exists(table_path.c_str())) 
            throw std::runtime_error(("Table does not exist: " + table_name).c_str());
        return Table(path_, table_name);
    }

    void drop_table(String table_name) {
        table_name.trim();
        String table_path = path_ + "/" + table_name + ".dat";
        if (!fs::exists(table_path.c_str())) 
            throw std::runtime_error(("Table does not exist: " + table_name).c_str());
        Table::drop(path_, table_name);
    }

    Vector<String> list_tables() const {
        Vector<String> tables;
        if (!fs::exists(path_.c_str())) return tables;
        for (const auto& entry : fs::directory_iterator(path_.c_str())) {
            if (entry.is_regular_file() && entry.path().extension() == ".dat") {
                tables.push_back(String(entry.path().stem().string().c_str()));
            }
        }
        return tables;
    }

    static void create(const String& name) {
        String path = "data/" + name;
        if (fs::exists(path.c_str())) 
            throw std::runtime_error(("Database already exists: " + name).c_str());
        
        std::error_code ec;
        fs::create_directories(path.c_str(), ec);
        if (ec) {
            String errorMsg(ec.message().c_str());
            throw std::runtime_error((String("Failed to create database directory: ") + name + String(" Error: ") + errorMsg).c_str());
        }
    }

    static void drop(const String& name) {
        String path = "data/" + name;
        if (!fs::exists(path.c_str())) 
            throw std::runtime_error(("Database does not exist: " + name).c_str());
        fs::remove_all(path.c_str());
    }

    static Vector<String> list_databases() {
        Vector<String> dbs;
        if (!fs::exists("data")) return dbs;
        for (const auto& entry : fs::directory_iterator("data")) {
            if (entry.is_directory()) {
                dbs.push_back(String(entry.path().filename().string().c_str()));
            }
        }
        return dbs;
    }
};

#endif // DATABASE_HPP