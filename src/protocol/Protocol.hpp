#ifndef LIMS_PROTOCOL_PROTOCOL_HPP
#define LIMS_PROTOCOL_PROTOCOL_HPP

#include "domain/Book.hpp"
#include "domain/Supplier.hpp"
#include "domain/Transaction.hpp"
#include "inventory/InventoryService.hpp"

#include <string>
#include <vector>

inline constexpr const char* kDefaultHost = "127.0.0.1";
inline constexpr unsigned short kDefaultPort = 4545;

enum class SessionRole {
    Unauthenticated,
    User,
    Admin
};

enum class RequestType {
    Ping,
    Role,
    List,
    Search,
    Availability,
    Sort,
    Report,
    AddBook,
    EditBook,
    RemoveBook,
    ListSuppliers,
    AddSupplier,
    EditSupplier,
    RemoveSupplier,
    Stock,
    ListTransactions,
    Save,
    Quit,
    Unknown
};

struct ProtocolRequest {
    RequestType type = RequestType::Unknown;
    std::vector<std::string> fields;
};

ProtocolRequest parseRequest(const std::string& line);

std::string makeOk(const std::string& payload);
std::string makeErr(const std::string& message);
std::string encodeBooks(const std::vector<Book>& books);
std::string encodeSuppliers(const std::vector<Supplier>& suppliers);
std::string encodeTransactions(const std::vector<Transaction>& transactions);
std::string encodeAvailability(int bookId, int availableCopies);
std::string encodeReport(const InventoryReport& report);

#endif
