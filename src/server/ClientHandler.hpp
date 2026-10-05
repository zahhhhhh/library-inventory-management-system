#ifndef LIMS_SERVER_CLIENT_HANDLER_HPP
#define LIMS_SERVER_CLIENT_HANDLER_HPP

#include "inventory/InventoryService.hpp"
#include "net/Socket.hpp"
#include "persistence/LimsStore.hpp"

class ClientHandler {
public:
    ClientHandler(InventoryService& inventory, LimsStore& store);

    void handle(Socket client);

private:
    InventoryService& inventory_;
    LimsStore& store_;
};

#endif
