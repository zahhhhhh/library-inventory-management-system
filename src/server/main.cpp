#include "server/LimsServer.hpp"

#include <iostream>
#include <string>

int main()
{
    LimsServer server;
    std::string error;
    if (!server.run(error)) {
        std::cerr << "LIMS Server failed: " << error << '\n';
        return 1;
    }

    return 0;
}
