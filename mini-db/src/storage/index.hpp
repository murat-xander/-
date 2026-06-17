#ifndef INDEX_HPP
#define INDEX_HPP

#include "../common/container/string.hpp"
#include "../common/container/vector.hpp"
#include "row.hpp"
#include <fstream>
#include <stdexcept>
#include <cstdio>

const int BPLUS_ORDER = 5;

class BPlusTree {
private:
    struct Node {
        bool is_leaf;
        Vector<Value> keys;
        Vector<size_t> children;
        size_t next_leaf;

        Node(bool leaf) : is_leaf(leaf), next_leaf(0) {}

        void serialize(std::ofstream& out) const {
            out.write(reinterpret_cast<const char*>(&is_leaf), sizeof(is_leaf));
            
            size_t key_cnt = keys.size();
            out.write(reinterpret_cast<const char*>(&key_cnt), sizeof(key_cnt));
            for (const auto& key : keys) {
                DataType type = key.get_type();
                out.write(reinterpret_cast<const char*>(&type), sizeof(type));
                if (type == DataType::INT) {
                    int val = key.get_int();
                    out.write(reinterpret_cast<const char*>(&val), sizeof(val));
                } else {
                    const String& str = key.get_string();
                    size_t len = str.length();
                    out.write(reinterpret_cast<const char*>(&len), sizeof(len));
                    out.write(str.c_str(), len);
                }
            }

            size_t child_cnt = children.size();
            out.write(reinterpret_cast<const char*>(&child_cnt), sizeof(child_cnt));
            for (size_t child : children) {
                out.write(reinterpret_cast<const char*>(&child), sizeof(child));
            }

            out.write(reinterpret_cast<const char*>(&next_leaf), sizeof(next_leaf));
        }

        static Node deserialize(std::ifstream& in) {
            bool is_leaf;
            in.read(reinterpret_cast<char*>(&is_leaf), sizeof(is_leaf));
            
            size_t key_cnt;
            in.read(reinterpret_cast<char*>(&key_cnt), sizeof(key_cnt));
            Node node(is_leaf);
            
            for (size_t i = 0; i < key_cnt; ++i) {
                DataType type;
                in.read(reinterpret_cast<char*>(&type), sizeof(type));
                if (type == DataType::INT) {
                    int val;
                    in.read(reinterpret_cast<char*>(&val), sizeof(val));
                    node.keys.push_back(Value(val));
                } else {
                    size_t len;
                    in.read(reinterpret_cast<char*>(&len), sizeof(len));
                    char* buf = new char[len + 1];
                    in.read(buf, len);
                    buf[len] = '\0';
                    node.keys.push_back(Value(String(buf)));
                    delete[] buf;
                }
            }

            size_t child_cnt;
            in.read(reinterpret_cast<char*>(&child_cnt), sizeof(child_cnt));
            for (size_t i = 0; i < child_cnt; ++i) {
                size_t child;
                in.read(reinterpret_cast<char*>(&child), sizeof(child));
                node.children.push_back(child);
            }

            in.read(reinterpret_cast<char*>(&node.next_leaf), sizeof(node.next_leaf));
            return node;
        }
    };

    String index_file_;
    size_t root_offset_;
    DataType key_type_;

    Node read_node(size_t offset) const {
        std::ifstream in(index_file_.c_str(), std::ios::binary);
        if (!in) throw std::runtime_error(("Cannot open index file: " + index_file_).c_str());
        in.seekg(offset);
        return Node::deserialize(in);
    }

    size_t write_node(const Node& node) {
        std::ofstream out(index_file_.c_str(), std::ios::binary | std::ios::app);
        if (!out) throw std::runtime_error(("Cannot write index file: " + index_file_).c_str());
        size_t offset = out.tellp();
        node.serialize(out);
        out.flush();
        return offset;
    }

    void update_node(size_t offset, const Node& node) {
        std::ofstream out(index_file_.c_str(), std::ios::binary | std::ios::in | std::ios::out);
        if (!out) throw std::runtime_error(("Cannot update index file: " + index_file_).c_str());
        out.seekp(offset);
        node.serialize(out);
        out.flush();
    }

    void split_node(size_t parent_off, size_t child_off, int idx) {
        Node child = read_node(child_off);
        Node new_node(child.is_leaf);
        int mid = child.keys.size() / 2;

        Value up_key = child.keys[mid];

        if (child.is_leaf) {
            for (size_t i = mid; i < child.keys.size(); ++i) new_node.keys.push_back(child.keys[i]);
            for (size_t i = mid; i < child.children.size(); ++i) new_node.children.push_back(child.children[i]);
            new_node.next_leaf = child.next_leaf;
            child.next_leaf = write_node(new_node);
        } else {
            for (size_t i = mid + 1; i < child.keys.size(); ++i) new_node.keys.push_back(child.keys[i]);
            for (size_t i = mid + 1; i < child.children.size(); ++i) new_node.children.push_back(child.children[i]);
        }

        child.keys.resize(mid);
        if (!child.is_leaf) child.children.resize(mid + 1);
        else child.children.resize(mid);

        update_node(child_off, child);
        size_t new_node_off = write_node(new_node);

        Node parent = read_node(parent_off);
        parent.keys.insert(parent.keys.begin() + idx, up_key);
        parent.children.insert(parent.children.begin() + idx + 1, new_node_off);
        update_node(parent_off, parent);
    }

