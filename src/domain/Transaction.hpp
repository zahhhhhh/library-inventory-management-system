#ifndef LIMS_DOMAIN_TRANSACTION_HPP
#define LIMS_DOMAIN_TRANSACTION_HPP

#include "TransactionType.hpp"

#include <string>

struct Transaction {
    int id = 0;
    int bookId = 0;
    TransactionType type = TransactionType::ACQUIRE;
    int quantity = 0;
    std::string timestamp;
};

#endif
