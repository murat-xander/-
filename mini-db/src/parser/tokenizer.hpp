#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include "../common/container/string.hpp"
#include "../common/container/vector.hpp"
#include <cctype>
#include <stdexcept>

enum class TokenType {
    KEYWORD,
    IDENTIFIER,
    INT_LITERAL,
    STRING_LITERAL,
    OPERATOR,
    PUNCTUATION,
    EOF_TOKEN
};

struct Token {
    TokenType type;
    String value;
    Token(TokenType t, const String& v) : type(t), value(v) {}
};

class Tokenizer {
private:
    String input_;
    size_t pos_;

    char peek() const { 
        return (pos_ >= input_.length()) ? '\0' : input_[pos_]; 
    }

    char consume() { 
        return input_[pos_++]; 
    }

    void skip_whitespace() { 
        while (isspace(static_cast<unsigned char>(peek()))) 
            consume(); 
    }

    Token read_keyword_or_id() {
        size_t start = pos_;
        while (isalnum(static_cast<unsigned char>(peek()))) 
            consume();
        String val = input_.substr(start, pos_ - start).to_lower();
        
        // 使用初始化列表构造Vector
        static const Vector<String> keywords = {
            "create", "drop", "use", "database", "table", "select", "from",
            "where", "delete", "insert", "values", "update", "set", "int",
            "string", "primary", "exit", "*"
        };
        
        for (const auto& kw : keywords) {
            if (val == kw) {
                return Token(TokenType::KEYWORD, val);
            }
        }
        return Token(TokenType::IDENTIFIER, val);
    }

    Token read_int() {
        size_t start = pos_;
        while (isdigit(static_cast<unsigned char>(peek()))) 
            consume();
        return Token(TokenType::INT_LITERAL, input_.substr(start, pos_ - start));
    }

    Token read_string() {
        consume();
        size_t start = pos_;
        while (peek() != '"' && peek() != '\0') 
            consume();
        if (peek() == '\0') 
            throw std::runtime_error("Unterminated string literal");
        String val = input_.substr(start, pos_ - start);
        consume();
        return Token(TokenType::STRING_LITERAL, val);
    }

public:
    Tokenizer(const String& input) : input_(input.trim()), pos_(0) {}

    Vector<Token> tokenize() {
        Vector<Token> tokens;
        while (pos_ < input_.length()) {
            skip_whitespace();
            if (pos_ >= input_.length()) break;
            
            char c = peek();
            if (isalpha(static_cast<unsigned char>(c))) {
                tokens.push_back(read_keyword_or_id());
            } else if (isdigit(static_cast<unsigned char>(c))) {
                tokens.push_back(read_int());
            } else if (c == '"') {
                tokens.push_back(read_string());
            } else if (c == '=' || c == '<' || c == '>') {
                String op(&c, 1);
                consume();
                tokens.push_back(Token(TokenType::OPERATOR, op));
            } else if (c == '(' || c == ')' || c == ',' || c == ';') {
                String punc(&c, 1);
                consume();
                tokens.push_back(Token(TokenType::PUNCTUATION, punc));
            } else if (c == '*') {
                consume();
                tokens.push_back(Token(TokenType::KEYWORD, "*"));
            } else {
                throw std::runtime_error(("Unexpected character: " + String(&c, 1)).c_str());
            }
        }
        tokens.push_back(Token(TokenType::EOF_TOKEN, ""));
        return tokens;
    }
};

#endif // TOKENIZER_HPP