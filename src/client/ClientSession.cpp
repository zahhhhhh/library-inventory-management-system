#include "client/ClientSession.hpp"

bool ClientSession::connect(const std::string& host, unsigned short port, std::string& error)
{
    return socket_.connectTo(host, port, error);
}

bool ClientSession::request(const std::string& command, std::string& response, std::string& error)
{
    if (!socket_.sendLine(command, error)) {
        return false;
    }
    return socket_.receiveLine(response, error);
}

void ClientSession::close()
{
    socket_.close();
}

bool ClientSession::isConnected() const
{
    return socket_.isOpen();
}
