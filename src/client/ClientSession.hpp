#ifndef LIMS_CLIENT_CLIENT_SESSION_HPP
#define LIMS_CLIENT_CLIENT_SESSION_HPP

#include "net/Socket.hpp"

#include <string>

class ClientSession {
public:
    bool connect(const std::string& host, unsigned short port, std::string& error);
    bool request(const std::string& command, std::string& response, std::string& error);
    void close();
    bool isConnected() const;

private:
    Socket socket_;
};

#endif
