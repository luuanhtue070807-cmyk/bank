#pragma once
#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "Account.h"
#include "BankException.h"

// =====================================================================
// Customer: thông tin khách hàng và danh sách tài khoản.
// Quan hệ aggregation với Account: chỉ giữ con trỏ, không sở hữu.
// Bank là nơi duy nhất sở hữu và giải phóng Account.
// =====================================================================
class Customer {
public:
    Customer(std::string customerId, std::string name, std::string phone)
        : customerId_(std::move(customerId)),
          name_(std::move(name)),
          phone_(std::move(phone))
    {
        if (customerId_.empty()) throw BankException("Mã khách hàng không được rỗng");
        if (name_.empty())       throw BankException("Tên khách hàng không được rỗng");
    }

    // Khách hàng không được sao chép: hai bản sao sẽ cùng trỏ đến một tập tài khoản
    Customer(const Customer&) = delete;
    Customer& operator=(const Customer&) = delete;

    // ---------- Truy vấn ----------
    const std::string& getCustomerId() const noexcept { return customerId_; }
    const std::string& getName() const noexcept { return name_; }
    const std::string& getPhone() const noexcept { return phone_; }
    std::size_t getAccountCount() const noexcept { return accounts_.size(); }

    bool hasAccount(const std::string& accountId) const {
        return std::any_of(accounts_.begin(), accounts_.end(),
            [&accountId](const Account* a) { return a->getAccountId() == accountId; });
    }

    // Trả về danh sách tài khoản chỉ đọc, đúng với UML: viewAccounts(): List<Account>.
    // Việc in ra màn hình để Bank hoặc main() thực hiện.
    std::vector<const Account*> viewAccounts() const {
        std::vector<const Account*> result;
        result.reserve(accounts_.size());
        for (const Account* a : accounts_)
            result.push_back(a);
        return result;
    }

    // ---------- Quản lý tài khoản (do Bank gọi) ----------
    void addAccount(Account* account) {
        if (account == nullptr)
            throw BankException("Tài khoản không hợp lệ");
        if (hasAccount(account->getAccountId()))
            throw BankException("Tài khoản đã có trong danh sách của khách hàng: "
                                + account->getAccountId());
        accounts_.push_back(account);
    }

    // Gỡ con trỏ khỏi danh sách. Không giải phóng tài khoản.
    void removeAccount(const std::string& accountId) {
        accounts_.erase(
            std::remove_if(accounts_.begin(), accounts_.end(),
                [&accountId](const Account* a) { return a->getAccountId() == accountId; }),
            accounts_.end());
    }

private:
    std::string customerId_;   // mã khách hàng, duy nhất trong Bank
    std::string name_;
    std::string phone_;
    std::vector<Account*> accounts_;   // không sở hữu
};