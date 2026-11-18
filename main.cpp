#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Bank.h"
#include "BankException.h"
#include "Card.h"
#include "EmailNotifier.h"
#include "SMSNotifier.h"

namespace {

// Ném khi người dùng kết thúc đầu vào (Ctrl+Z hoặc Ctrl+D).
// Không kế thừa BankException nên không bị bắt nhầm như lỗi nghiệp vụ.
struct InputClosed {};

// Một đăng ký thông báo: observer nào đang gắn vào tài khoản nào.
struct Subscription {
    std::string accountId;
    Observer*   observer;      // không sở hữu, notifier được giữ trong MenuState
    std::string label;         // mô tả để hiển thị, ví dụ "Email a@b.com"
};

// Trạng thái phụ của menu. Khai báo trước Bank trong main() để bị hủy sau Bank.
struct MenuState {
    std::vector<std::unique_ptr<Observer>> notifiers;   // sở hữu các observer
    std::vector<Subscription>              subscriptions;
};

// ============================================================
// Nhập liệu
// ============================================================

std::string trim(const std::string& s) {
    const auto begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return "";
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(begin, end - begin + 1);
}

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) throw InputClosed{};
    return trim(line);
}

int readInt(const std::string& prompt, int minValue, int maxValue) {
    while (true) {
        const std::string s = readLine(prompt);
        try {
            std::size_t pos = 0;
            const int v = std::stoi(s, &pos);
            if (pos == s.size() && v >= minValue && v <= maxValue)
                return v;
        } catch (const std::exception&) {
            // không phải số, sẽ hỏi lại bên dưới
        }
        std::cout << "Vui lòng nhập số từ " << minValue << " đến " << maxValue << ".\n";
    }
}

double readNumber(const std::string& prompt) {
    while (true) {
        const std::string s = readLine(prompt);
        try {
            std::size_t pos = 0;
            const double v = std::stod(s, &pos);
            if (pos == s.size() && std::isfinite(v))
                return v;
        } catch (const std::exception&) {
            // không phải số, sẽ hỏi lại bên dưới
        }
        std::cout << "Số không hợp lệ, vui lòng nhập lại (ví dụ: 1000 hoặc 2500.5).\n";
    }
}

// ============================================================
// Giao diện
// ============================================================

void printMenu() {
    std::cout << "\n==================== HỆ THỐNG NGÂN HÀNG ====================\n"
                 "  1. Tạo khách hàng\n"
                 "  2. Xem khách hàng và tài khoản\n"
                 "  3. Mở tài khoản\n"
                 "  4. Đóng tài khoản\n"
                 "  5. Khóa / mở khóa tài khoản\n"
                 "  6. Nạp tiền\n"
                 "  7. Rút tiền\n"
                 "  8. Chuyển khoản\n"
                 "  9. Hoàn tác giao dịch\n"
                 " 10. Xem sao kê tài khoản\n"
                 " 11. Đăng ký thông báo cho tài khoản\n"
                 " 12. Hủy thông báo của tài khoản\n"
                 " 13. Cấp thẻ\n"
                 " 14. Nạp dữ liệu mẫu (để thử nhanh)\n"
                 "  0. Thoát\n"
                 "=============================================================\n";
}

AccountType askAccountType() {
    std::cout << "Loại tài khoản:\n"
                 "  1. Tiết kiệm (Savings)\n"
                 "  2. Thanh toán (Checking)\n"
                 "  3. Tín dụng (Credit)\n";
    switch (readInt("Chọn (1-3): ", 1, 3)) {
        case 1:  return AccountType::Savings;
        case 2:  return AccountType::Checking;
        default: return AccountType::Credit;
    }



    
}

// Xóa mọi đăng ký thông báo gắn với một tài khoản đã đóng
void removeSubscriptions(MenuState& state, const std::string& accountId) {
    auto& subs = state.subscriptions;
    subs.erase(std::remove_if(subs.begin(), subs.end(),
        [&accountId](const Subscription& s) { return s.accountId == accountId; }),
        subs.end());
}

// ============================================================
// Các chức năng
// ============================================================

void createCustomerAction(Bank& bank) {
    const std::string id    = readLine("Mã khách hàng: ");
    const std::string name  = readLine("Họ tên: ");
    const std::string phone = readLine("Số điện thoại: ");
    bank.createCustomer(id, name, phone);
    std::cout << "[OK] Đã tạo khách hàng " << id << ".\n";
}

