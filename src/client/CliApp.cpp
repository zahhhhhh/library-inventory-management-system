#include "client/CliApp.hpp"

#include "protocol/Protocol.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string trimCopy(std::string value)
{
    const auto isNotSpace = [](unsigned char ch) { return !std::isspace(static_cast<unsigned char>(ch)); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), isNotSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), isNotSpace).base(), value.end());
    return value;
}

std::vector<std::string> split(const std::string& text, char delimiter)
{
    std::vector<std::string> parts;
    std::string current;
    for (char ch : text) {
        if (ch == delimiter) {
            parts.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    parts.push_back(current);
    return parts;
}

bool parseIntText(const std::string& text, int& value)
{
    const std::string trimmed = trimCopy(text);
    if (trimmed.empty()) {
        return false;
    }

    std::size_t consumed = 0;
    try {
        value = std::stoi(trimmed, &consumed);
    } catch (const std::exception&) {
        return false;
    }

    return consumed == trimmed.size();
}

void printRule()
{
    std::cout << "------------------------------------------------------------\n";
}

void displayBooks(const std::string& records)
{
    std::cout << "\nCatalogue\n";
    printRule();
    if (records.empty()) {
        std::cout << "No books found.\n";
        printRule();
        return;
    }

    const std::vector<std::string> rows = split(records, ';');
    for (const std::string& row : rows) {
        if (row.empty()) {
            continue;
        }
        const std::vector<std::string> fields = split(row, '~');
        if (fields.size() < 6) {
            continue;
        }
        std::cout << "ID: " << fields[0] << '\n';
        std::cout << "ISBN: " << fields[1] << '\n';
        std::cout << "Title: " << fields[2] << '\n';
        std::cout << "Author: " << fields[3] << '\n';
        std::cout << "Available: " << fields[4] << " / " << fields[5] << '\n';
        printRule();
    }
}

void displaySuppliers(const std::string& records)
{
    std::cout << "\nSuppliers\n";
    printRule();
    if (records.empty()) {
        std::cout << "No suppliers found.\n";
        printRule();
        return;
    }

    const std::vector<std::string> rows = split(records, ';');
    for (const std::string& row : rows) {
        if (row.empty()) {
            continue;
        }
        const std::vector<std::string> fields = split(row, '~');
        if (fields.size() < 3) {
            continue;
        }
        std::cout << "ID: " << fields[0] << '\n';
        std::cout << "Name: " << fields[1] << '\n';
        std::cout << "Contact: " << fields[2] << '\n';
        printRule();
    }
}

void displayTransactions(const std::string& records)
{
    std::cout << "\nTransaction History\n";
    printRule();
    if (records.empty()) {
        std::cout << "No transactions found.\n";
        printRule();
        return;
    }

    const std::vector<std::string> rows = split(records, ';');
    for (const std::string& row : rows) {
        if (row.empty()) {
            continue;
        }
        const std::vector<std::string> fields = split(row, '~');
        if (fields.size() < 5) {
            continue;
        }
        std::cout << "ID: " << fields[0] << '\n';
        std::cout << "Book ID: " << fields[1] << '\n';
        std::cout << "Type: " << fields[2] << '\n';
        std::cout << "Quantity: " << fields[3] << '\n';
        std::cout << "Date/Time: " << fields[4] << '\n';
        printRule();
    }
}

void displayReport(const std::string& payload)
{
    int titles = 0;
    int total = 0;
    int available = 0;
    int unavailable = 0;
    for (const std::string& item : split(payload, '~')) {
        const auto pos = item.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        const std::string key = item.substr(0, pos);
        int value = 0;
        if (!parseIntText(item.substr(pos + 1), value)) {
            continue;
        }
        if (key == "titles") {
            titles = value;
        } else if (key == "totalCopies") {
            total = value;
        } else if (key == "availableCopies") {
            available = value;
        } else if (key == "unavailable") {
            unavailable = value;
        }
    }

    std::cout << "\nInventory Report\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Book titles: " << titles << '\n';
    std::cout << "Total copies: " << total << '\n';
    std::cout << "Available copies: " << available << '\n';
    std::cout << "Unavailable titles: " << unavailable << '\n';
}

void displayAvailability(const std::vector<std::string>& parts)
{
    if (parts.size() < 4) {
        return;
    }
    std::cout << "\nAvailability for book " << parts[2] << ": " << parts[3] << " copies\n";
}

} // namespace

void CliApp::printMainMenu() const
{
    std::cout << "\n========================================\n";
    std::cout << " Library Inventory Management System\n";
    std::cout << "========================================\n\n";
    std::cout << "1. Regular User\n";
    std::cout << "2. Administrator\n";
    std::cout << "3. Exit\n\n";
}

void CliApp::printUserMenu() const
{
    std::cout << "\n----------------------------------------\n";
    std::cout << " Regular User Menu\n";
    std::cout << "----------------------------------------\n\n";
    std::cout << "1. View catalogue\n";
    std::cout << "2. Search books\n";
    std::cout << "3. Check book availability\n";
    std::cout << "4. Sort catalogue\n";
    std::cout << "5. Return to main menu\n\n";
}

void CliApp::printAdminMenu() const
{
    std::cout << "\n----------------------------------------\n";
    std::cout << " Administrator Menu\n";
    std::cout << "----------------------------------------\n\n";
    std::cout << "1. View catalogue\n";
    std::cout << "2. Search books\n";
    std::cout << "3. Check book availability\n";
    std::cout << "4. Sort catalogue\n";
    std::cout << "5. Add book\n";
    std::cout << "6. Edit book\n";
    std::cout << "7. Remove book\n";
    std::cout << "8. View suppliers\n";
    std::cout << "9. Add supplier\n";
    std::cout << "10. Edit supplier\n";
    std::cout << "11. Remove supplier\n";
    std::cout << "12. Record stock movement\n";
    std::cout << "13. View transaction history\n";
    std::cout << "14. Generate inventory report\n";
    std::cout << "15. Save inventory\n";
    std::cout << "16. Return to main menu\n\n";
}

void CliApp::showError(const std::string& text) const
{
    std::cout << "Error: " << text << '\n';
}

bool CliApp::send(const std::string& command, std::string& response)
{
    std::string error;
    if (!session_.request(command, response, error)) {
        showError(error);
        return false;
    }
    return true;
}

void CliApp::showResponse(const std::string& response)
{
    const std::vector<std::string> parts = split(response, '|');
    if (parts.empty()) {
        showError("Empty server response.");
        return;
    }

    if (parts[0] == "ERR") {
        showError(parts.size() > 1 ? parts[1] : "Request failed.");
        return;
    }

    if (parts[0] != "OK") {
        showError(response);
        return;
    }

    if (parts.size() > 1 && parts[1] == "BOOKS") {
        displayBooks(parts.size() > 2 ? parts[2] : "");
        return;
    }
    if (parts.size() > 1 && parts[1] == "SUPPLIERS") {
        displaySuppliers(parts.size() > 2 ? parts[2] : "");
        return;
    }
    if (parts.size() > 1 && parts[1] == "TRANSACTIONS") {
        displayTransactions(parts.size() > 2 ? parts[2] : "");
        return;
    }
    if (parts.size() > 1 && parts[1] == "REPORT") {
        displayReport(parts.size() > 2 ? parts[2] : "");
        return;
    }
    if (parts.size() > 1 && parts[1] == "AVAILABILITY") {
        displayAvailability(parts);
        return;
    }

    if (parts.size() > 1 && !parts[1].empty()) {
        std::cout << parts[1] << '\n';
    }
}

bool CliApp::readRequiredLine(const std::string& prompt, std::string& value)
{
    while (true) {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line)) {
            return false;
        }
        value = trimCopy(line);
        if (!value.empty()) {
            return true;
        }
        std::cout << "Input cannot be empty.\n";
    }
}

