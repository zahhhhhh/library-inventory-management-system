#ifndef LIMS_DOMAIN_TRANSACTION_TYPE_HPP
#define LIMS_DOMAIN_TRANSACTION_TYPE_HPP

enum class TransactionType {
    ACQUIRE,
    BORROW,
    RETURN,
    WRITEOFF
};

#endif
