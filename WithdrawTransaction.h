#pragma once
#include <string>
#include <utility>

#include "Account.h"
#include "Transaction.h"

// =====================================================================
// WithdrawTransaction: rút tiền từ một tài khoản.
// Hoàn tác bằng cách nạp lại đúng số tiền đã rút.
// =====================================================================
class WithdrawTransaction : public Transaction {
public:
    WithdrawTransaction(std::string transactionId, double amount, std::string date,
                        Account& source)
        : Transaction(std::move(transactionId), amount, std::move(date)),
          source_(source) {}

    const Account& getSource() const noexcept { return source_; }

protected:
    void doExecute() override {
        source_.withdraw(getAmount());        // ném InsufficientFunds hoặc AccountFrozen
    }

    void doRollback() override {
        source_.deposit(getAmount());         // ném AccountFrozen nếu tài khoản đang bị khóa
    }

private:
    Account& source_;
};