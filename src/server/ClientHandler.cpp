#include "server/ClientHandler.hpp"

#include "protocol/Protocol.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace {

// Coursework demonstration password only — not production security.
constexpr const char* kAdminPassword = "admin123";

bool parseIntField(const std::string& text, int& value)
{
    if (text.empty()) {
        return false;
    }

    std::size_t consumed = 0;
    try {
        value = std::stoi(text, &consumed);
    } catch (const std::exception&) {
        return false;
    }

    return consumed == text.size();
}

bool containsReservedDelimiter(const std::string& text)
{
    return text.find('|') != std::string::npos || text.find('~') != std::string::npos;
}

std::string requireNoReservedText(const std::vector<std::string>& fields)
{
    for (const std::string& field : fields) {
        if (containsReservedDelimiter(field)) {
            return makeErr("Text fields cannot contain '|' or '~'.");
        }
    }
    return {};
}

std::string handleRole(const ProtocolRequest& request, SessionRole& role)
{
    if (request.fields.size() < 2) {
        return makeErr("Malformed ROLE request.");
    }

    if (request.fields[1] == "USER") {
        role = SessionRole::User;
        return makeOk("ROLE|USER");
    }

    if (request.fields[1] == "ADMIN") {
        if (request.fields.size() < 3 || request.fields[2] != kAdminPassword) {
            return makeErr("Invalid administrator credentials.");
        }
        role = SessionRole::Admin;
        return makeOk("ROLE|ADMIN");
    }

    return makeErr("Unknown role.");
}

bool hasInventoryAccess(SessionRole role)
{
    return role == SessionRole::User || role == SessionRole::Admin;
}

std::string requireAdmin(SessionRole role)
{
    if (!hasInventoryAccess(role)) {
        return makeErr("Role required.");
    }
    if (role != SessionRole::Admin) {
        return makeErr("Administrator access required.");
    }
    return {};
}

std::string handleSearch(const ProtocolRequest& request, const InventoryService& inventory)
{
    if (request.fields.size() < 3) {
        return makeErr("Malformed SEARCH request.");
    }

    const std::string& field = request.fields[1];
    const std::string& query = request.fields[2];
    std::vector<Book> matches;
    if (field == "ISBN") {
        matches = inventory.searchByIsbn(query);
    } else if (field == "TITLE") {
        matches = inventory.searchByTitle(query);
    } else if (field == "AUTHOR") {
        matches = inventory.searchByAuthor(query);
    } else {
        return makeErr("Unknown search field.");
    }

    return encodeBooks(matches);
}

std::string handleAvailability(const ProtocolRequest& request, const InventoryService& inventory)
{
    if (request.fields.size() < 2) {
        return makeErr("Malformed AVAILABILITY request.");
    }

    int bookId = 0;
    if (!parseIntField(request.fields[1], bookId)) {
        return makeErr("Malformed numeric field: bookId.");
    }

    int availableCopies = 0;
    std::string error;
    if (!inventory.getAvailability(bookId, availableCopies, error)) {
        return makeErr(error);
    }

    return encodeAvailability(bookId, availableCopies);
}

std::string handleSort(const ProtocolRequest& request, InventoryService& inventory)
{
    if (request.fields.size() != 2) {
        return makeErr("Malformed SORT request.");
    }

    CatalogueSort key = CatalogueSort::Title;
    const std::string& field = request.fields[1];
    if (field == "TITLE") {
        key = CatalogueSort::Title;
    } else if (field == "AUTHOR") {
        key = CatalogueSort::Author;
    } else if (field == "YEAR") {
        key = CatalogueSort::Year;
    } else if (field == "ISBN") {
        key = CatalogueSort::Isbn;
    } else {
        return makeErr("Unknown sort field.");
    }

    inventory.sortCatalogue(key);
    return encodeBooks(inventory.getBooks());
}

