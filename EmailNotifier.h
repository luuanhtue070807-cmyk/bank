#pragma once
#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include <utility>

#include "EventObserver.h"

// =====================================================================
// EmailNotifier: người quan sát gửi thông báo qua email.
// Nhận std::ostream& làm đầu ra để kiểm thử được bằng std::ostringstream.
// =====================================================================
class EmailNotifier : public Observer {
public:
    explicit EmailNotifier(std::string email, std::ostream& out = std::cout)
        : email_(std::move(email)), out_(out) {}

    void update(const AccountEvent& event) override {
        ++sentCount_;
        out_ << "[EMAIL -> " << email_ << "] "
             << toString(event.type) << " | TK: " << event.accountId;
        if (event.amount > 0)
            out_ << " | Số tiền: " << event.amount;
        out_ << "\n";
    }

    const std::string& getEmail() const noexcept { return email_; }
    std::size_t getSentCount() const noexcept { return sentCount_; }

private:
    std::string email_;
    std::ostream& out_;          // không sở hữu
    std::size_t sentCount_ = 0;
};