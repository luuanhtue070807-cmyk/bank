#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "Account.h"
#include "BankException.h"
#include "Card.h"
#include "CreateAccount.h"
#include "CreditAccount.h"
#include "Customer.h"
#include "DepositTransaction.h"
#include "EventObserver.h"
#include "Transaction.h"
#include "TransferTransaction.h"
#include "WithdrawTransaction.h"

// =====================================================================
// Bank: lớp điều phối trung tâm.
// - Sở hữu khách hàng, tài khoản, thẻ và lịch sử giao dịch.
// - Mọi thao tác tiền đi qua Bank để được ghi vào lịch sử.
// - Lỗi nghiệp vụ được báo bằng BankException hoặc lớp con.
// =====================================================================
class Bank {
public:
    Bank() = default;
    Bank(const Bank&) = delete;
    Bank& operator=(const Bank&) = delete;

    // ================= Khách hàng =================
    Customer& createCustomer(const std::string& customerId,
                             const std::string& name,
                             const std::string& phone) {
        if (findCustomer(customerId) != nullptr)
            throw BankException("Mã khách hàng đã tồn tại: " + customerId);

        customers_.push_back(std::make_unique<Customer>(customerId, name, phone));
        return *customers_.back();
    }

    Customer* findCustomer(const std::string& customerId) const {
        for (const auto& c : customers_)
            if (c->getCustomerId() == customerId) return c.get();
        return nullptr;
    }

    void printCustomerAccounts(const std::string& customerId) const {
        const Customer* customer = findCustomer(customerId);
        if (customer == nullptr)
            throw BankException("Không tìm thấy khách hàng: " + customerId);

        std::cout << "Khách hàng: " << customer->getName()
                  << " (" << customer->getCustomerId() << ")\n";
        for (const Account* acc : customer->viewAccounts()) {
            std::cout << "  - " << acc->getAccountId()
                      << " | " << acc->getType()
                      << " | Số dư: " << acc->getBalance() << "\n";
        }
    }

    // ================= Tài khoản =================
    // Mở tài khoản. Bank sinh mã duy nhất và giữ quyền sở hữu.
    Account& openAccount(const std::string& type,
                         const std::string& customerId,
                         double initialBalance,
                         double creditLimit = 0.0) {
        Customer* customer = findCustomer(customerId);
        if (customer == nullptr)
            throw BankException("Không tìm thấy khách hàng: " + customerId);

        // Kiểm tra loại trước, để không sinh mã cho loại tài khoản sai
        const AccountType kind = CreateAccount::parseType(type);
        const std::string newId = makeAccountId(kind, customerId, accountSeq_ + 1);

        std::unique_ptr<Account> acc =
            factory_.createAccount(newId, kind, initialBalance, creditLimit);
        ++accountSeq_;

        accounts_.push_back(std::move(acc));
        Account& ref = *accounts_.back();
        customer->addAccount(&ref);      // Customer chỉ giữ con trỏ, không sở hữu
        return ref;
    }

    // Đóng tài khoản. Chỉ đóng khi số dư bằng 0 và chưa có giao dịch nào.
    void closeAccount(const std::string& accountId) {
        Account& target = requireAccount(accountId);
        if (target.getBalance() != 0.0)
            throw BankException("Chỉ có thể đóng tài khoản có số dư bằng 0");
        if (hasHistory(accountId))
            throw BankException("Tài khoản đã có giao dịch nên không thể đóng. "
                                "Hãy khóa tài khoản để giữ lại lịch sử.");

        for (auto& customer : customers_)
            customer->removeAccount(accountId);

        cards_.erase(std::remove_if(cards_.begin(), cards_.end(),
            [&accountId](const std::unique_ptr<Card>& c) {
                return c->getAccountId() == accountId;
            }), cards_.end());

        accounts_.erase(std::find_if(accounts_.begin(), accounts_.end(),
            [&accountId](const std::unique_ptr<Account>& a) {
                return a->getAccountId() == accountId;
            }));
    }

    void freezeAccount(const std::string& accountId, bool frozen) {
        requireAccount(accountId).setFrozen(frozen);
    }

    // Trả về nullptr nếu không có
    Account* findAccount(const std::string& accountId) const {
        for (const auto& acc : accounts_)
            if (acc->getAccountId() == accountId) return acc.get();
        return nullptr;
    }

    // ================= Người quan sát =================
    // Observer là con trỏ không sở hữu. Phải detach trước khi observer bị hủy.
    void attachObserver(const std::string& accountId, Observer* observer) {
        requireAccount(accountId).attach(observer);
    }

