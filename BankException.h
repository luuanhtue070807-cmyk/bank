#pragma once
#include <exception>
#include <string>
#include <utility>

// Lớp cơ sở cho mọi lỗi nghiệp vụ của ngân hàng.
// Kế thừa std::exception để có thể bắt bằng catch (const std::exception&).
class BankException : public std::exception {
public:
    explicit BankException(std::string message)
        : message_(std::move(message)) {}

    ~BankException() override = default;

    // Tương đương what() trong UML. Trả về const char* theo chuẩn std::exception.
    const char* what() const noexcept override {
        return message_.c_str();
    }

    // Trả về nội dung lỗi dưới dạng std::string, tiện cho việc nối chuỗi
    std::string getMessage() const {
        return message_;
    }

protected:
    std::string message_;   // #message trong UML
};