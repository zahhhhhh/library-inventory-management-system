#include "protocol/Protocol.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {

std::string trimCopy(std::string value)
{
    const auto isNotSpace = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), isNotSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), isNotSpace).base(), value.end());
    return value;
}

std::vector<std::string> splitPipe(const std::string& text)
{
    std::vector<std::string> fields;
    std::string current;
    for (char ch : text) {
        if (ch == '|') {
            fields.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    fields.push_back(current);
    return fields;
}

} // namespace

ProtocolRequest parseRequest(const std::string& line)
{
    ProtocolRequest request;
    request.fields = splitPipe(trimCopy(line));
    if (request.fields.empty()) {
        request.type = RequestType::Unknown;
        return request;
    }

    const std::string& command = request.fields[0];
    if (command == "PING") {
        request.type = RequestType::Ping;
    } else if (command == "ROLE") {
        request.type = RequestType::Role;
    } else if (command == "LIST") {
        request.type = RequestType::List;
    } else if (command == "SEARCH") {
        request.type = RequestType::Search;
    } else if (command == "AVAILABILITY") {
        request.type = RequestType::Availability;
    } else if (command == "SORT") {
        request.type = RequestType::Sort;
    } else if (command == "REPORT") {
        request.type = RequestType::Report;
    } else if (command == "ADD_BOOK") {
        request.type = RequestType::AddBook;
    } else if (command == "EDIT_BOOK") {
        request.type = RequestType::EditBook;
    } else if (command == "REMOVE_BOOK") {
        request.type = RequestType::RemoveBook;
    } else if (command == "LIST_SUPPLIERS") {
        request.type = RequestType::ListSuppliers;
    } else if (command == "ADD_SUPPLIER") {
        request.type = RequestType::AddSupplier;
    } else if (command == "EDIT_SUPPLIER") {
        request.type = RequestType::EditSupplier;
    } else if (command == "REMOVE_SUPPLIER") {
        request.type = RequestType::RemoveSupplier;
    } else if (command == "STOCK") {
        request.type = RequestType::Stock;
    } else if (command == "LIST_TRANSACTIONS") {
        request.type = RequestType::ListTransactions;
    } else if (command == "SAVE") {
        request.type = RequestType::Save;
    } else if (command == "QUIT") {
        request.type = RequestType::Quit;
    } else {
        request.type = RequestType::Unknown;
    }

    return request;
}

std::string makeOk(const std::string& payload)
{
    return payload.empty() ? std::string("OK|") : ("OK|" + payload);
}

std::string makeErr(const std::string& message)
{
    return "ERR|" + message;
}

std::string encodeBooks(const std::vector<Book>& books)
{
    std::ostringstream out;
    out << "BOOKS|";
    bool first = true;
    for (const Book& book : books) {
        if (!first) {
            out << ';';
        }
        first = false;
        out << book.id << '~' << book.isbn << '~' << book.title << '~' << book.author << '~'
            << book.availableCopies << '~' << book.totalCopies;
    }
    return makeOk(out.str());
}

std::string transactionTypeLabel(TransactionType type)
{
    switch (type) {
    case TransactionType::ACQUIRE:
        return "ACQUIRE";
    case TransactionType::BORROW:
        return "BORROW";
    case TransactionType::RETURN:
        return "RETURN";
    case TransactionType::WRITEOFF:
        return "WRITEOFF";
    }
    return "UNKNOWN";
}

std::string encodeSuppliers(const std::vector<Supplier>& suppliers)
{
    std::ostringstream out;
    out << "SUPPLIERS|";
    bool first = true;
    for (const Supplier& supplier : suppliers) {
        if (!first) {
            out << ';';
        }
        first = false;
        out << supplier.id << '~' << supplier.name << '~' << supplier.contact;
    }
    return makeOk(out.str());
}

std::string encodeTransactions(const std::vector<Transaction>& transactions)
{
    std::ostringstream out;
    out << "TRANSACTIONS|";
    bool first = true;
    for (const Transaction& transaction : transactions) {
        if (!first) {
            out << ';';
        }
        first = false;
        out << transaction.id << '~' << transaction.bookId << '~'
            << transactionTypeLabel(transaction.type) << '~' << transaction.quantity << '~'
            << transaction.timestamp;
    }
    return makeOk(out.str());
}

std::string encodeAvailability(int bookId, int availableCopies)
{
    return makeOk("AVAILABILITY|" + std::to_string(bookId) + "|" + std::to_string(availableCopies));
}

std::string encodeReport(const InventoryReport& report)
{
    return makeOk("REPORT|titles=" + std::to_string(report.titleCount) +
                  "~totalCopies=" + std::to_string(report.totalCopies) +
                  "~availableCopies=" + std::to_string(report.availableCopies) +
                  "~unavailable=" + std::to_string(static_cast<int>(report.unavailableBooks.size())));
}