    void detachObserver(const std::string& accountId, Observer* observer) {
        requireAccount(accountId).detach(observer);
    }

    // ================= Thẻ =================
    // Thẻ tín dụng chỉ cấp cho tài khoản tín dụng, thẻ ghi nợ không cấp cho tài khoản tín dụng.
    Card& issueCard(const std::string& accountId, CardType type) {
        const Account& acc = requireAccount(accountId);
        const bool isCredit = acc.getKind() == AccountType::Credit;

        if (type == CardType::Credit && !isCredit)
            throw BankException("Chỉ tài khoản tín dụng mới được cấp thẻ tín dụng");
        if (type == CardType::Debit && isCredit)
            throw BankException("Tài khoản tín dụng không được cấp thẻ ghi nợ");

        ++cardSeq_;
        char buf[24];
        // Demo: số thẻ bắt đầu bằng 4 và 15 chữ số tăng dần.
        // Thẻ thật cần thuật toán sinh số riêng, ví dụ Luhn.
        std::snprintf(buf, sizeof buf, "4%015zu", cardSeq_);
        cards_.push_back(std::make_unique<Card>(buf, type, accountId));
        return *cards_.back();
    }

    // ================= Giao dịch =================
    // Các hàm dưới đây trả về mã giao dịch vừa ghi nhận.

    std::string deposit(const std::string& accountId, double amount) {
        Account& target = requireAccount(accountId);
        return record(std::make_unique<DepositTransaction>(
                          nextTransactionId(), amount, currentDate(), target),
                      "Nạp tiền", {accountId});
    }

    std::string withdraw(const std::string& accountId, double amount) {
        Account& source = requireAccount(accountId);
        return record(std::make_unique<WithdrawTransaction>(
                          nextTransactionId(), amount, currentDate(), source),
                      "Rút tiền", {accountId});
    }

    std::string transferMoney(const std::string& fromId, const std::string& toId, double amount) {
        Account& source = requireAccount(fromId);
        Account& target = requireAccount(toId);
        return record(std::make_unique<TransferTransaction>(
                          nextTransactionId(), amount, currentDate(), source, target),
                      "Chuyển khoản", {fromId, toId});
    }

    // Hoàn tác giao dịch đã hoàn thành. Nếu không thể, lỗi được ném ra và trạng thái giữ nguyên.
        void rollbackTransaction(const std::string& transactionId) {
        for (const auto& f : failures_)
            if (f.transactionId == transactionId)
                throw BankException("Chỉ có thể hoàn tác giao dịch đã hoàn thành: " + transactionId);
        requireEntry(transactionId).tx->rollback();
    }

    // ================= Hiển thị =================
    void printStatement(const std::string& accountId) const {
        const Account& acc = requireAccount(accountId);

        std::cout << "========== SAO KÊ TÀI KHOẢN ==========\n";
        std::cout << "Mã tài khoản : " << acc.getAccountId() << "\n";
        std::cout << "Loại         : " << acc.getType() << "\n";
        std::cout << "Số dư        : " << acc.getBalance() << "\n";
        std::cout << "Trạng thái   : " << (acc.isFrozen() ? "Bị khóa" : "Hoạt động") << "\n";

        if (acc.getKind() == AccountType::Credit) {
            // An toàn: getKind() == Credit chỉ xảy ra với CreditAccount
            const auto& credit = static_cast<const CreditAccount&>(acc);
            std::cout << "Hạn mức      : " << credit.getCreditLimit() << "\n";
            std::cout << "Còn được rút : " << credit.getAvailableCredit() << "\n";
        }
        // Tiết kiệm: lãi được hưởng. Tín dụng: lãi phải trả trên số nợ.
        std::cout << "Lãi ước tính : " << acc.calculateInterest() << "\n";

        std::cout << "--- Lịch sử giao dịch ---\n";
                struct Line {
            std::string txId, date, kind, status;
            double amount;
        };
        std::vector<Line> lines;
        for (const auto& entry : history_) {
            if (!involves(entry, accountId)) continue;
            lines.push_back({entry.tx->getTransactionId(), entry.tx->getDate(), entry.kind,
                             toString(entry.tx->getStatus()), entry.tx->getAmount()});
        }
        for (const auto& f : failures_) {
            if (!containsId(f.accountIds, accountId)) continue;
            lines.push_back({f.transactionId, f.date, f.kind,
                             toString(TransactionStatus::Failed), f.amount});
        }
        // Mã giao dịch có độ dài cố định (TX-000001...) nên sắp xếp chuỗi cũng là sắp theo thời gian
        std::sort(lines.begin(), lines.end(),
                  [](const Line& a, const Line& b) { return a.txId < b.txId; });
        if (lines.empty())
            std::cout << "(Chưa có giao dịch)\n";
        for (const Line& l : lines)
            std::cout << "[" << l.txId << "] " << l.date << " | " << l.kind << " | "
                      << l.amount << " | " << l.status << "\n";
        std::cout << "=======================================\n";
    }

private:
    // Một dòng lịch sử. Bank sở hữu giao dịch để có thể hoàn tác sau này.
    struct HistoryEntry {
        std::unique_ptr<Transaction> tx;
        std::string kind;                     // "Nạp tiền", "Rút tiền", "Chuyển khoản"
        std::vector<std::string> accountIds;  // tài khoản liên quan
    };
        // Giao dịch THẤT BẠI chỉ được lưu dưới dạng dữ liệu thuần (không giữ tham chiếu tới tài khoản),
    // nên không chặn việc đóng tài khoản. Vẫn hiển thị trong sao kê để kiểm toán.
    struct FailedAttempt {
        std::string transactionId;
        std::string date;
        std::string kind;
        double amount;
        std::vector<std::string> accountIds;
    };
    // Thứ tự khai báo quan trọng: khi Bank bị hủy, history_ bị hủy trước accounts_,
    // nên không còn giao dịch nào giữ tham chiếu đến tài khoản đã bị giải phóng.
    std::vector<std::unique_ptr<Customer>> customers_;
    std::vector<std::unique_ptr<Account>>  accounts_;
    std::vector<std::unique_ptr<Card>>     cards_;
    std::vector<HistoryEntry>              history_;
    std::vector<FailedAttempt>             failures_;
    CreateAccount factory_;
    std::size_t accountSeq_ = 0;
    std::size_t cardSeq_    = 0;
    std::size_t txSeq_      = 0;

