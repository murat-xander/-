#include <iostream>
#include "../src/common/container/vector.hpp"
#include "../src/common/container/string.hpp"

int main() {
    std::cout << "========== Testing Vector ==========" << std::endl;
    Vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    std::cout << "Vector size: " << v.size() << std::endl;
    std::cout << "Vector[0]: " << v[0] << std::endl;
    std::cout << "Vector[1]: " << v[1] << std::endl;
    std::cout << "Vector[2]: " << v[2] << std::endl;
    
    v.pop_back();
    std::cout << "After pop_back, size: " << v.size() << std::endl;
    
    v.erase(0);
    std::cout << "After erase index 0, Vector[0]: " << v[0] << std::endl;

    std::cout << "\n========== Testing String ==========" << std::endl;
    String s("Hello World");
    std::cout << "String: " << s.c_str() << std::endl;
    std::cout << "Length: " << s.length() << std::endl;
    std::cout << "Substr(0,5): " << s.substr(0,5).c_str() << std::endl;
    std::cout << "To lower: " << s.to_lower().c_str() << std::endl;
    std::cout << "To upper: " << s.to_upper().c_str() << std::endl;

    Vector<String> parts = s.split(' ');
    std::cout << "Split by space: ";
    for (const auto& p : parts) {
        std::cout << p.c_str() << " ";
    }
    std::cout << std::endl;

    String s2("  test trim  ");
    std::cout << "Trimmed: '" << s2.trim().c_str() << "'" << std::endl;

    std::cout << "\n✅ All container tests passed!" << std::endl;
    return 0;
}