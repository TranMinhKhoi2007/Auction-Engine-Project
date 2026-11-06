#pragma once

#include <string>

namespace auction::domain {

// Best effort wipe on every exit, including exceptions. Does not own the buffer.
class SecretBuffer {
public:
    explicit SecretBuffer(std::string& value) : value_(value) {}
    ~SecretBuffer() {
        volatile char* bytes = value_.data();
        for (size_t i = 0; i < value_.size(); ++i) {
            bytes[i] = 0;
        }
        value_.clear();
    }
    SecretBuffer(const SecretBuffer&) = delete;
    SecretBuffer& operator=(const SecretBuffer&) = delete;
private:
    std::string& value_;
};

} // namespace auction::domain
