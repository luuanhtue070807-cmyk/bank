#pragma once
#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include <utility>

#include "EventObserver.h"

// =====================================================================
// SMSNotifier: người quan sát gửi thông báo qua tin nhắn SMS.
// =====================================================================
class SMSNotifier : public Observer {
public:
    explicit SMSNotifier(std::string phone, std::ostream& out = std::cout)
        : phone_(std::move(phone)), out_(out) {}

    void update(const AccountEvent& event) override {
        ++sentCount_;
        out_ << "[SMS -> " << phone_ << "] "
             << toString(event.type) << " TK " << event.accountId;
        if (event.amount > 0)
            out_ << " " << event.amount << " VND";
        out_ << "\n";
    }

    const std::string& getPhone() const noexcept { return phone_; }
    std::size_t getSentCount() const noexcept { return sentCount_; }

private:
    std::string phone_;
    std::ostream& out_;          // không sở hữu
    std::size_t sentCount_ = 0;
};