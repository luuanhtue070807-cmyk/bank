// CheckingAccount.h
#pragma once
#include <memory>
#include <string>
#include <utility>

#include "Account.h"
#include "InterestStrategy.h"

class CheckingAccount : public Account {
public:
    static constexpr double kInterestRate = 0.0;     // không có lãi

    CheckingAccount(std::string accountId, double initialBalance)
        : Account(std::move(accountId), initialBalance,
                  std::make_unique<CheckingInterest>(), kInterestRate) {}

    AccountType getKind() const noexcept override { return AccountType::Checking; }
};