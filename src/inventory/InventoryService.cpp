#include "inventory/InventoryService.hpp"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <utility>

namespace {

std::string trimCopy(std::string value)
{
    const auto isNotSpace = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), isNotSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), isNotSpace).base(), value.end());
    return value;
}

std::string toLowerCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool containsIgnoreCase(const std::string& haystack, const std::string& needle)
{
    return toLowerCopy(haystack).find(toLowerCopy(needle)) != std::string::npos;
}

} // namespace

bool InventoryService::addBook(const Book& book, std::string& error)
{
    if (bookIdTaken(book.id)) {
        error = "Book ID must be unique.";
        return false;
    }

    if (!validateBook(book, 0, error)) {
        return false;
    }

    books_.push_back(book);
    return true;
}

bool InventoryService::editBook(const Book& book, std::string& error)
{
    Book* existing = findBookByIdMutable(book.id);
    if (existing == nullptr) {
        error = "Book not found.";
        return false;
    }

    if (!validateBook(book, book.id, error)) {
        return false;
    }

    *existing = book;
    return true;
}

bool InventoryService::removeBook(int id, std::string& error)
{
    auto it = std::find_if(books_.begin(), books_.end(), [id](const Book& book) {
        return book.id == id;
    });

    if (it == books_.end()) {
        error = "Book not found.";
        return false;
    }

    if (bookHasTransactions(id)) {
        error = "Cannot remove a book that has transaction history.";
        return false;
    }

    books_.erase(it);
    return true;
}

const std::vector<Book>& InventoryService::getBooks() const
{
    return books_;
}

const Book* InventoryService::findBookById(int id) const
{
    auto it = std::find_if(books_.begin(), books_.end(), [id](const Book& book) {
        return book.id == id;
    });
    return it == books_.end() ? nullptr : &(*it);
}

std::vector<Book> InventoryService::searchByIsbn(const std::string& isbn) const
{
    const std::string wanted = trimCopy(isbn);
    std::vector<Book> matches;
    for (const Book& book : books_) {
        if (book.isbn == wanted) {
            matches.push_back(book);
        }
    }
    return matches;
}

std::vector<Book> InventoryService::searchByTitle(const std::string& query) const
{
    const std::string wanted = trimCopy(query);
    std::vector<Book> matches;
    if (wanted.empty()) {
        return matches;
    }

    for (const Book& book : books_) {
        if (containsIgnoreCase(book.title, wanted)) {
            matches.push_back(book);
        }
    }
    return matches;
}

std::vector<Book> InventoryService::searchByAuthor(const std::string& query) const
{
    const std::string wanted = trimCopy(query);
    std::vector<Book> matches;
    if (wanted.empty()) {
        return matches;
    }

    for (const Book& book : books_) {
        if (containsIgnoreCase(book.author, wanted)) {
            matches.push_back(book);
        }
    }
    return matches;
}

void InventoryService::sortCatalogue(CatalogueSort key)
{
    std::sort(books_.begin(), books_.end(), [key](const Book& left, const Book& right) {
        switch (key) {
        case CatalogueSort::Title:
            return left.title < right.title;
        case CatalogueSort::Author:
            return left.author < right.author;
        case CatalogueSort::Year:
            return left.year < right.year;
        case CatalogueSort::Isbn:
            return left.isbn < right.isbn;
        }
        return left.id < right.id;
    });
}

bool InventoryService::getAvailability(int bookId, int& availableCopies, std::string& error) const
{
    const Book* book = findBookById(bookId);
    if (book == nullptr) {
        error = "Book not found.";
        return false;
    }

    availableCopies = book->availableCopies;
    return true;
}

bool InventoryService::addSupplier(const Supplier& supplier, std::string& error)
{
    if (supplierIdTaken(supplier.id)) {
        error = "Supplier ID must be unique.";
        return false;
    }

    if (!validateSupplier(supplier, error)) {
        return false;
    }

    suppliers_.push_back(supplier);
    return true;
}