    Account& requireAccount(const std::string& accountId) const {
        Account* acc = findAccount(accountId);
        if (acc == nullptr)
            throw BankException("Không tìm thấy tài khoản: " + accountId);
        return *acc;
    }

    HistoryEntry& requireEntry(const std::string& transactionId) {
        for (auto& entry : history_)
            if (entry.tx->getTransactionId() == transactionId) return entry;
        throw BankException("Không tìm thấy giao dịch: " + transactionId);
    }

    bool hasHistory(const std::string& accountId) const {
        return std::any_of(history_.begin(), history_.end(),
            [&accountId](const HistoryEntry& e) { return involves(e, accountId); });
    }

    static bool involves(const HistoryEntry& entry, const std::string& accountId) {
        return std::find(entry.accountIds.begin(), entry.accountIds.end(), accountId)
               != entry.accountIds.end();
    }

        static bool containsId(const std::vector<std::string>& ids, const std::string& id) {
        return std::find(ids.begin(), ids.end(), id) != ids.end();
    }

    // Thực thi trước, rồi mới ghi nhận. Thành công: vào history_ (để hoàn tác được).
    // Thất bại: vào failures_ dưới dạng dữ liệu thuần, rồi ném lỗi tiếp.
    std::string record(std::unique_ptr<Transaction> tx,
                       const std::string& kind,
                       std::vector<std::string> accountIds) {
        std::string id = tx->getTransactionId();
        try {
            tx->execute();
        } catch (const std::exception&) {
            failures_.push_back(FailedAttempt{id, tx->getDate(), kind, tx->getAmount(),
                                              std::move(accountIds)});
            throw;
        }
        history_.push_back(HistoryEntry{std::move(tx), kind, std::move(accountIds)});
        return id;
    }

    // Ví dụ: "TX-000001"
    std::string nextTransactionId() {
        ++txSeq_;
        char buf[24];
        std::snprintf(buf, sizeof buf, "TX-%06zu", txSeq_);
        return buf;
    }

    // Ví dụ: "Sav-KH01-1", "Che-KH01-2", "Cre-KH02-3"
    static std::string makeAccountId(AccountType type,
                                     const std::string& customerId,
                                     std::size_t seq) {
        return toString(type).substr(0, 3) + "-" + customerId + "-" + std::to_string(seq);
    }

    // Ví dụ: "2026-11-07 14:30:05"
    static std::string currentDate() {
        std::time_t now = std::time(nullptr);
        std::tm local{};
#ifdef _WIN32
        localtime_s(&local, &now);
#else
        localtime_r(&now, &local);
#endif
        char buf[32];
        std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &local);
        return buf;
    }
};