bool CliApp::readRequiredInt(const std::string& prompt, int& value)
{
    while (true) {
        std::string line;
        if (!readRequiredLine(prompt, line)) {
            return false;
        }
        if (parseIntText(line, value)) {
            return true;
        }
        std::cout << "Please enter a whole number.\n";
    }
}

bool CliApp::readMenuChoice(int minChoice, int maxChoice, int& choice)
{
    while (true) {
        std::cout << "Select option: ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            return false;
        }

        if (!parseIntText(line, choice) || choice < minChoice || choice > maxChoice) {
            std::cout << "Invalid option.\n";
            continue;
        }
        return true;
    }
}

void CliApp::disconnect()
{
    if (!session_.isConnected()) {
        return;
    }

    std::string response;
    std::string error;
    session_.request("QUIT", response, error);
    session_.close();
}

bool CliApp::connectAsUser()
{
    disconnect();
    std::string error;
    if (!session_.connect(kDefaultHost, kDefaultPort, error)) {
        showError(error);
        return false;
    }

    std::string response;
    if (!send("ROLE|USER", response)) {
        session_.close();
        return false;
    }
    if (response.rfind("ERR|", 0) == 0) {
        showResponse(response);
        session_.close();
        return false;
    }
    return true;
}

bool CliApp::connectAsAdmin()
{
    std::string password;
    if (!readRequiredLine("Administrator password: ", password)) {
        return false;
    }

    disconnect();
    std::string error;
    if (!session_.connect(kDefaultHost, kDefaultPort, error)) {
        showError(error);
        return false;
    }

    std::string response;
    if (!send("ROLE|ADMIN|" + password, response)) {
        session_.close();
        return false;
    }
    if (response.rfind("ERR|", 0) == 0) {
        showResponse(response);
        session_.close();
        return false;
    }
    return true;
}