bool InventoryService::editSupplier(const Supplier& supplier, std::string& error)
{
    Supplier* existing = findSupplierByIdMutable(supplier.id);
    if (existing == nullptr) {
        error = "Supplier not found.";
        return false;
    }

    if (!validateSupplier(supplier, error)) {
        return false;
    }

    *existing = supplier;
    return true;
}

bool InventoryService::removeSupplier(int id, std::string& error)
{
    auto it = std::find_if(suppliers_.begin(), suppliers_.end(), [id](const Supplier& supplier) {
        return supplier.id == id;
    });

    if (it == suppliers_.end()) {
        error = "Supplier not found.";
        return false;
    }

    if (supplierIsReferenced(id)) {
        error = "Cannot remove a supplier that is referenced by a book.";
        return false;
    }

    suppliers_.erase(it);
    return true;
}

const std::vector<Supplier>& InventoryService::getSuppliers() const
{
    return suppliers_;
}

const Supplier* InventoryService::findSupplierById(int id) const
{
    auto it = std::find_if(suppliers_.begin(), suppliers_.end(), [id](const Supplier& supplier) {
        return supplier.id == id;
    });
    return it == suppliers_.end() ? nullptr : &(*it);
}

bool InventoryService::acquire(int bookId, int quantity, std::string& error)
{
    return applyStockChange(bookId, TransactionType::ACQUIRE, quantity, error);
}

bool InventoryService::borrow(int bookId, int quantity, std::string& error)
{
    return applyStockChange(bookId, TransactionType::BORROW, quantity, error);
}

bool InventoryService::returnCopies(int bookId, int quantity, std::string& error)
{
    return applyStockChange(bookId, TransactionType::RETURN, quantity, error);
}

bool InventoryService::writeOff(int bookId, int quantity, std::string& error)
{
    return applyStockChange(bookId, TransactionType::WRITEOFF, quantity, error);
}

const std::vector<Transaction>& InventoryService::getTransactions() const
{
    return transactions_;
}

bool InventoryService::replaceState(const std::vector<Supplier>& suppliers,
                                   const std::vector<Book>& books,
                                   const std::vector<Transaction>& transactions,
                                   std::string& error)
{
    InventoryService candidate;

    for (const Supplier& supplier : suppliers) {
        if (!candidate.addSupplier(supplier, error)) {
            return false;
        }
    }

    for (const Book& book : books) {
        if (!candidate.addBook(book, error)) {
            return false;
        }
    }

    int maxTransactionId = 0;
    for (const Transaction& transaction : transactions) {
        if (transaction.id <= 0) {
            error = "Transaction ID must be greater than 0.";
            return false;
        }

        if (transaction.id > maxTransactionId) {
            maxTransactionId = transaction.id;
        }

        for (const Transaction& existing : candidate.transactions_) {
            if (existing.id == transaction.id) {
                error = "Transaction ID must be unique.";
                return false;
            }
        }

        if (transaction.quantity <= 0) {
            error = "Transaction quantity must be greater than 0.";
            return false;
        }

        if (candidate.findBookById(transaction.bookId) == nullptr) {
            error = "Transaction references unknown book.";
            return false;
        }

        candidate.transactions_.push_back(transaction);
    }

    candidate.nextTransactionId_ = maxTransactionId + 1;
    *this = std::move(candidate);
    return true;
}

InventoryReport InventoryService::generateReport() const
{
    InventoryReport report;
    report.titleCount = static_cast<int>(books_.size());

    for (const Book& book : books_) {
        report.totalCopies += book.totalCopies;
        report.availableCopies += book.availableCopies;
        if (book.availableCopies == 0) {
            report.unavailableBooks.push_back(book);
        }
    }

    return report;
}

Book* InventoryService::findBookByIdMutable(int id)
{
    return const_cast<Book*>(static_cast<const InventoryService*>(this)->findBookById(id));
}

Supplier* InventoryService::findSupplierByIdMutable(int id)
{
    return const_cast<Supplier*>(static_cast<const InventoryService*>(this)->findSupplierById(id));
}

