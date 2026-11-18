#pragma once
#include <string>
#include <utility>

#include "Account.h"
#include "Transaction.h"

// =====================================================================
// DepositTransaction: nạp tiền vào một tài khoản.
// Hoàn tác bằng cách rút đúng số tiền đã nạp.
// =====================================================================
class DepositTransaction : public Transaction {
public:
    DepositTransaction(std::string transactionId, double amount, std::string date,
                       Account& target)
        : Transaction(std::move(transactionId), amount, std::move(date)),
          target_(target) {}

    const Account& getTarget() const noexcept { return target_; }

protected:
    void doExecute() override {
        target_.deposit(getAmount());         // ném AccountFrozen nếu bị khóa
    }

    void doRollback() override {
        // Nếu số tiền đã bị tiêu, withdraw sẽ ném InsufficientFunds
        // và trạng thái giao dịch giữ nguyên là Completed.
        target_.withdraw(getAmount());
    }

private:
    Account& target_;                         // không sở hữu, Bank giữ tài khoản
};