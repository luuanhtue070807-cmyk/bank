#pragma once
#include <memory>
#include <string>
#include <utility>

#include "Account.h"
#include "BankException.h"
#include "InterestStrategy.h"
#include "LimitExceeded.h"

// =====================================================================
// CreditAccount: tài khoản tín dụng
// - Được rút tới (số dư + hạn mức). Số nợ chịu lãi 15%/năm.
// - Vượt hạn mức thì ném LimitExceeded, không phải InsufficientFunds.
// =====================================================================
class CreditAccount : public Account {
public:
    static constexpr double kInterestRate = 0.15;

    CreditAccount(std::string accountId, double initialBalance, double creditLimit)
        : Account(std::move(accountId), initialBalance,
                  std::make_unique<CreditInterest>(), kInterestRate),
          creditLimit_(requireNonNegative(creditLimit)) {}

    double getCreditLimit() const noexcept { return creditLimit_; }

    // Số tiền tối đa có thể rút ngay bây giờ
    double getAvailableCredit() const noexcept { return getBalance() + creditLimit_; }

    AccountType getKind() const noexcept override { return AccountType::Credit; }

protected:
    void checkWithdrawal(double amount) const override {
        if (amount > getAvailableCredit())
            throw LimitExceeded("Vượt quá hạn mức tín dụng của tài khoản " + getAccountId());
    }

private:
    double creditLimit_;

    static double requireNonNegative(double limit) {
        if (limit < 0)
            throw BankException("Hạn mức tín dụng không được âm");
        return limit;
    }
};