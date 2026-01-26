#pragma once
#include <iostream>
#include <cstring>      // for memcpy
#include <new>          // for std::nothrow

class Buffer {
public:
    Buffer(size_t init_capacity = 5120)
        : capacity(init_capacity), size(0), data(nullptr)
    {
        data = new(std::nothrow) char[capacity];
        if (!data) {
            std::cerr << "Failed to allocate buffer of size " << capacity << "\n";
            capacity = 0;
        }
    }

    ~Buffer() {
        delete[] data;
    }


    bool write(const char* src, size_t len) {
        if (!ensureCapacity(size + len)) {
            std::cerr << "Failed to expand buffer during write\n";
            return false;
        }
        memcpy(data + size, src, len);
        size += len;
        return true;
    }

    char* getData() const {
        return data;
    }

    size_t getSize() const {
        return size;
    }

    void clear() {
        size = 0;
    }

    bool isValid() const {
        return data != nullptr;
    }

private:
    char* data;
    size_t size;
    size_t capacity;

    bool ensureCapacity(size_t required) {
        if (required <= capacity)
            return true;

        size_t new_capacity = capacity > 0 ? capacity : 1024;
        while (new_capacity < required)
            new_capacity *= 2;

        char* new_data = new(std::nothrow) char[new_capacity];
        if (!new_data) {
            std::cerr << "Memory allocation failed for capacity: " << new_capacity << "\n";
            return false;
        }

        if (data && size > 0) {
            memcpy(new_data, data, size);
        }

        delete[] data;
        data = new_data;
        capacity = new_capacity;
        return true;
    }

    // forbid copy
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
};
