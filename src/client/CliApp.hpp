#ifndef LIMS_CLIENT_CLI_APP_HPP
#define LIMS_CLIENT_CLI_APP_HPP

#include "client/ClientSession.hpp"

#include <string>

class CliApp {
public:
    void run();

private:
    ClientSession session_;

    void printMainMenu() const;
    void printUserMenu() const;
    void printAdminMenu() const;

    bool connectAsUser();
    bool connectAsAdmin();
    void disconnect();

    void userLoop();
    void adminLoop();

    void viewCatalogue();
    void searchBooks();
    void checkAvailability();
    void sortCatalogue();
    void addBook();
    void editBook();
    void removeBook();
    void viewSuppliers();
    void addSupplier();
    void editSupplier();
    void removeSupplier();
    void recordStockMovement();
    void viewTransactions();
    void generateReport();
    void saveInventory();

    bool send(const std::string& command, std::string& response);
    void showResponse(const std::string& response);
    void showError(const std::string& text) const;

    bool readMenuChoice(int minChoice, int maxChoice, int& choice);
    bool readRequiredInt(const std::string& prompt, int& value);
    bool readRequiredLine(const std::string& prompt, std::string& value);
};

#endif
