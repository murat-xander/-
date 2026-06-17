#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "../parser/parser.hpp"
#include "../storage/database.hpp"
#include <filesystem>

namespace fs = std::filesystem;

class Engine {
private:
    Database* current_db_;

    String format_result(const Vector<Row>& rows, const Vector<Column>& cols, const String& col_name) const {
        String res;
        Vector<Column> display_cols;
        
        if (col_name == "*") {
            display_cols = cols;
        } else {
            int idx = -1;
            for (size_t i = 0; i < cols.size(); ++i) {
                if (cols[i].get_name() == col_name) {
                    idx = i;
                    break;
                }
            }
            if (idx == -1) throw std::runtime_error(("Column not found: " + col_name).c_str());
            display_cols.push_back(cols[idx]);
        }

        for (size_t i = 0; i < display_cols.size(); ++i) {
            if (i > 0) res += "\t";
            res += display_cols[i].get_name();
        }
        res += "\n";

        for (size_t i = 0; i < display_cols.size(); ++i) {
            if (i > 0) res += "\t";
            res += "--------";
        }
        res += "\n";

        for (const auto& row : rows) {
            for (size_t i = 0; i < display_cols.size(); ++i) {
                if (i > 0) res += "\t";
                if (col_name == "*") {
                    res += row.get_value(i).to_string();
                } else {
                    int idx = -1;
                    for (size_t j = 0; j < cols.size(); ++j) {
                        if (cols[j].get_name() == col_name) {
                            idx = j;
                            break;
                        }
                    }
                    res += row.get_value(idx).to_string();
                }
            }
            res += "\n";
        }

        res += String(std::to_string(rows.size()).c_str()) + " rows in set";
        return res;
    }

public:
    Engine() : current_db_(nullptr) {
        // 修复点：确保 data 目录存在
        std::error_code ec;
        fs::create_directories("data", ec);
    }

    ~Engine() { delete current_db_; }

    String execute(const String& sql) {
        try {
            Tokenizer tokenizer(sql);
            Vector<Token> tokens = tokenizer.tokenize();
            Parser parser(tokens);
            Command cmd = parser.parse();

            switch (cmd.type) {
                case CommandType::CREATE_DATABASE:
                    Database::create(cmd.db_name);
                    return "Query OK, 1 database created";

                case CommandType::DROP_DATABASE:
                    if (current_db_ != nullptr && current_db_->get_name() == cmd.db_name) {
                        delete current_db_;
                        current_db_ = nullptr;
                    }
                    Database::drop(cmd.db_name);
                    return "Query OK, 1 database dropped";

                case CommandType::USE_DATABASE:
                    delete current_db_;
                    current_db_ = new Database(cmd.db_name);
                    return "Database changed to " + cmd.db_name;

                case CommandType::CREATE_TABLE:
                    if (!current_db_) throw std::runtime_error("No database selected");
                    current_db_->create_table(cmd.table_name, cmd.columns);
                    return "Query OK, 0 rows affected";

                case CommandType::DROP_TABLE:
                    if (!current_db_) throw std::runtime_error("No database selected");
                    current_db_->drop_table(cmd.table_name);
                    return "Query OK, 0 rows affected";

                case CommandType::SELECT: {
                    if (!current_db_) throw std::runtime_error("No database selected");
                    Table table = current_db_->get_table(cmd.table_name);
                    Vector<Row> rows;
                    
                    if (cmd.op.empty()) {
                        rows = table.select_all();
                    } else {
                        // ✅ 修复：使用cond_column作为查询条件列
                        rows = table.select(cmd.cond_column, cmd.op, cmd.value);
                    }
                    
                    // 显示列仍然使用cmd.column
                    return format_result(rows, table.get_columns(), cmd.column);
                }

                case CommandType::INSERT: {
                    if (!current_db_) throw std::runtime_error("No database selected");
                    Table table = current_db_->get_table(cmd.table_name);
                    table.insert_row(Row(cmd.values));
                    return "Query OK, 1 row affected";
                }

                case CommandType::DELETE: {
                    if (!current_db_) throw std::runtime_error("No database selected");
                    Table table = current_db_->get_table(cmd.table_name);
                    size_t count;
                    
                    if (cmd.op.empty()) {
                        count = table.get_row_count();
                        Table::drop(current_db_->get_path(), cmd.table_name);
                        current_db_->create_table(cmd.table_name, table.get_columns());
                    } else {
                        // ✅ 修复：使用cond_column作为删除条件列
                        count = table.delete_rows(cmd.cond_column, cmd.op, cmd.value);
                    }
                    
                    return "Query OK, " + String(std::to_string(count).c_str()) + " rows affected";
                }

                case CommandType::UPDATE: {
                    if (!current_db_) throw std::runtime_error("No database selected");
                    Table table = current_db_->get_table(cmd.table_name);
                    // ✅ 修复：使用cond_column作为更新条件列
                    size_t count = table.update_rows(cmd.set_column, cmd.set_value, 
                                                     cmd.cond_column, cmd.op, cmd.value);
                    return "Query OK, " + String(std::to_string(count).c_str()) + " rows affected";
                }
                
                case CommandType::EXIT:
                    return "exit";

                default:
                    return "Error: Unsupported command";
            }
        } catch (const std::exception& e) {
            return "Error: " + String(e.what());
        }
    }
};

#endif // ENGINE_HPP