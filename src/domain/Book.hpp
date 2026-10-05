#ifndef LIMS_DOMAIN_BOOK_HPP
#define LIMS_DOMAIN_BOOK_HPP

#include <string>

struct Book {
    int id = 0;
    std::string isbn;
    std::string title;
    std::string author;
    int year = 0;
    int totalCopies = 0;
    int availableCopies = 0;
    int supplierId = 0;
};

#endif
