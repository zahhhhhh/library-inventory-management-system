#include "server/LimsServer.hpp"

#include "protocol/Protocol.hpp"
#include "server/ClientHandler.hpp"

#include <iostream>

bool LimsServer::loadInventory(std::string& error)
{
    return store_.load(inventory_, LimsStore::kDefaultFilename, error);
}

bool LimsServer::seedDemoCatalogueIfEmpty(std::string& error)
{
    if (!inventory_.getBooks().empty()) {
        return true;
    }

    Supplier supplier;
    supplier.id = 1;
    supplier.name = "City Books";
    supplier.contact = "orders@citybooks.example";
    if (!inventory_.addSupplier(supplier, error)) {
        return false;
    }

    Book pride;
    pride.id = 1;
    pride.isbn = "9780141439518";
    pride.title = "Pride and Prejudice";
    pride.author = "Jane Austen";
    pride.year = 1813;
    pride.totalCopies = 3;
    pride.availableCopies = 3;
    pride.supplierId = 1;
    if (!inventory_.addBook(pride, error)) {
        return false;
    }

    Book dune;
    dune.id = 2;
    dune.isbn = "9780441172719";
    dune.title = "Dune";
    dune.author = "Frank Herbert";
    dune.year = 1965;
    dune.totalCopies = 2;
    dune.availableCopies = 2;
    dune.supplierId = 1;
    return inventory_.addBook(dune, error);
}

bool LimsServer::run(std::string& error)
{
    if (!loadInventory(error)) {
        return false;
    }
    if (!seedDemoCatalogueIfEmpty(error)) {
        return false;
    }

    if (!listener_.listenOn(kDefaultHost, kDefaultPort, error)) {
        return false;
    }

    std::cout << "LIMS Server\n";
    std::cout << "Listening on " << kDefaultHost << ':' << kDefaultPort << std::endl;

    ClientHandler handler(inventory_, store_);
    while (true) {
        Socket client;
        if (!listener_.acceptClient(client, error)) {
            return false;
        }

        std::cout << "Client connected." << std::endl;
        handler.handle(std::move(client));
        std::cout << "Client disconnected." << std::endl;
    }
}
