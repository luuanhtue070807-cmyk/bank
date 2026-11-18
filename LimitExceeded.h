#pragma once
#include <string>

#include "BankException.h"

// Ném khi vượt quá một hạn mức, ví dụ hạn mức tín dụng
class LimitExceeded : public BankException {
public:
    using BankException::BankException;
};