#ifndef PARSER_HPP
#define PARSER_HPP

#include "tokenizer.hpp"
#include "../storage/column.hpp"
#include "../storage/row.hpp"

// SQL命令类型枚举
enum class CommandType {
    CREATE_DATABASE, // 创建数据库
    DROP_DATABASE,   // 删除数据库
    USE_DATABASE,    // 切换数据库
    CREATE_TABLE,    // 创建表
    DROP_TABLE,      // 删除表
    SELECT,          // 查询
    INSERT,          // 插入
    DELETE,          // 删除
    UPDATE,          // 更新
    EXIT,            // 退出
    UNKNOWN          // 未知命令
};

// SQL命令结构体：存储解析后的命令信息
struct Command {
    CommandType type;         // 命令类型
    String db_name;           // 数据库名
    String table_name;        // 表名
    Vector<Column> columns;   // 列定义（用于CREATE TABLE）
    String column;            // SELECT的显示列
    String cond_column;       // ✅ 新增：WHERE子句的条件列
    String op;                // 运算符（用于WHERE子句）
    Value value;              // 条件值（用于WHERE子句）
    Vector<Value> values;     // 插入的值（用于INSERT）
    String set_column;        // 要更新的列名（用于UPDATE）
    Value set_value;          // 更新后的值（用于UPDATE）

    // 构造函数：初始化默认值
    Command() : type(CommandType::UNKNOWN), value(0), set_value(0) {}
};

// 语法分析器：将词法单元转换为命令对象
class Parser {
private:
    Vector<Token> tokens_; // 词法单元列表
    size_t pos_;           // 当前解析位置

    // 查看当前词法单元
    const Token& peek() const {
        return tokens_[pos_];
    }

    // 消费当前词法单元
    const Token& consume() {
        return tokens_[pos_++];
    }

    // 匹配指定类型和值的词法单元
    bool match(TokenType type, const String& value = "") {
        if (peek().type != type) return false;
        if (!value.empty() && peek().value != value) return false;
        consume();
        return true;
    }

    // 期望指定类型和值的词法单元，不匹配则抛出异常
    void expect(TokenType type, const String& value = "") {
        if (!match(type, value)) {
            String msg = "Expected " + token_type_to_string(type);
            if (!value.empty()) msg += " '" + value + "'";
            msg += ", got '" + peek().value + "'";
            throw std::runtime_error(msg.c_str());
        }
    }

    // 将词法单元类型转换为字符串
    String token_type_to_string(TokenType type) const {
        switch (type) {
            case TokenType::KEYWORD: return "keyword";
            case TokenType::IDENTIFIER: return "identifier";
            case TokenType::INT_LITERAL: return "integer literal";
            case TokenType::STRING_LITERAL: return "string literal";
            case TokenType::OPERATOR: return "operator";
            case TokenType::PUNCTUATION: return "punctuation";
            case TokenType::EOF_TOKEN: return "end of input";
            default: return "unknown";
        }
    }

    // 解析CREATE命令
    Command parse_create() {
        if (match(TokenType::KEYWORD, "database")) {
            // CREATE DATABASE
            Command cmd;
            cmd.type = CommandType::CREATE_DATABASE;
            cmd.db_name = consume().value;
            return cmd;
        } else if (match(TokenType::KEYWORD, "table")) {
            // CREATE TABLE
            Command cmd;
            cmd.type = CommandType::CREATE_TABLE;
            cmd.table_name = consume().value;
            expect(TokenType::PUNCTUATION, "(");
            
            // 解析列定义
            while (!match(TokenType::PUNCTUATION, ")")) {
                String col_name = consume().value; // 读取列名
                String type_str = consume().value.to_lower();
                DataType type;
                if (type_str == "int") type = DataType::INT;
                else if (type_str == "string") type = DataType::STRING;
                else throw std::runtime_error(("Unknown data type: " + type_str).c_str());
                // 解析主键约束
                bool is_primary = match(TokenType::KEYWORD, "primary");
                cmd.columns.push_back(Column(col_name, type, is_primary));
                
                // 处理逗号分隔
                if (!match(TokenType::PUNCTUATION, ",")) {
                    expect(TokenType::PUNCTUATION, ")");
                    break;
                }
            }
            return cmd;
        } else {
            throw std::runtime_error("Expected 'database' or 'table' after 'create'");
        }
    }

    // 解析DROP命令
    Command parse_drop() {
        if (match(TokenType::KEYWORD, "database")) {
            // DROP DATABASE
            Command cmd;
            cmd.type = CommandType::DROP_DATABASE;
            cmd.db_name = consume().value;
            return cmd;
        } else if (match(TokenType::KEYWORD, "table")) {
            // DROP TABLE
            Command cmd;
            cmd.type = CommandType::DROP_TABLE;
            cmd.table_name = consume().value;
            return cmd;
        } else {
            throw std::runtime_error("Expected 'database' or 'table' after 'drop'");
        }
    }