void viewCustomerAction(const Bank& bank) {
    bank.printCustomerAccounts(readLine("Mã khách hàng: "));
}

void openAccountAction(Bank& bank) {
    const std::string customerId = readLine("Mã khách hàng: ");
    const AccountType kind = askAccountType();
    const double initial = readNumber("Số dư ban đầu: ");

    double creditLimit = 0.0;
    if (kind == AccountType::Credit)
        creditLimit = readNumber("Hạn mức tín dụng: ");

    const Account& acc = bank.openAccount(toString(kind), customerId, initial, creditLimit);
    std::cout << "[OK] Đã mở tài khoản " << acc.getAccountId()
              << " (" << acc.getType() << ")\n";
}

void closeAccountAction(Bank& bank, MenuState& state) {
    const std::string id = readLine("Mã tài khoản cần đóng: ");
    bank.closeAccount(id);          // ném lỗi nếu còn tiền hoặc đã có giao dịch
    removeSubscriptions(state, id); // tài khoản không còn, bỏ luôn đăng ký thông báo
    std::cout << "[OK] Đã đóng tài khoản " << id << ".\n";
}

void freezeAction(Bank& bank) {
    const std::string id = readLine("Mã tài khoản: ");
    std::cout << "  1. Khóa tài khoản\n  2. Mở khóa tài khoản\n";
    const bool freeze = readInt("Chọn (1-2): ", 1, 2) == 1;
    bank.freezeAccount(id, freeze);
    std::cout << "[OK] " << (freeze ? "Đã khóa " : "Đã mở khóa ") << id << ".\n";
}

void depositAction(Bank& bank) {
    const std::string id = readLine("Mã tài khoản: ");
    const double amount  = readNumber("Số tiền nạp: ");
    const std::string txId = bank.deposit(id, amount);
    std::cout << "[OK] Nạp thành công. Mã giao dịch: " << txId << "\n";
}

void withdrawAction(Bank& bank) {
    const std::string id = readLine("Mã tài khoản: ");
    const double amount  = readNumber("Số tiền rút: ");
    const std::string txId = bank.withdraw(id, amount);
    std::cout << "[OK] Rút thành công. Mã giao dịch: " << txId << "\n";
}

void transferAction(Bank& bank) {
    const std::string fromId = readLine("Mã tài khoản nguồn: ");
    const std::string toId   = readLine("Mã tài khoản nhận: ");
    const double amount      = readNumber("Số tiền chuyển: ");
    const std::string txId = bank.transferMoney(fromId, toId, amount);
    std::cout << "[OK] Chuyển khoản thành công. Mã giao dịch: " << txId << "\n";
}

void rollbackAction(Bank& bank) {
    const std::string txId = readLine("Mã giao dịch cần hoàn tác: ");
    bank.rollbackTransaction(txId);
    std::cout << "[OK] Đã hoàn tác giao dịch " << txId << ".\n";
}

void statementAction(const Bank& bank) {
    bank.printStatement(readLine("Mã tài khoản: "));
}

void subscribeAction(Bank& bank, MenuState& state) {
    const std::string id = readLine("Mã tài khoản: ");
    if (bank.findAccount(id) == nullptr)
        throw BankException("Không tìm thấy tài khoản: " + id);

    std::cout << "Kênh thông báo:\n  1. Email\n  2. SMS\n";
    const int channel = readInt("Chọn (1-2): ", 1, 2);

    std::unique_ptr<Observer> notifier;
    std::string label;
    if (channel == 1) {
        const std::string email = readLine("Địa chỉ email: ");
        if (email.empty()) throw BankException("Email không được rỗng");
        notifier = std::make_unique<EmailNotifier>(email);
        label = "Email " + email;
    } else {
        const std::string phone = readLine("Số điện thoại: ");
        if (phone.empty()) throw BankException("Số điện thoại không được rỗng");
        notifier = std::make_unique<SMSNotifier>(phone);
        label = "SMS " + phone;
    }

    Observer* raw = notifier.get();
    bank.attachObserver(id, raw);                 // nếu lỗi, notifier tự được giải phóng
    state.notifiers.push_back(std::move(notifier));
    state.subscriptions.push_back(Subscription{id, raw, label});
    std::cout << "[OK] Đã đăng ký " << label << " cho tài khoản " << id << ".\n";
}