bool parseBookFields(const ProtocolRequest& request, const std::string& commandName, Book& book, std::string& response)
{
    if (request.fields.size() != 9) {
        response = makeErr("Malformed " + commandName + " request.");
        return false;
    }

    const std::string reserved = requireNoReservedText(
        {request.fields[2], request.fields[3], request.fields[4]});
    if (!reserved.empty()) {
        response = reserved;
        return false;
    }

    if (!parseIntField(request.fields[1], book.id)) {
        response = makeErr("Malformed numeric field: id.");
        return false;
    }
    if (!parseIntField(request.fields[5], book.year)) {
        response = makeErr("Malformed numeric field: year.");
        return false;
    }
    if (!parseIntField(request.fields[6], book.totalCopies)) {
        response = makeErr("Malformed numeric field: totalCopies.");
        return false;
    }
    if (!parseIntField(request.fields[7], book.availableCopies)) {
        response = makeErr("Malformed numeric field: availableCopies.");
        return false;
    }
    if (!parseIntField(request.fields[8], book.supplierId)) {
        response = makeErr("Malformed numeric field: supplierId.");
        return false;
    }

    book.isbn = request.fields[2];
    book.title = request.fields[3];
    book.author = request.fields[4];
    return true;
}

bool parseSupplierFields(const ProtocolRequest& request, const std::string& commandName, Supplier& supplier,
                         std::string& response)
{
    if (request.fields.size() != 4) {
        response = makeErr("Malformed " + commandName + " request.");
        return false;
    }

    const std::string reserved = requireNoReservedText({request.fields[2], request.fields[3]});
    if (!reserved.empty()) {
        response = reserved;
        return false;
    }

    if (!parseIntField(request.fields[1], supplier.id)) {
        response = makeErr("Malformed numeric field: id.");
        return false;
    }

    supplier.name = request.fields[2];
    supplier.contact = request.fields[3];
    return true;
}

std::string handleAddBook(const ProtocolRequest& request, InventoryService& inventory)
{
    Book book;
    std::string response;
    if (!parseBookFields(request, "ADD_BOOK", book, response)) {
        return response;
    }

    std::string error;
    if (!inventory.addBook(book, error)) {
        return makeErr(error);
    }
    return makeOk("Book added.");
}

std::string handleEditBook(const ProtocolRequest& request, InventoryService& inventory)
{
    Book book;
    std::string response;
    if (!parseBookFields(request, "EDIT_BOOK", book, response)) {
        return response;
    }

    std::string error;
    if (!inventory.editBook(book, error)) {
        return makeErr(error);
    }
    return makeOk("Book updated.");
}

std::string handleRemoveBook(const ProtocolRequest& request, InventoryService& inventory)
{
    if (request.fields.size() != 2) {
        return makeErr("Malformed REMOVE_BOOK request.");
    }

    int bookId = 0;
    if (!parseIntField(request.fields[1], bookId)) {
        return makeErr("Malformed numeric field: bookId.");
    }

    std::string error;
    if (!inventory.removeBook(bookId, error)) {
        return makeErr(error);
    }
    return makeOk("Book removed.");
}

std::string handleAddSupplier(const ProtocolRequest& request, InventoryService& inventory)
{
    Supplier supplier;
    std::string response;
    if (!parseSupplierFields(request, "ADD_SUPPLIER", supplier, response)) {
        return response;
    }

    std::string error;
    if (!inventory.addSupplier(supplier, error)) {
        return makeErr(error);
    }
    return makeOk("Supplier added.");
}

std::string handleEditSupplier(const ProtocolRequest& request, InventoryService& inventory)
{
    Supplier supplier;
    std::string response;
    if (!parseSupplierFields(request, "EDIT_SUPPLIER", supplier, response)) {
        return response;
    }

    std::string error;
    if (!inventory.editSupplier(supplier, error)) {
        return makeErr(error);
    }
    return makeOk("Supplier updated.");
}

std::string handleRemoveSupplier(const ProtocolRequest& request, InventoryService& inventory)
{
    if (request.fields.size() != 2) {
        return makeErr("Malformed REMOVE_SUPPLIER request.");
    }

    int supplierId = 0;
    if (!parseIntField(request.fields[1], supplierId)) {
        return makeErr("Malformed numeric field: id.");
    }

    std::string error;
    if (!inventory.removeSupplier(supplierId, error)) {
        return makeErr(error);
    }
    return makeOk("Supplier removed.");
}

