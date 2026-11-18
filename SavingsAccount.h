// SavingsAccount.h
#pragma once
#include <memory>
#include <string>
#include <utility>

#include "Account.h"
#include "InterestStrategy.h"

class SavingsAccount : public Account {
public:
    static constexpr double kInterestRate = 0.054;   // 5,4%/năm

    SavingsAccount(std::string accountId, double initialBalance)
        : Account(std::move(accountId), initialBalance,
                  std::make_unique<SavingsInterest>(), kInterestRate) {}

    AccountType getKind() const noexcept override { return AccountType::Savings; }
};