void CliApp::viewCatalogue()
{
    std::string response;
    if (send("LIST", response)) {
        showResponse(response);
    }
}

void CliApp::searchBooks()
{
    std::cout << "\nSearch by:\n";
    std::cout << "1. ISBN\n";
    std::cout << "2. Title\n";
    std::cout << "3. Author\n";
    std::cout << "4. Back\n\n";

    int choice = 0;
    if (!readMenuChoice(1, 4, choice) || choice == 4) {
        return;
    }

    std::string query;
    if (!readRequiredLine("Enter search text: ", query)) {
        return;
    }

    std::string field = "ISBN";
    if (choice == 2) {
        field = "TITLE";
    } else if (choice == 3) {
        field = "AUTHOR";
    }

    std::string response;
    if (send("SEARCH|" + field + "|" + query, response)) {
        showResponse(response);
    }
}

void CliApp::checkAvailability()
{
    int bookId = 0;
    if (!readRequiredInt("Enter book ID: ", bookId)) {
        return;
    }

    std::string response;
    if (send("AVAILABILITY|" + std::to_string(bookId), response)) {
        showResponse(response);
    }
}

void CliApp::sortCatalogue()
{
    std::cout << "\nSort by:\n";
    std::cout << "1. Title\n";
    std::cout << "2. Author\n";
    std::cout << "3. Year\n";
    std::cout << "4. ISBN\n";
    std::cout << "5. Back\n\n";

    int choice = 0;
    if (!readMenuChoice(1, 5, choice) || choice == 5) {
        return;
    }

    const char* field = "TITLE";
    if (choice == 2) {
        field = "AUTHOR";
    } else if (choice == 3) {
        field = "YEAR";
    } else if (choice == 4) {
        field = "ISBN";
    }

    std::string response;
    if (send(std::string("SORT|") + field, response)) {
        std::cout << "Catalogue sorted.\n";
        showResponse(response);
    }
}

void CliApp::addBook()
{
    int id = 0;
    int year = 0;
    int totalCopies = 0;
    int availableCopies = 0;
    int supplierId = 0;
    std::string isbn;
    std::string title;
    std::string author;
    if (!readRequiredInt("Enter book ID: ", id) || !readRequiredLine("Enter ISBN: ", isbn) ||
        !readRequiredLine("Enter title: ", title) || !readRequiredLine("Enter author: ", author) ||
        !readRequiredInt("Enter year: ", year) || !readRequiredInt("Enter total copies: ", totalCopies) ||
        !readRequiredInt("Enter available copies: ", availableCopies) ||
        !readRequiredInt("Enter supplier ID (0 for none): ", supplierId)) {
        return;
    }

    std::ostringstream command;
    command << "ADD_BOOK|" << id << '|' << isbn << '|' << title << '|' << author << '|' << year << '|'
            << totalCopies << '|' << availableCopies << '|' << supplierId;
    std::string response;
    if (send(command.str(), response)) {
        showResponse(response);
    }
}

void CliApp::editBook()
{
    int id = 0;
    int year = 0;
    int totalCopies = 0;
    int availableCopies = 0;
    int supplierId = 0;
    std::string isbn;
    std::string title;
    std::string author;
    if (!readRequiredInt("Enter book ID: ", id) || !readRequiredLine("Enter ISBN: ", isbn) ||
        !readRequiredLine("Enter title: ", title) || !readRequiredLine("Enter author: ", author) ||
        !readRequiredInt("Enter year: ", year) || !readRequiredInt("Enter total copies: ", totalCopies) ||
        !readRequiredInt("Enter available copies: ", availableCopies) ||
        !readRequiredInt("Enter supplier ID (0 for none): ", supplierId)) {
        return;
    }

    std::ostringstream command;
    command << "EDIT_BOOK|" << id << '|' << isbn << '|' << title << '|' << author << '|' << year << '|'
            << totalCopies << '|' << availableCopies << '|' << supplierId;
    std::string response;
    if (send(command.str(), response)) {
        showResponse(response);
    }
}

void CliApp::removeBook()
{
    int bookId = 0;
    if (!readRequiredInt("Enter book ID: ", bookId)) {
        return;
    }

    std::string response;
    if (send("REMOVE_BOOK|" + std::to_string(bookId), response)) {
        showResponse(response);
    }
}

