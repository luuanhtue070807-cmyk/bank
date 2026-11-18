#pragma once
#include <string>

// =====================================================================
// PHẦN 1: Loại sự kiện
// Thay cho chuỗi "text" trong bản cũ, để tránh gõ sai và dễ kiểm tra.
// =====================================================================
enum class EventType {
    Deposit,     // nạp tiền
    Withdraw,    // rút tiền
    Frozen,      // tài khoản bị khóa
    Unfrozen     // tài khoản được mở khóa
};

// Chuyển EventType thành chuỗi để hiển thị hoặc ghi log
inline std::string toString(EventType type) {
    switch (type) {
        case EventType::Deposit:  return "NẠP TIỀN";
        case EventType::Withdraw: return "RÚT TIỀN";
        case EventType::Frozen:   return "KHÓA TÀI KHOẢN";
        case EventType::Unfrozen: return "MỞ KHÓA TÀI KHOẢN";
    }
    return "KHÔNG XÁC ĐỊNH";
}

// =====================================================================
// PHẦN 2: Sự kiện của tài khoản
// Đây là dữ liệu được gửi đến observer. Không có constructor riêng
// nên có thể khởi tạo bằng dấu ngoặc nhọn: {accountId, type, amount}.
// =====================================================================
struct AccountEvent {
    std::string accountId;   // tài khoản phát sinh sự kiện
    EventType   type;        // loại sự kiện
    double      amount;      // số tiền (0 nếu là khóa/mở khóa)
};

// =====================================================================
// PHẦN 3: Interface Observer
// Lớp nào muốn nhận thông báo từ Account thì kế thừa lớp này
// và viết lại hàm update(). Ví dụ: EmailNotifier, SMSNotifier.
// =====================================================================
class Observer {
public:
    virtual ~Observer() = default;

    // Được Account gọi mỗi khi có sự kiện
    virtual void update(const AccountEvent& event) = 0;
};