void unsubscribeAction(Bank& bank, MenuState& state) {
    auto& subs = state.subscriptions;
    if (subs.empty()) {
        std::cout << "Chưa có thông báo nào được đăng ký.\n";
        return;
    }

    for (std::size_t i = 0; i < subs.size(); ++i)
        std::cout << "  " << (i + 1) << ". TK " << subs[i].accountId
                  << " -> " << subs[i].label << "\n";

    const int idx = readInt("Chọn số cần hủy: ", 1, static_cast<int>(subs.size()));
    const Subscription sub = subs[static_cast<std::size_t>(idx - 1)];
    bank.detachObserver(sub.accountId, sub.observer);
    subs.erase(subs.begin() + (idx - 1));
    std::cout << "[OK] Đã hủy " << sub.label << " khỏi tài khoản " << sub.accountId << ".\n";
}

void issueCardAction(Bank& bank) {
    const std::string id = readLine("Mã tài khoản: ");
    std::cout << "Loại thẻ:\n  1. Ghi nợ\n  2. Tín dụng\n";
    const CardType type = readInt("Chọn (1-2): ", 1, 2) == 1
                              ? CardType::Debit
                              : CardType::Credit;
    const Card& card = bank.issueCard(id, type);
    std::cout << "[OK] Đã cấp thẻ " << card.getMaskedNumber()
              << " (" << toString(card.getType()) << ")\n";
}

void seedDemoAction(Bank& bank) {
    bank.createCustomer("KH01", "Nguyễn Văn A", "0900000001");
    bank.createCustomer("KH02", "Trần Thị B", "0900000002");

    const Account& a = bank.openAccount("Savings",  "KH01", 10000);
    const Account& b = bank.openAccount("Checking", "KH01", 5000);
    const Account& c = bank.openAccount("Credit",   "KH02", 0, 5000);
    const Account& d = bank.openAccount("Savings",  "KH02", 2000);

    std::cout << "[OK] Đã tạo dữ liệu mẫu:\n"
              << "  Khách hàng KH01: " << a.getAccountId() << ", " << b.getAccountId() << "\n"
              << "  Khách hàng KH02: " << c.getAccountId() << ", " << d.getAccountId() << "\n";
}

// ============================================================
// Điều phối
// ============================================================

// Chạy một chức năng. Lỗi nghiệp vụ được in ra, không làm dừng chương trình.
void runAction(int choice, Bank& bank, MenuState& state) {
    try {
        switch (choice) {
            case 1:  createCustomerAction(bank);        break;
            case 2:  viewCustomerAction(bank);          break;
            case 3:  openAccountAction(bank);           break;
            case 4:  closeAccountAction(bank, state);   break;
            case 5:  freezeAction(bank);                break;
            case 6:  depositAction(bank);               break;
            case 7:  withdrawAction(bank);              break;
            case 8:  transferAction(bank);              break;
            case 9:  rollbackAction(bank);              break;
            case 10: statementAction(bank);             break;
            case 11: subscribeAction(bank, state);      break;
            case 12: unsubscribeAction(bank, state);    break;
            case 13: issueCardAction(bank);             break;
            case 14: seedDemoAction(bank);              break;
            default: break;
        }
    } catch (const BankException& e) {
        std::cout << "[LỖI] " << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cout << "[LỖI HỆ THỐNG] " << e.what() << "\n";
    }
}

} // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);   // hiển thị tiếng Việt đúng
    SetConsoleCP(65001);         // nhận tiếng Việt đúng
#endif

    MenuState state;   // khai báo trước bank để bị hủy sau bank
    Bank bank;

    try {
        while (true) {
            printMenu();
            const int choice = readInt("Chọn chức năng: ", 0, 14);
            if (choice == 0) break;

            runAction(choice, bank, state);
            readLine("\nNhấn Enter để quay lại menu...");
        }
    } catch (const InputClosed&) {
        std::cout << "\nKết thúc nhập liệu.\n";
    }

    std::cout << "Cảm ơn bạn đã sử dụng hệ thống.\n";
    return 0;
}