void CliApp::viewSuppliers()
{
    std::string response;
    if (send("LIST_SUPPLIERS", response)) {
        showResponse(response);
    }
}

void CliApp::addSupplier()
{
    int id = 0;
    std::string name;
    std::string contact;
    if (!readRequiredInt("Enter supplier ID: ", id) || !readRequiredLine("Enter name: ", name) ||
        !readRequiredLine("Enter contact: ", contact)) {
        return;
    }

    std::string response;
    if (send("ADD_SUPPLIER|" + std::to_string(id) + "|" + name + "|" + contact, response)) {
        showResponse(response);
    }
}

void CliApp::editSupplier()
{
    int id = 0;
    std::string name;
    std::string contact;
    if (!readRequiredInt("Enter supplier ID: ", id) || !readRequiredLine("Enter name: ", name) ||
        !readRequiredLine("Enter contact: ", contact)) {
        return;
    }

    std::string response;
    if (send("EDIT_SUPPLIER|" + std::to_string(id) + "|" + name + "|" + contact, response)) {
        showResponse(response);
    }
}

void CliApp::removeSupplier()
{
    int id = 0;
    if (!readRequiredInt("Enter supplier ID: ", id)) {
        return;
    }

    std::string response;
    if (send("REMOVE_SUPPLIER|" + std::to_string(id), response)) {
        showResponse(response);
    }
}

void CliApp::recordStockMovement()
{
    std::cout << "\nStock movement:\n";
    std::cout << "1. Acquire\n";
    std::cout << "2. Borrow\n";
    std::cout << "3. Return\n";
    std::cout << "4. Write off\n";
    std::cout << "5. Back\n\n";

    int choice = 0;
    if (!readMenuChoice(1, 5, choice) || choice == 5) {
        return;
    }

    const char* type = "ACQUIRE";
    if (choice == 2) {
        type = "BORROW";
    } else if (choice == 3) {
        type = "RETURN";
    } else if (choice == 4) {
        type = "WRITEOFF";
    }

    int bookId = 0;
    int quantity = 0;
    if (!readRequiredInt("Enter book ID: ", bookId) || !readRequiredInt("Enter quantity: ", quantity)) {
        return;
    }

    std::string response;
    if (send(std::string("STOCK|") + std::to_string(bookId) + "|" + type + "|" + std::to_string(quantity),
             response)) {
        showResponse(response);
    }
}

void CliApp::viewTransactions()
{
    std::string response;
    if (send("LIST_TRANSACTIONS", response)) {
        showResponse(response);
    }
}

void CliApp::generateReport()
{
    std::string response;
    if (send("REPORT", response)) {
        showResponse(response);
    }
}

void CliApp::saveInventory()
{
    std::string response;
    if (send("SAVE", response)) {
        showResponse(response);
    }
}

void CliApp::userLoop()
{
    while (true) {
        printUserMenu();
        int choice = 0;
        if (!readMenuChoice(1, 5, choice)) {
            disconnect();
            return;
        }
        if (choice == 1) {
            viewCatalogue();
        } else if (choice == 2) {
            searchBooks();
        } else if (choice == 3) {
            checkAvailability();
        } else if (choice == 4) {
            sortCatalogue();
        } else {
            disconnect();
            return;
        }
    }
}

void CliApp::adminLoop()
{
    while (true) {
        printAdminMenu();
        int choice = 0;
        if (!readMenuChoice(1, 16, choice)) {
            disconnect();
            return;
        }
        switch (choice) {
        case 1:
            viewCatalogue();
            break;
        case 2:
            searchBooks();
            break;
        case 3:
            checkAvailability();
            break;
        case 4:
            sortCatalogue();
            break;
        case 5:
            addBook();
            break;
        case 6:
            editBook();
            break;
        case 7:
            removeBook();
            break;
        case 8:
            viewSuppliers();
            break;
        case 9:
            addSupplier();
            break;
        case 10:
            editSupplier();
            break;
        case 11:
            removeSupplier();
            break;
        case 12:
            recordStockMovement();
            break;
        case 13:
            viewTransactions();
            break;
        case 14:
            generateReport();
            break;
        case 15:
            saveInventory();
            break;
        default:
            disconnect();
            return;
        }
    }
}

void CliApp::run()
{
    while (true) {
        printMainMenu();
        int choice = 0;
        if (!readMenuChoice(1, 3, choice)) {
            disconnect();
            return;
        }

        if (choice == 1) {
            if (connectAsUser()) {
                userLoop();
            }
        } else if (choice == 2) {
            if (connectAsAdmin()) {
                adminLoop();
            }
        } else {
            disconnect();
            std::cout << "Goodbye.\n";
            return;
        }
    }
}
