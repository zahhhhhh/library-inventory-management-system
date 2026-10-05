#ifndef LIMS_INVENTORY_INVENTORY_SERVICE_HPP
#define LIMS_INVENTORY_INVENTORY_SERVICE_HPP

#include "domain/Book.hpp"
#include "domain/Supplier.hpp"
#include "domain/Transaction.hpp"
#include "domain/TransactionType.hpp"

#include <string>
#include <vector>

enum class CatalogueSort {
    Title,
    Author,
    Year,
    Isbn
};

struct InventoryReport {
    int titleCount = 0;
    int totalCopies = 0;
    int availableCopies = 0;
    std::vector<Book> unavailableBooks;
};

class InventoryService {
public:
    bool addBook(const Book& book, std::string& error);
    bool editBook(const Book& book, std::string& error);
    bool removeBook(int id, std::string& error);

    const std::vector<Book>& getBooks() const;
    const Book* findBookById(int id) const;

    std::vector<Book> searchByIsbn(const std::string& isbn) const;
    std::vector<Book> searchByTitle(const std::string& query) const;
    std::vector<Book> searchByAuthor(const std::string& query) const;

    void sortCatalogue(CatalogueSort key);

    bool getAvailability(int bookId, int& availableCopies, std::string& error) const;

    bool addSupplier(const Supplier& supplier, std::string& error);
    bool editSupplier(const Supplier& supplier, std::string& error);
    bool removeSupplier(int id, std::string& error);
    const std::vector<Supplier>& getSuppliers() const;
    const Supplier* findSupplierById(int id) const;

    bool acquire(int bookId, int quantity, std::string& error);
    bool borrow(int bookId, int quantity, std::string& error);
    bool returnCopies(int bookId, int quantity, std::string& error);
    bool writeOff(int bookId, int quantity, std::string& error);

    const std::vector<Transaction>& getTransactions() const;

    InventoryReport generateReport() const;

    bool replaceState(const std::vector<Supplier>& suppliers,
                      const std::vector<Book>& books,
                      const std::vector<Transaction>& transactions,
                      std::string& error);

private:
    std::vector<Book> books_;
    std::vector<Supplier> suppliers_;
    std::vector<Transaction> transactions_;
    int nextTransactionId_ = 1;

    Book* findBookByIdMutable(int id);
    Supplier* findSupplierByIdMutable(int id);

    bool validateBook(const Book& book, int ignoreBookId, std::string& error) const;
    bool validateSupplier(const Supplier& supplier, std::string& error) const;
    bool isbnTaken(const std::string& isbn, int ignoreBookId) const;
    bool bookIdTaken(int id) const;
    bool supplierIdTaken(int id) const;
    bool bookHasTransactions(int bookId) const;
    bool supplierIsReferenced(int supplierId) const;

    bool applyStockChange(int bookId, TransactionType type, int quantity, std::string& error);
    void recordTransaction(int bookId, TransactionType type, int quantity);
    std::string currentTimestamp() const;
};

#endif
