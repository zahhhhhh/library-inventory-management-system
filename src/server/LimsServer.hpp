#ifndef LIMS_SERVER_LIMS_SERVER_HPP
#define LIMS_SERVER_LIMS_SERVER_HPP

#include "inventory/InventoryService.hpp"
#include "net/Socket.hpp"
#include "persistence/LimsStore.hpp"

#include <string>

class LimsServer {
public:
    bool run(std::string& error);

private:
    bool loadInventory(std::string& error);
    bool seedDemoCatalogueIfEmpty(std::string& error);

    InventoryService inventory_;
    LimsStore store_;
    Socket listener_;
};

#endif