bool InventoryService::validateBook(const Book& book, int ignoreBookId, std::string& error) const
{
    if (book.id <= 0) {
        error = "Book ID must be greater than 0.";
        return false;
    }

    const std::string isbn = trimCopy(book.isbn);
    if (isbn.empty()) {
        error = "ISBN must not be empty.";
        return false;
    }

    if (isbnTaken(isbn, ignoreBookId)) {
        error = "ISBN must be unique.";
        return false;
    }

    if (trimCopy(book.title).empty()) {
        error = "Title must not be empty.";
        return false;
    }

    if (trimCopy(book.author).empty()) {
        error = "Author must not be empty.";
        return false;
    }

    if (book.year <= 0) {
        error = "Year must be greater than 0.";
        return false;
    }

    if (book.totalCopies < 0) {
        error = "Total copies cannot be negative.";
        return false;
    }

    if (book.availableCopies < 0 || book.availableCopies > book.totalCopies) {
        error = "Available copies must be between 0 and total copies.";
        return false;
    }

    if (book.supplierId != 0 && findSupplierById(book.supplierId) == nullptr) {
        error = "Supplier not found.";
        return false;
    }

    return true;
}

bool InventoryService::validateSupplier(const Supplier& supplier, std::string& error) const
{
    if (supplier.id <= 0) {
        error = "Supplier ID must be greater than 0.";
        return false;
    }

    if (trimCopy(supplier.name).empty()) {
        error = "Supplier name must not be empty.";
        return false;
    }

    return true;
}

bool InventoryService::isbnTaken(const std::string& isbn, int ignoreBookId) const
{
    for (const Book& book : books_) {
        if (book.id != ignoreBookId && trimCopy(book.isbn) == isbn) {
            return true;
        }
    }
    return false;
}

bool InventoryService::bookIdTaken(int id) const
{
    return findBookById(id) != nullptr;
}

bool InventoryService::supplierIdTaken(int id) const
{
    return findSupplierById(id) != nullptr;
}

bool InventoryService::bookHasTransactions(int bookId) const
{
    for (const Transaction& transaction : transactions_) {
        if (transaction.bookId == bookId) {
            return true;
        }
    }
    return false;
}

bool InventoryService::supplierIsReferenced(int supplierId) const
{
    for (const Book& book : books_) {
        if (book.supplierId == supplierId) {
            return true;
        }
    }
    return false;
}

bool InventoryService::applyStockChange(int bookId, TransactionType type, int quantity, std::string& error)
{
    if (quantity <= 0) {
        error = "Quantity must be greater than 0.";
        return false;
    }

    Book* book = findBookByIdMutable(bookId);
    if (book == nullptr) {
        error = "Book not found.";
        return false;
    }

    int totalCopies = book->totalCopies;
    int availableCopies = book->availableCopies;

    switch (type) {
    case TransactionType::ACQUIRE:
        totalCopies += quantity;
        availableCopies += quantity;
        break;
    case TransactionType::BORROW:
        if (quantity > availableCopies) {
            error = "Cannot borrow more copies than are available.";
            return false;
        }
        availableCopies -= quantity;
        break;
    case TransactionType::RETURN:
        if (availableCopies + quantity > totalCopies) {
            error = "Cannot return more copies than have been borrowed.";
            return false;
        }
        availableCopies += quantity;
        break;
    case TransactionType::WRITEOFF:
        if (quantity > availableCopies) {
            error = "Cannot write off more copies than are available.";
            return false;
        }
        totalCopies -= quantity;
        availableCopies -= quantity;
        break;
    }

    if (availableCopies < 0 || availableCopies > totalCopies || totalCopies < 0) {
        error = "Stock change would break available/total copy limits.";
        return false;
    }

    book->totalCopies = totalCopies;
    book->availableCopies = availableCopies;
    recordTransaction(bookId, type, quantity);
    return true;
}

void InventoryService::recordTransaction(int bookId, TransactionType type, int quantity)
{
    Transaction transaction;
    transaction.id = nextTransactionId_++;
    transaction.bookId = bookId;
    transaction.type = type;
    transaction.quantity = quantity;
    transaction.timestamp = currentTimestamp();
    transactions_.push_back(transaction);
}

std::string InventoryService::currentTimestamp() const
{
    const std::time_t now = std::time(nullptr);
    std::tm localTime {};
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    char buffer[20] = {};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);
    return buffer;
}
