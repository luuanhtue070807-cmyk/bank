#pragma once
#include <string>

#include "BankException.h"

// Ném khi số dư không đủ để rút hoặc chuyển tiền
class InsufficientFunds : public BankException {
public:
    using BankException::BankException;
};