#pragma once
#include <string>

#include "BankException.h"

// Ném khi thao tác trên tài khoản đang bị khóa
class AccountFrozen : public BankException {
public:
    using BankException::BankException;
};