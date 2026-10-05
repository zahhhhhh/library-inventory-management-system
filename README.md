# Library Inventory Management System (LIMS)

## Project Overview

LIMS is a C++17 client-server command-line application for managing the inventory of a small community library.

Regular users retrieve catalogue information. Administrators perform management operations such as updating books, suppliers and stock. The client communicates with the server over localhost TCP. Inventory data is stored and saved on the server, not on the client.

## Project Status

**Phase II – Implementation**

The client-server prototype is implemented. Inventory operations, LIMS1 persistence, role-based interaction and input/business validation are in place.

## Features

### Regular User

- View catalogue
- Search by ISBN, title or author
- Check availability
- Sort catalogue by title, author, year or ISBN

### Administrator

Administrators have the regular-user functionality plus:

- Add, edit and remove books
- View, add, edit and remove suppliers
- Record ACQUIRE, BORROW, RETURN and WRITEOFF stock movements
- View transaction history
- Generate inventory reports
- Save inventory data

## Architecture

```text
CLI Client
    |
    | TCP 127.0.0.1:4545
    v
LIMS Server
    |
    +-- ClientHandler / Protocol
    |
    +-- InventoryService
    |      |
    |      +-- Book
    |      +-- Supplier
    |      +-- Transaction
    |
    +-- LimsStore
           |
           +-- data/library.lims
```

**CLI Client** collects input and displays results. It does not open or write `library.lims`.

**LIMS Server** accepts connections, manages sessions and enforces USER and ADMIN authorization.

**ClientHandler / Protocol** interprets line-oriented requests and returns responses.

**InventoryService** owns the in-memory inventory and applies business rules.

**LimsStore** loads and saves the LIMS1 persistence file.

**Domain types** are `Book`, `Supplier`, `Transaction` and `TransactionType`.

Administrator authorization is enforced on the server. Hiding options in the client menu is not sufficient to protect management commands.

## Data Model

**Book:** `id`, `isbn`, `title`, `author`, `year`, `totalCopies`, `availableCopies`, `supplierId`

**Supplier:** `id`, `name`, `contact`

**Transaction:** `id`, `bookId`, `type`, `quantity`, `timestamp`

**TransactionType:** `ACQUIRE`, `BORROW`, `RETURN`, `WRITEOFF`

Relationships:

- one Supplier may supply multiple Books
- one Book may have multiple Transactions
- relationships use integer identifiers rather than pointers

## Memory Management

The project uses RAII and value semantics.

Inventory collections are stored in `std::vector`. Text uses `std::string`. The design does not use owning raw pointers or manual `new`/`delete`. The TCP `Socket` wrapper owns its operating-system handle and closes it in the destructor.

## Persistence

Persistence is server-side only. The default file is `data/library.lims` in a UTF-8 LIMS1 text format with labelled sections `[SUPPLIERS]`, `[BOOKS]` and `[TRANSACTIONS]`.

A missing file is treated as an empty inventory, not as a fatal error. Malformed records are rejected. A complete file is validated first; live inventory is replaced only after a successful load.

## Project Structure

```text
library-inventory-management-system/
├── src/
│   ├── client/         # interactive CLI and TCP session
│   ├── server/         # listener, sessions and command handling
│   ├── domain/         # Book, Supplier, Transaction types
│   ├── inventory/      # business rules and in-memory state
│   ├── persistence/    # LIMS1 load and save
│   ├── net/            # RAII TCP socket
│   └── protocol/       # request/response encoding
├── data/
│   └── library.lims
├── CMakeLists.txt
└── README.md
```

The `build/` directory is generated locally and is not source.

## Build

Requirements: CMake 3.16+ and a C++17 compiler (this project is built with MinGW-w64 g++ on Windows).

```text
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

This produces `lims-client` and `lims-server`.

## Running the Application

Start the server first, then the client.

Terminal 1:

```text
build\lims-server.exe
```

Terminal 2:

```text
build\lims-client.exe
```

The client connects to `127.0.0.1:4545`. Exiting the client does not stop the server.

## Roles

**Regular User** may view, search, sort and check availability.

**Administrator** may also change books, suppliers and stock, view history, generate reports and save the inventory.

For this coursework prototype the administrator password is `admin123`. This is a simple demonstration credential only and is not production authentication.

## Validation

Examples of rejected operations:

- duplicate ISBN
- invalid supplier reference
- stock movement that would break available/total copy limits
- malformed numeric protocol fields
- text fields containing reserved protocol delimiters `|` or `~`
- removing a supplier that is still referenced by a book
- removing a book that already has transaction history

Business rules are applied by `InventoryService` on the server.

## Scope

### Implemented Baseline

Phase II provides a localhost client-server CLI, role-based catalogue and administration functions, server-side validation, and LIMS1 persistence.

### Possible Future Extensions

- graphical interface
- database persistence
- stronger authentication
- concurrent clients
- member accounts, reservations and fines
- CSV import/export