std::string handleStock(const ProtocolRequest& request, InventoryService& inventory)
{
    if (request.fields.size() != 4) {
        return makeErr("Malformed STOCK request.");
    }

    int bookId = 0;
    int quantity = 0;
    if (!parseIntField(request.fields[1], bookId)) {
        return makeErr("Malformed numeric field: bookId.");
    }
    if (!parseIntField(request.fields[3], quantity)) {
        return makeErr("Malformed numeric field: quantity.");
    }

    TransactionType type {};
    std::string error;
    if (!transactionTypeFromString(request.fields[2], type, error)) {
        return makeErr(error);
    }

    bool ok = false;
    switch (type) {
    case TransactionType::ACQUIRE:
        ok = inventory.acquire(bookId, quantity, error);
        break;
    case TransactionType::BORROW:
        ok = inventory.borrow(bookId, quantity, error);
        break;
    case TransactionType::RETURN:
        ok = inventory.returnCopies(bookId, quantity, error);
        break;
    case TransactionType::WRITEOFF:
        ok = inventory.writeOff(bookId, quantity, error);
        break;
    }

    if (!ok) {
        return makeErr(error);
    }
    return makeOk("Stock updated.");
}

std::string handleSave(const InventoryService& inventory, const LimsStore& store)
{
    std::string error;
    if (!store.save(inventory, LimsStore::kDefaultFilename, error)) {
        return makeErr(error);
    }
    return makeOk("Inventory saved.");
}

std::string dispatch(const ProtocolRequest& request, SessionRole& role, InventoryService& inventory,
                     LimsStore& store, bool& quit)
{
    quit = false;

    switch (request.type) {
    case RequestType::Ping:
        return makeOk("PONG");
    case RequestType::Role:
        return handleRole(request, role);
    case RequestType::Quit:
        quit = true;
        return makeOk("BYE");
    case RequestType::List:
    case RequestType::Search:
    case RequestType::Availability:
    case RequestType::Sort:
        if (!hasInventoryAccess(role)) {
            return makeErr("Role required.");
        }
        if (request.type == RequestType::List) {
            return encodeBooks(inventory.getBooks());
        }
        if (request.type == RequestType::Search) {
            return handleSearch(request, inventory);
        }
        if (request.type == RequestType::Sort) {
            return handleSort(request, inventory);
        }
        return handleAvailability(request, inventory);
    case RequestType::Report:
    case RequestType::AddBook:
    case RequestType::EditBook:
    case RequestType::RemoveBook:
    case RequestType::ListSuppliers:
    case RequestType::AddSupplier:
    case RequestType::EditSupplier:
    case RequestType::RemoveSupplier:
    case RequestType::Stock:
    case RequestType::ListTransactions:
    case RequestType::Save: {
        const std::string denied = requireAdmin(role);
        if (!denied.empty()) {
            return denied;
        }
        if (request.type == RequestType::Report) {
            return encodeReport(inventory.generateReport());
        }
        if (request.type == RequestType::AddBook) {
            return handleAddBook(request, inventory);
        }
        if (request.type == RequestType::EditBook) {
            return handleEditBook(request, inventory);
        }
        if (request.type == RequestType::RemoveBook) {
            return handleRemoveBook(request, inventory);
        }
        if (request.type == RequestType::ListSuppliers) {
            return encodeSuppliers(inventory.getSuppliers());
        }
        if (request.type == RequestType::AddSupplier) {
            return handleAddSupplier(request, inventory);
        }
        if (request.type == RequestType::EditSupplier) {
            return handleEditSupplier(request, inventory);
        }
        if (request.type == RequestType::RemoveSupplier) {
            return handleRemoveSupplier(request, inventory);
        }
        if (request.type == RequestType::Stock) {
            return handleStock(request, inventory);
        }
        if (request.type == RequestType::ListTransactions) {
            return encodeTransactions(inventory.getTransactions());
        }
        return handleSave(inventory, store);
    }
    case RequestType::Unknown:
        break;
    }

    return makeErr("Unknown command.");
}

} // namespace

ClientHandler::ClientHandler(InventoryService& inventory, LimsStore& store)
    : inventory_(inventory), store_(store)
{
}

void ClientHandler::handle(Socket client)
{
    SessionRole role = SessionRole::Unauthenticated;
    std::string error;

    while (true) {
        std::string line;
        if (!client.receiveLine(line, error)) {
            break;
        }

        bool quit = false;
        const std::string response = dispatch(parseRequest(line), role, inventory_, store_, quit);
        if (!client.sendLine(response, error)) {
            break;
        }
        if (quit) {
            break;
        }
    }
}
