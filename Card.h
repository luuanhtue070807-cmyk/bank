#pragma once
#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

#include "BankException.h"

// Loại thẻ. Thẻ ghi nợ gắn với tài khoản Savings/Checking, thẻ tín dụng gắn với Credit.
enum class CardType {
    Debit,
    Credit
};

inline std::string toString(CardType type) {
    switch (type) {
        case CardType::Debit:  return "Ghi nợ";
        case CardType::Credit: return "Tín dụng";
    }
    return "Không xác định";
}

// =====================================================================
// Card: thẻ thanh toán gắn với một tài khoản.
// Chỉ giữ mã tài khoản dạng chuỗi, không giữ con trỏ đến Account.
// =====================================================================
class Card {
public:
    Card(std::string cardNumber, CardType type, std::string accountId)
        : cardNumber_(std::move(cardNumber)),
          type_(type),
          accountId_(std::move(accountId))
    {
        if (cardNumber_.size() != 16 ||
            !std::all_of(cardNumber_.begin(), cardNumber_.end(),
                         [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }))
            throw BankException("Số thẻ phải gồm đúng 16 chữ số");
        if (accountId_.empty())
            throw BankException("Thẻ phải được gắn với một tài khoản");
    }

    const std::string& getCardNumber() const noexcept { return cardNumber_; }
    CardType getType() const noexcept { return type_; }
    const std::string& getAccountId() const noexcept { return accountId_; }

    // Chỉ hiển thị 4 số cuối
    std::string getMaskedNumber() const {
        return "**** **** **** " + cardNumber_.substr(cardNumber_.size() - 4);
    }

private:
    std::string cardNumber_;
    CardType type_;
    std::string accountId_;
};