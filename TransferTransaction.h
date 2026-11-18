#pragma once
#include <string>
#include <utility>

#include "Account.h"
#include "AccountFrozen.h"
#include "BankException.h"
#include "Transaction.h"

// =====================================================================
// TransferTransaction: chuyển tiền từ tài khoản nguồn sang tài khoản nhận.
// Hoặc cả hai bước thành công, hoặc không bước nào làm thay đổi số dư.
// =====================================================================
class TransferTransaction : public Transaction {
public:
    TransferTransaction(std::string transactionId, double amount, std::string date,
                        Account& source, Account& target)
        : Transaction(std::move(transactionId), amount, std::move(date)),
          source_(source),
          target_(target)
    {
        if (&source_ == &target_)
            throw BankException("Không thể chuyển tiền cho chính tài khoản đó");
    }

    const Account& getSource() const noexcept { return source_; }
    const Account& getTarget() const noexcept { return target_; }

protected:
    void doExecute() override {
        requireUsable(source_, "nguồn");
        requireUsable(target_, "nhận");

        source_.withdraw(getAmount());        // ném InsufficientFunds nếu không đủ tiền
        try {
            target_.deposit(getAmount());
        } catch (...) {
            source_.deposit(getAmount());     // bù lại cho nguồn rồi ném tiếp lỗi
            throw;
        }
    }

    void doRollback() override {
        requireUsable(source_, "nguồn");
        requireUsable(target_, "nhận");

        target_.withdraw(getAmount());        // ném InsufficientFunds nếu bên nhận đã tiêu tiền
        try {
            source_.deposit(getAmount());
        } catch (...) {
            target_.deposit(getAmount());     // bù lại cho bên nhận rồi ném tiếp lỗi
            throw;
        }
    }

private:
    Account& source_;
    Account& target_;

    static void requireUsable(const Account& acc, const std::string& role) {
        if (acc.isFrozen())
            throw AccountFrozen("Tài khoản " + role + " " + acc.getAccountId() + " đang bị khóa");
    }
};