    // 解析USE命令
    Command parse_use() {
        Command cmd;
        cmd.type = CommandType::USE_DATABASE;
        cmd.db_name = consume().value;
        return cmd;
    }

    // 解析SELECT命令
    Command parse_select() {
        Command cmd;
        cmd.type = CommandType::SELECT;
        
        // 解析列名或*
        if (match(TokenType::KEYWORD, "*")) {
            cmd.column = "*";
        } else {
            cmd.column = consume().value;
        }
        
        expect(TokenType::KEYWORD, "from");
        cmd.table_name = consume().value;
        
        // 解析WHERE子句（条件列保存到cond_column，不覆盖显示列）
        if (match(TokenType::KEYWORD, "where")) {
            cmd.cond_column = consume().value; // ✅ 修复：使用cond_column
            cmd.op = consume().value;
            if (peek().type == TokenType::INT_LITERAL) {
                cmd.value = Value(atoi(consume().value.c_str()));
            } else if (peek().type == TokenType::STRING_LITERAL) {
                cmd.value = Value(consume().value);
            } else {
                throw std::runtime_error("Expected value in where clause");
            }
        }
        
        return cmd;
    }

    // 解析INSERT命令
    Command parse_insert() {
        Command cmd;
        cmd.type = CommandType::INSERT;
        cmd.table_name = consume().value;
        expect(TokenType::KEYWORD, "values");
        expect(TokenType::PUNCTUATION, "(");
        
        // 解析插入的值
        while (!match(TokenType::PUNCTUATION, ")")) {
            if (peek().type == TokenType::INT_LITERAL) {
                cmd.values.push_back(Value(atoi(consume().value.c_str())));
            } else if (peek().type == TokenType::STRING_LITERAL) {
                cmd.values.push_back(Value(consume().value));
            } else {
                throw std::runtime_error("Expected value in insert");
            }
            
            // 处理逗号分隔
            if (!match(TokenType::PUNCTUATION, ",")) {
                expect(TokenType::PUNCTUATION, ")");
                break;
            }
        }
        
        return cmd;
    }

    // 解析DELETE命令
    Command parse_delete() {
        Command cmd;
        cmd.type = CommandType::DELETE;
        cmd.table_name = consume().value;
        
        // 解析WHERE子句
        if (match(TokenType::KEYWORD, "where")) {
            cmd.cond_column = consume().value; // ✅ 修复：使用cond_column
            cmd.op = consume().value;
            if (peek().type == TokenType::INT_LITERAL) {
                cmd.value = Value(atoi(consume().value.c_str()));
            } else if (peek().type == TokenType::STRING_LITERAL) {
                cmd.value = Value(consume().value);
            } else {
                throw std::runtime_error("Expected value in where clause");
            }
        }
        
        return cmd;
    }

    // 解析UPDATE命令
    Command parse_update() {
        Command cmd;
        cmd.type = CommandType::UPDATE;
        cmd.table_name = consume().value;
        expect(TokenType::KEYWORD, "set");
        cmd.set_column = consume().value;
        expect(TokenType::OPERATOR, "=");
        
        // 解析更新后的值
        if (peek().type == TokenType::INT_LITERAL) {
            cmd.set_value = Value(atoi(consume().value.c_str()));
        } else if (peek().type == TokenType::STRING_LITERAL) {
            cmd.set_value = Value(consume().value);
        } else {
            throw std::runtime_error("Expected value in set clause");
        }
        
        // 解析WHERE子句
        if (match(TokenType::KEYWORD, "where")) {
            cmd.cond_column = consume().value; // ✅ 修复：使用cond_column
            cmd.op = consume().value;
            if (peek().type == TokenType::INT_LITERAL) {
                cmd.value = Value(atoi(consume().value.c_str()));
            } else if (peek().type == TokenType::STRING_LITERAL) {
                cmd.value = Value(consume().value);
            } else {
                throw std::runtime_error("Expected value in where clause");
            }
        }
        
        return cmd;
    }

public:
    // 构造函数
    Parser(const Vector<Token>& tokens) : tokens_(tokens), pos_(0) {}

    // 解析主方法
    Command parse() {
        if (match(TokenType::KEYWORD, "create")) return parse_create();
        if (match(TokenType::KEYWORD, "drop")) return parse_drop();
        if (match(TokenType::KEYWORD, "use")) return parse_use();
        if (match(TokenType::KEYWORD, "select")) return parse_select();
        if (match(TokenType::KEYWORD, "insert")) return parse_insert();
        if (match(TokenType::KEYWORD, "delete")) return parse_delete();
        if (match(TokenType::KEYWORD, "update")) return parse_update();
        if (match(TokenType::KEYWORD, "exit")) {
            Command cmd;
            cmd.type = CommandType::EXIT;
            return cmd;
        }
        
        throw std::runtime_error(("Unknown command: " + peek().value).c_str());
    }
};

#endif // PARSER_HPP