    void insert_non_full(size_t node_off, const Value& key, size_t row_num) {
        Node node = read_node(node_off);

        if (node.is_leaf) {
            if (node.keys.empty()) {
                node.keys.push_back(key);
                node.children.push_back(row_num);
            } else {
                int i = static_cast<int>(node.keys.size()) - 1;
                while (i >= 0 && key < node.keys[i]) i--;
                
                if (i >= 0 && node.keys[i] == key) 
                    throw std::runtime_error("Duplicate primary key");
                
                node.keys.insert(node.keys.begin() + i + 1, key);
                node.children.insert(node.children.begin() + i + 1, row_num);
            }
            update_node(node_off, node);
        } else {
            int i = static_cast<int>(node.keys.size()) - 1;
            while (i >= 0 && key < node.keys[i]) i--;
            i++;
            size_t child_off = node.children[i];
            Node child = read_node(child_off);

            if (child.keys.size() == BPLUS_ORDER - 1) {
                split_node(node_off, child_off, i);
                node = read_node(node_off);
                if (key > node.keys[i]) i++;
                child_off = node.children[i];
            }

            insert_non_full(child_off, key, row_num);
        }
    }

    size_t find_leaf(size_t node_off, const Value& key) const {
        Node node = read_node(node_off);
        while (!node.is_leaf) {
            int i = 0;
            int key_count = static_cast<int>(node.keys.size());
            while (i < key_count && key >= node.keys[i]) i++;
            node_off = node.children[i];
            node = read_node(node_off);
        }
        return node_off;
    }

public:
    BPlusTree(const String& file, DataType type) : index_file_(file + ".idx"), key_type_(type) {
        std::remove(index_file_.c_str());

        std::ofstream out(index_file_.c_str(), std::ios::binary);
        if (!out) throw std::runtime_error(("Cannot create index file: " + index_file_).c_str());
        size_t placeholder = 0;
        out.write(reinterpret_cast<const char*>(&placeholder), sizeof(placeholder));
        out.flush();
        out.close();

        Node root(true);
        root_offset_ = write_node(root);

        std::ofstream out2(index_file_.c_str(), std::ios::binary | std::ios::in | std::ios::out);
        out2.seekp(0);
        out2.write(reinterpret_cast<const char*>(&root_offset_), sizeof(root_offset_));
        out2.flush();
        out2.close();
    }

    void insert(const Value& key, size_t row_num) {
        if (key.get_type() != key_type_) 
            throw std::runtime_error("Key type mismatch");

        Node root = read_node(root_offset_);
        if (root.keys.size() == BPLUS_ORDER - 1) {
            Node new_root(false);
            size_t new_root_off = write_node(new_root);
            new_root.children.push_back(root_offset_);
            split_node(new_root_off, root_offset_, 0);
            root_offset_ = new_root_off;
            
            std::ofstream out(index_file_.c_str(), std::ios::binary | std::ios::in | std::ios::out);
            out.seekp(0);
            out.write(reinterpret_cast<const char*>(&root_offset_), sizeof(root_offset_));
            out.flush();
            out.close();
        }

        insert_non_full(root_offset_, key, row_num);
    }

    size_t find(const Value& key) const {
        if (key.get_type() != key_type_) 
            throw std::runtime_error("Key type mismatch");

        size_t leaf_off = find_leaf(root_offset_, key);
        Node leaf = read_node(leaf_off);

        int i = 0;
        while (i < static_cast<int>(leaf.keys.size()) && leaf.keys[i] < key) i++;
        if (i < static_cast<int>(leaf.keys.size()) && leaf.keys[i] == key) {
            return leaf.children[i];
        }
        return static_cast<size_t>(-1);
    }

    Vector<size_t> find_range(const Value& key, const String& op) const {
        Vector<size_t> res;
        if (key.get_type() != key_type_)
            throw std::runtime_error("Key type mismatch");

        size_t cur_leaf = find_leaf(root_offset_, key);
        while (cur_leaf != 0)
        {
            Node leaf = read_node(cur_leaf);
            for (size_t i = 0; i < leaf.keys.size(); i++)
            {
                bool match = false;
                if (op == "=")
                    match = (leaf.keys[i] == key);
                else if (op == "<")
                    match = (leaf.keys[i] < key);
                else if (op == ">")
                    match = (leaf.keys[i] > key);

                if (match)
                    res.push_back(leaf.children[i]);
            }
            cur_leaf = leaf.next_leaf;
        }
        return res;
    }

    void remove(const Value& key) {
        (void)key;
        throw std::runtime_error("Index delete not implemented");
    }
};

class Index {
private:
    BPlusTree tree_;
    String col_name_;
    size_t col_idx_;

public:
    Index(const String& table_name, const String& col_name, DataType type, size_t col_idx)
        : tree_(table_name, type), col_name_(col_name), col_idx_(col_idx) {}

    const String& get_column_name() const { return col_name_; }
    size_t get_column_index() const { return col_idx_; }

    void insert(const Value& key, size_t row_num) { tree_.insert(key, row_num); }
    size_t find(const Value& key) const { return tree_.find(key); }
    Vector<size_t> find_range(const Value& key, const String& op) const { 
        return tree_.find_range(key, op); 
    }
    void remove(const Value& key) { tree_.remove(key); }
};

#endif // INDEX_HPP