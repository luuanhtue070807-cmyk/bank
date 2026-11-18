#pragma once
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "AccountFrozen.h"
#include "BankException.h"
#include "EventObserver.h"
#include "InsufficientFunds.h"
#include "InterestStrategy.h"

// =====================================================================
// Loại tài khoản. Đặt ở đây để Account, CreateAccount và Bank
// cùng dùng mà không tạo vòng phụ thuộc giữa các file.
// =====================================================================
enum class AccountType {
    Savings,     // tiết kiệm
    Checking,    // thanh toán
    Credit       // tín dụng
};

inline std::string toString(AccountType type) {
    switch (type) {
        case AccountType::Savings:  return "Savings";
        case AccountType::Checking: return "Checking";
        case AccountType::Credit:   return "Credit";
    }
    return "Unknown";
}

// =====================================================================
// Account: lớp cha trừu tượng cho mọi loại tài khoản.
// Quy ước: số dư >= 0 là tiền thật. Riêng CreditAccount, số dư âm là số nợ.
// Mọi lỗi nghiệp vụ được báo bằng exception, không trả về bool.
// =====================================================================
class Account {
public:
    virtual ~Account() = default;
    Account(const Account&) = delete;
    Account& operator=(const Account&) = delete;

    // ---------- Truy vấn ----------
    const std::string& getAccountId() const noexcept { return accountId_; }
    double getBalance() const noexcept { return balance_; }
    bool isFrozen() const noexcept { return frozen_; }
    double getInterestRate() const noexcept { return interestRate_; }

    // Lớp con tự khai báo loại của mình. Bank dùng hàm này thay cho dynamic_cast.
    virtual AccountType getKind() const noexcept = 0;

    // Tên loại dạng chuỗi, suy ra từ getKind(). Lớp con không cần viết lại.
    std::string getType() const { return toString(getKind()); }

    // Lãi do chiến lược quyết định. Với CreditAccount, kết quả là lãi trên số nợ.
    double calculateInterest() const {
        return interest_->calculate(balance_, interestRate_);
    }

    // ---------- Thay đổi trạng thái ----------
    void setFrozen(bool frozen) {
        if (frozen_ == frozen) return;
        frozen_ = frozen;
        notifyObservers({accountId_, frozen ? EventType::Frozen : EventType::Unfrozen, 0.0});
    }

    // Ném AccountFrozen nếu bị khóa, BankException nếu số tiền không hợp lệ.
    void deposit(double amount) {
        ensureUsable();
        ensurePositive(amount);
        balance_ += amount;
        notifyObservers({accountId_, EventType::Deposit, amount});
    }

    // Ném AccountFrozen, BankException, InsufficientFunds hoặc LimitExceeded.
    // Nếu ném lỗi thì số dư không thay đổi.
    void withdraw(double amount) {
        ensureUsable();
        ensurePositive(amount);
        checkWithdrawal(amount);
        balance_ -= amount;
        notifyObservers({accountId_, EventType::Withdraw, amount});
    }

    // ---------- Observer (con trỏ không sở hữu) ----------
    void attach(Observer* observer) {
        if (observer == nullptr)
            throw BankException("Observer không hợp lệ");
        if (std::find(observers_.begin(), observers_.end(), observer) == observers_.end())
            observers_.push_back(observer);
    }

    void detach(Observer* observer) {
        observers_.erase(std::remove(observers_.begin(), observers_.end(), observer),
                         observers_.end());
    }

protected:
    // Chỉ lớp con được gọi constructor này
    Account(std::string id, double initialBalance,
            std::unique_ptr<InterestStrategy> strategy, double interestRate)
        : accountId_(std::move(id)),
          balance_(initialBalance),
          frozen_(false),
          interest_(std::move(strategy)),
          interestRate_(interestRate)
    {
        if (accountId_.empty())  throw BankException("Mã tài khoản không được rỗng");
        if (balance_ < 0)        throw BankException("Số dư ban đầu không được âm");
        if (!interest_)          throw BankException("Thiếu chiến lược tính lãi");
        if (interestRate_ < 0)   throw BankException("Tỷ lệ lãi không được âm");
    }

    // Điểm mở rộng: lớp con quyết định điều kiện rút tiền.
    // Hàm ném lỗi cụ thể thay vì trả về bool, nên lớp con chọn được loại lỗi.
    // Mặc định: không được rút quá số dư.
    virtual void checkWithdrawal(double amount) const {
        if (amount > balance_)
            throw InsufficientFunds("Không đủ số dư trong tài khoản " + accountId_);
    }

private:
    std::string accountId_;
    double balance_;
    bool frozen_;
    std::vector<Observer*> observers_;
    std::unique_ptr<InterestStrategy> interest_;
    double interestRate_;

    void ensureUsable() const {
        if (frozen_)
            throw AccountFrozen("Tài khoản " + accountId_ + " đang bị khóa");
    }

    static void ensurePositive(double amount) {
        if (amount <= 0)
            throw BankException("Số tiền phải lớn hơn 0");
    }

    void notifyObservers(const AccountEvent& event) {
        // Sao chép danh sách để an toàn nếu observer attach/detach trong lúc update
        auto snapshot = observers_;
        for (Observer* o : snapshot)
            o->update(event);
    }
};