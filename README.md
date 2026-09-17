# Library Inventory Management System (LIMS)

## Project Overview

The Library Inventory Management System (LIMS) is a C++ command-line application designed to manage the inventory of a small community library.

The system will maintain information about books, physical copy quantities, suppliers, and inventory transactions. 

## Project Status

**Phase 1 – Conception**

The project is currently in the conceptual design and initial build-skeleton stage. Core functionality will be implemented during Phase 2 following feedback on the Phase 1 design.

## Planned Features

The baseline application will support:

- Adding, editing, and removing books
- Recording stock movements:
  - Acquire
  - Borrow
  - Return
  - Write-off
- Searching books by ISBN, title, or author
- Sorting the catalogue
- Generating inventory reports
- Maintaining supplier reference data
- Recording transaction history
- Saving inventory data to disk
- Loading inventory data when the application starts
- Input validation and error handling

## Data Model

The application will use three main entities:

- **Book** – represents a title and its inventory quantities
- **Supplier** – represents supplier reference information
- **Transaction** – records stock movements affecting a book

Relationships between records will use integer identifiers rather than raw pointers.

## Architecture

The application is planned around three main layers:

1. **Interface Layer** – command-line menu, user input, and output
2. **Logic Layer** – validation, inventory operations, searching, sorting, and reporting
3. **Data Layer** – entities, in-memory collections, and persistence

Persistent data will be stored in a designed text format called **LIMS1** using a local `library.lims` file.

## Memory Management

The project will use RAII and value semantics.

Entities will be stored using standard C++ containers such as `std::vector`, while strings will use `std::string`. The baseline design does not require owning raw pointers or manual `new` and `delete`.

## Toolchain

- **Language:** C++17
- **Compiler:** g++
- **Build system:** CMake 3.16+
- **Version control:** Git
- **Repository:** GitHub

## Project Structure

```text
library-inventory-management-system/
├── src/
│   └── main.cpp
├── .gitignore
├── CMakeLists.txt
└── README.md
