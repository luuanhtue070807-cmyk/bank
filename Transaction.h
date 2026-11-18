#pragma once
#include <string>
#include <utility>

#include "BankException.h"

// =====================================================================
// Trạng thái của một giao dịch
// =====================================================================
enum class TransactionStatus {
    Pending,      // chờ xử lý
    Completed,    // đã hoàn thành
    Failed,       // thất bại khi thực thi
    RolledBack    // đã hoàn tác
};

inline std::string toString(TransactionStatus status) {
    switch (status) {
        case TransactionStatus::Pending:    return "CHỜ XỬ LÝ";
        case TransactionStatus::Completed:  return "THÀNH CÔNG";
        case TransactionStatus::Failed:     return "THẤT BẠI";
        case TransactionStatus::RolledBack: return "ĐÃ HOÀN TÁC";
    }
    return "KHÔNG XÁC ĐỊNH";
}

// =====================================================================
// Transaction: lớp trừu tượng cho mọi giao dịch.
// Lớp này quản lý vòng đời trạng thái (Pending -> Completed/Failed -> RolledBack).
// Lớp con chỉ viết logic cụ thể trong doExecute() và doRollback().
// Mọi lỗi được báo bằng BankException (hoặc lớp con).
// =====================================================================
class Transaction {
public:
    Transaction(std::string transactionId, double amount, std::string date)
        : transactionId_(std::move(transactionId)),
          amount_(amount),
          date_(std::move(date)),
          status_(TransactionStatus::Pending)
    {
        if (transactionId_.empty()) throw BankException("Mã giao dịch không được rỗng");
        if (amount_ <= 0)           throw BankException("Số tiền giao dịch phải lớn hơn 0");
    }

    virtual ~Transaction() = default;
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    // ---------- Truy vấn ----------
    const std::string& getTransactionId() const noexcept { return transactionId_; }
    double getAmount() const noexcept { return amount_; }
    const std::string& getDate() const noexcept { return date_; }
    TransactionStatus getStatus() const noexcept { return status_; }

    // ---------- Thao tác ----------
    // Thực thi giao dịch. Nếu thất bại, trạng thái chuyển sang Failed và lỗi được ném tiếp.
    void execute() {
        if (status_ != TransactionStatus::Pending)
            throw BankException("Giao dịch " + transactionId_ + " đã được xử lý trước đó");
        try {
            doExecute();
            status_ = TransactionStatus::Completed;
        } catch (...) {
            status_ = TransactionStatus::Failed;
            throw;
        }
    }

    // Hoàn tác giao dịch đã hoàn thành. Nếu thất bại, lỗi được ném ra và trạng thái giữ nguyên.
    void rollback() {
        if (status_ != TransactionStatus::Completed)
            throw BankException("Chỉ có thể hoàn tác giao dịch đã hoàn thành: " + transactionId_);
        doRollback();
        status_ = TransactionStatus::RolledBack;
    }

protected:
    // Lớp con bắt buộc phải cài đặt. Không cần quản lý status_.
    virtual void doExecute() = 0;
    virtual void doRollback() = 0;

private:
    std::string transactionId_;
    double amount_;
    std::string date_;
    TransactionStatus status_;
};