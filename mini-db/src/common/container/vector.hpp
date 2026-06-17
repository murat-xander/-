#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <utility>

template <typename T>
class Vector {
private:
    T* data_;
    size_t size_;
    size_t capacity_;

    void reallocate(size_t new_cap) {
        if (new_cap <= capacity_) return;
        T* new_data = static_cast<T*>(operator new(new_cap * sizeof(T)));
        for (size_t i = 0; i < size_; ++i) {
            new (new_data + i) T(std::move(data_[i]));
            data_[i].~T();
        }
        operator delete(data_);
        data_ = new_data;
        capacity_ = new_cap;
    }

public:
    Vector() : data_(nullptr), size_(0), capacity_(0) {}

    ~Vector() { 
        clear(); 
        operator delete(data_); 
    }

    Vector(const Vector& other) : size_(other.size_), capacity_(other.capacity_) {
        data_ = static_cast<T*>(operator new(capacity_ * sizeof(T)));
        for (size_t i = 0; i < size_; ++i) 
            new (data_ + i) T(other.data_[i]);
    }

    Vector& operator=(const Vector& other) {
        if (this == &other) return *this;
        clear();
        if (capacity_ < other.size_) {
            operator delete(data_);
            capacity_ = other.capacity_;
            data_ = static_cast<T*>(operator new(capacity_ * sizeof(T)));
        }
        size_ = other.size_;
        for (size_t i = 0; i < size_; ++i) 
            new (data_ + i) T(other.data_[i]);
        return *this;
    }

    Vector(Vector&& other) noexcept 
        : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    Vector& operator=(Vector&& other) noexcept {
        if (this == &other) return *this;
        clear();
        operator delete(data_);
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
        return *this;
    }

    // 初始化列表构造函数（修复关键字列表初始化问题）
    Vector(std::initializer_list<T> init) 
        : size_(init.size()), capacity_(init.size()) {
        data_ = static_cast<T*>(operator new(capacity_ * sizeof(T)));
        size_t i = 0;
        for (const auto& elem : init) {
            new (data_ + i) T(elem);
            i++;
        }
    }

    void push_back(const T& val) {
        if (size_ == capacity_) 
            reallocate(capacity_ == 0 ? 1 : capacity_ * 2);
        new (data_ + size_) T(val);
        size_++;
    }

    void push_back(T&& val) {
        if (size_ == capacity_) 
            reallocate(capacity_ == 0 ? 1 : capacity_ * 2);
        new (data_ + size_) T(std::move(val));
        size_++;
    }

    void pop_back() {
        if (size_ == 0) throw std::out_of_range("Vector is empty");
        size_--;
        data_[size_].~T();
    }

    T& operator[](size_t idx) {
        if (idx >= size_) throw std::out_of_range("Index out of range");
        return data_[idx];
    }

    const T& operator[](size_t idx) const {
        if (idx >= size_) throw std::out_of_range("Index out of range");
        return data_[idx];
    }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    size_t capacity() const { return capacity_; }
    void reserve(size_t new_cap) { 
        if (new_cap > capacity_) reallocate(new_cap); 
    }
    
    void clear() { 
        for (size_t i = 0; i < size_; ++i) 
            data_[i].~T(); 
        size_ = 0; 
    }

    void erase(size_t idx) {
        if (idx >= size_) throw std::out_of_range("Index out of range");
        data_[idx].~T();
        for (size_t i = idx; i < size_ - 1; ++i) {
            new (data_ + i) T(std::move(data_[i + 1]));
            data_[i + 1].~T();
        }
        size_--;
    }

    // 新增：resize方法（修复B+树分裂错误）
    void resize(size_t new_size) {
        if (new_size > size_) {
            // 我们的代码中不会用到扩大的情况，直接抛出异常
            throw std::runtime_error("Vector resize to larger size not supported");
        }
        // 缩小：销毁多余元素
        for (size_t i = new_size; i < size_; ++i) {
            data_[i].~T();
        }
        size_ = new_size;
    }

    // 新增：insert方法（修复B+树插入错误）
    void insert(T* pos, const T& val) {
        size_t idx = pos - data_;
        if (idx > size_) throw std::out_of_range("Insert position out of range");
        
        if (size_ == capacity_) {
            // 扩容时计算新位置
            size_t new_cap = capacity_ == 0 ? 1 : capacity_ * 2;
            T* new_data = static_cast<T*>(operator new(new_cap * sizeof(T)));
            
            // 拷贝前半部分
            for (size_t i = 0; i < idx; ++i) {
                new (new_data + i) T(std::move(data_[i]));
                data_[i].~T();
            }
            
            // 插入新元素
            new (new_data + idx) T(val);
            
            // 拷贝后半部分
            for (size_t i = idx; i < size_; ++i) {
                new (new_data + i + 1) T(std::move(data_[i]));
                data_[i].~T();
            }
            
            operator delete(data_);
            data_ = new_data;
            capacity_ = new_cap;
        } else {
            // 不需要扩容，直接移动元素
            for (size_t i = size_; i > idx; --i) {
                new (data_ + i) T(std::move(data_[i - 1]));
                data_[i - 1].~T();
            }
            new (data_ + idx) T(val);
        }
        size_++;
    }

    T* begin() { return data_; }
    T* end() { return data_ + size_; }
    const T* begin() const { return data_; }
    const T* end() const { return data_ + size_; }
};

#endif // VECTOR_HPP