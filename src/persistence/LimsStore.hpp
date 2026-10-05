#ifndef LIMS_PERSISTENCE_LIMS_STORE_HPP
#define LIMS_PERSISTENCE_LIMS_STORE_HPP

#include "inventory/InventoryService.hpp"

#include <string>

class LimsStore {
public:
    static constexpr const char* kDefaultFilename = "data/library.lims";

    bool save(const InventoryService& inventory,
              const std::string& filename,
              std::string& error) const;

    bool load(InventoryService& inventory,
              const std::string& filename,
              std::string& error) const;
};

std::string transactionTypeToString(TransactionType type);
bool transactionTypeFromString(const std::string& text,
                               TransactionType& type,
                               std::string& error);

#endif
