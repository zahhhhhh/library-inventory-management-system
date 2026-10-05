#include "persistence/LimsStore.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string trimCopy(std::string value)
{
    const auto isNotSpace = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), isNotSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), isNotSpace).base(), value.end());
    return value;
}

void stripCarriageReturn(std::string& line)
{
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
}

std::vector<std::string> splitPipe(const std::string& line)
{
    std::vector<std::string> fields;
    std::string current;
    for (char ch : line) {
        if (ch == '|') {
            fields.push_back(trimCopy(current));
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    fields.push_back(trimCopy(current));
    return fields;
}

bool parseIntField(const std::string& text, int& value, const std::string& fieldName, std::string& error)
{
    const std::string trimmed = trimCopy(text);
    if (trimmed.empty()) {
        error = "Malformed numeric field: " + fieldName + ".";
        return false;
    }

    std::size_t consumed = 0;
    try {
        value = std::stoi(trimmed, &consumed);
    } catch (const std::exception&) {
        error = "Malformed numeric field: " + fieldName + ".";
        return false;
    }

    if (consumed != trimmed.size()) {
        error = "Malformed numeric field: " + fieldName + ".";
        return false;
    }

    return true;
}

enum class Section {
    None,
    Suppliers,
    Books,
    Transactions
};

bool parseSectionName(const std::string& line, Section& section, std::string& error)
{
    if (line == "[SUPPLIERS]") {
        section = Section::Suppliers;
        return true;
    }
    if (line == "[BOOKS]") {
        section = Section::Books;
        return true;
    }
    if (line == "[TRANSACTIONS]") {
        section = Section::Transactions;
        return true;
    }

    error = "Unknown section: " + line;
    return false;
}

bool parseSupplierRecord(const std::string& line, Supplier& supplier, std::string& error)
{
    const std::vector<std::string> fields = splitPipe(line);
    if (fields.size() != 3) {
        error = "Malformed supplier record.";
        return false;
    }

    if (!parseIntField(fields[0], supplier.id, "supplier id", error)) {
        return false;
    }
    supplier.name = fields[1];
    supplier.contact = fields[2];
    return true;
}

bool parseBookRecord(const std::string& line, Book& book, std::string& error)
{
    const std::vector<std::string> fields = splitPipe(line);
    if (fields.size() != 8) {
        error = "Malformed book record.";
        return false;
    }

    if (!parseIntField(fields[0], book.id, "book id", error) ||
        !parseIntField(fields[4], book.year, "year", error) ||
        !parseIntField(fields[5], book.totalCopies, "totalCopies", error) ||
        !parseIntField(fields[6], book.availableCopies, "availableCopies", error) ||
        !parseIntField(fields[7], book.supplierId, "supplierId", error)) {
        return false;
    }

    book.isbn = fields[1];
    book.title = fields[2];
    book.author = fields[3];
    return true;
}

bool parseTransactionRecord(const std::string& line, Transaction& transaction, std::string& error)
{
    const std::vector<std::string> fields = splitPipe(line);
    if (fields.size() != 5) {
        error = "Malformed transaction record.";
        return false;
    }

    if (!parseIntField(fields[0], transaction.id, "transaction id", error) ||
        !parseIntField(fields[1], transaction.bookId, "bookId", error) ||
        !parseIntField(fields[3], transaction.quantity, "quantity", error)) {
        return false;
    }

    if (!transactionTypeFromString(fields[2], transaction.type, error)) {
        return false;
    }

    transaction.timestamp = fields[4];
    return true;
}

bool ensureParentDirectory(const std::filesystem::path& path, std::string& error)
{
    const std::filesystem::path parent = path.parent_path();
    if (parent.empty()) {
        return true;
    }

    std::error_code ioError;
    std::filesystem::create_directories(parent, ioError);
    if (ioError) {
        error = "Cannot create directory '" + parent.string() + "'.";
        return false;
    }

    return true;
}

} // namespace

std::string transactionTypeToString(TransactionType type)
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

bool transactionTypeFromString(const std::string& text, TransactionType& type, std::string& error)
{
    const std::string value = trimCopy(text);
    if (value == "ACQUIRE") {
        type = TransactionType::ACQUIRE;
        return true;
    }
    if (value == "BORROW") {
        type = TransactionType::BORROW;
        return true;
    }
    if (value == "RETURN") {
        type = TransactionType::RETURN;
        return true;
    }
    if (value == "WRITEOFF") {
        type = TransactionType::WRITEOFF;
        return true;
    }

    error = "Unknown transaction type: " + value;
    return false;
}

bool LimsStore::save(const InventoryService& inventory,
                     const std::string& filename,
                     std::string& error) const
{
    const std::filesystem::path path(filename);
    if (!ensureParentDirectory(path, error)) {
        return false;
    }

    std::ofstream out(path);
    if (!out) {
        error = "Cannot write inventory file '" + filename + "'.";
        return false;
    }

    out << "LIMS1\n\n";

    out << "[SUPPLIERS]\n";
    for (const Supplier& supplier : inventory.getSuppliers()) {
        out << supplier.id << '|' << supplier.name << '|' << supplier.contact << '\n';
    }

    out << "\n[BOOKS]\n";
    for (const Book& book : inventory.getBooks()) {
        out << book.id << '|' << book.isbn << '|' << book.title << '|' << book.author << '|'
            << book.year << '|' << book.totalCopies << '|' << book.availableCopies << '|'
            << book.supplierId << '\n';
    }

    out << "\n[TRANSACTIONS]\n";
    for (const Transaction& transaction : inventory.getTransactions()) {
        out << transaction.id << '|' << transaction.bookId << '|'
            << transactionTypeToString(transaction.type) << '|' << transaction.quantity << '|'
            << transaction.timestamp << '\n';
    }

    if (!out) {
        error = "Failed while writing inventory file '" + filename + "'.";
        return false;
    }

    return true;
}

bool LimsStore::load(InventoryService& inventory,
                     const std::string& filename,
                     std::string& error) const
{
    const std::filesystem::path path(filename);
    std::error_code ioError;
    const bool exists = std::filesystem::exists(path, ioError);
    if (ioError) {
        error = "Cannot access inventory file '" + filename + "'.";
        return false;
    }
    if (!exists) {
        return inventory.replaceState({}, {}, {}, error);
    }

    if (std::filesystem::is_directory(path, ioError)) {
        error = "Cannot open inventory file '" + filename + "'.";
        return false;
    }

    std::ifstream in(path);
    if (!in) {
        error = "Cannot read inventory file '" + filename + "'.";
        return false;
    }

    std::string line;
    bool seenHeader = false;
    bool seenSuppliers = false;
    bool seenBooks = false;
    bool seenTransactions = false;
    Section section = Section::None;

    std::vector<Supplier> suppliers;
    std::vector<Book> books;
    std::vector<Transaction> transactions;

    while (std::getline(in, line)) {
        stripCarriageReturn(line);
        const std::string trimmed = trimCopy(line);
        if (trimmed.empty()) {
            continue;
        }

        if (!seenHeader) {
            if (trimmed != "LIMS1") {
                error = "Invalid LIMS1 header.";
                return false;
            }
            seenHeader = true;
            continue;
        }

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            if (!parseSectionName(trimmed, section, error)) {
                return false;
            }

            if (section == Section::Suppliers) {
                if (seenSuppliers) {
                    error = "Duplicate [SUPPLIERS] section.";
                    return false;
                }
                seenSuppliers = true;
            } else if (section == Section::Books) {
                if (seenBooks) {
                    error = "Duplicate [BOOKS] section.";
                    return false;
                }
                seenBooks = true;
            } else if (section == Section::Transactions) {
                if (seenTransactions) {
                    error = "Duplicate [TRANSACTIONS] section.";
                    return false;
                }
                seenTransactions = true;
            }
            continue;
        }

        if (section == Section::None) {
            error = "Malformed record outside a labelled section.";
            return false;
        }

        if (section == Section::Suppliers) {
            Supplier supplier;
            if (!parseSupplierRecord(trimmed, supplier, error)) {
                return false;
            }
            suppliers.push_back(supplier);
        } else if (section == Section::Books) {
            Book book;
            if (!parseBookRecord(trimmed, book, error)) {
                return false;
            }
            books.push_back(book);
        } else if (section == Section::Transactions) {
            Transaction transaction;
            if (!parseTransactionRecord(trimmed, transaction, error)) {
                return false;
            }
            transactions.push_back(transaction);
        }
    }

    if (!seenHeader) {
        error = "Invalid LIMS1 header.";
        return false;
    }

    InventoryService parsed;
    if (!parsed.replaceState(suppliers, books, transactions, error)) {
        return false;
    }

    return inventory.replaceState(parsed.getSuppliers(), parsed.getBooks(), parsed.getTransactions(), error);
}
