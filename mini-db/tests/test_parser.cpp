#include <iostream>
#include "../src/parser/tokenizer.hpp"
#include "../src/parser/parser.hpp"

int main() {
    std::cout << "========== Testing Tokenizer ==========" << std::endl;
    String sql("create table person (id int primary, name string)");
    Tokenizer tokenizer(sql);
    Vector<Token> tokens = tokenizer.tokenize();
    
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << token.value.c_str() << " ";
    }
    std::cout << std::endl;

    std::cout << "\n========== Testing Parser ==========" << std::endl;
    Parser parser(tokens);
    Command cmd = parser.parse();
    
    if (cmd.type == CommandType::CREATE_TABLE) {
        std::cout << "✅ Parsed CREATE TABLE command successfully" << std::endl;
        std::cout << "Table name: " << cmd.table_name.c_str() << std::endl;
        std::cout << "Columns: " << cmd.columns.size() << std::endl;
    } else {
        std::cerr << "❌ Parser test failed" << std::endl;
        return 1;
    }

    std::cout << "\n✅ All parser tests passed!" << std::endl;
    return 0;
}