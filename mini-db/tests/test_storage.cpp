#include <iostream>
#include "../src/storage/column.hpp"
#include "../src/storage/row.hpp"
#include "../src/storage/table.hpp"
#include "../src/storage/database.hpp"

int main() {
    std::cout << "========== Testing Storage Engine ==========" << std::endl;
    
    // 测试Column
    Column col1("id", DataType::INT, true);
    Column col2("name", DataType::STRING);
    std::cout << "Column 1: " << col1.serialize().c_str() << std::endl;
    std::cout << "Column 2: " << col2.serialize().c_str() << std::endl;

    // 测试Row
    Vector<Value> vals;
    vals.push_back(Value(1001));
    vals.push_back(Value(String("peter")));
    Row row(vals);
    std::cout << "Row serialized: " << row.serialize().c_str() << std::endl;

    // 测试Database和Table
    try {
        Database::create("test_db");
        Database db("test_db");
        
        Vector<Column> cols;
        cols.push_back(col1);
        cols.push_back(col2);
        db.create_table("person", cols);
        
        Table table = db.get_table("person");
        table.insert_row(row);
        
        Vector<Row> rows = table.select_all();
        std::cout << "Rows in table: " << rows.size() << std::endl;
        std::cout << "First row: " << rows[0].serialize().c_str() << std::endl;
        
        db.drop_table("person");
        Database::drop("test_db");
        std::cout << "✅ All storage tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "❌ Storage test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}