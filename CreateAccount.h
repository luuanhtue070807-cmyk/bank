#pragma once
#include <memory>
#include <string>

#include "Account.h"
#include "BankException.h"
#include "CheckingAccount.h"
#include "CreditAccount.h"
#include "SavingsAccount.h"

// =====================================================================
// CreateAccount: factory tạo tài khoản đúng lớp con.
// Không giữ trạng thái. Mã tài khoản do Bank sinh và truyền vào.
// =====================================================================
class CreateAccount {
public:
    // Chuyển chuỗi thành enum. Ném BankException nếu chuỗi không được hỗ trợ.
    static AccountType parseType(const std::string& type) {
        if (type == "Savings")  return AccountType::Savings;
        if (type == "Checking") return AccountType::Checking;
        if (type == "Credit")   return AccountType::Credit;
        throw BankException("Loại tài khoản không hợp lệ: " + type);
    }

    std::unique_ptr<Account> createAccount(const std::string& accountId,
                                           AccountType type,
                                           double initialBalance,
                                           double creditLimit = 0.0) const {
        // Hạn mức chỉ có ý nghĩa với tài khoản tín dụng
        if (type != AccountType::Credit && creditLimit != 0.0)
            throw BankException("Chỉ tài khoản tín dụng mới có hạn mức");

        switch (type) {
            case AccountType::Savings:
                return std::make_unique<SavingsAccount>(accountId, initialBalance);
            case AccountType::Checking:
                return std::make_unique<CheckingAccount>(accountId, initialBalance);
            case AccountType::Credit:
                return std::make_unique<CreditAccount>(accountId, initialBalance, creditLimit);
        }
        throw BankException("Loại tài khoản không hợp lệ");
    }
};