#ifndef LIMS_NET_SOCKET_HPP
#define LIMS_NET_SOCKET_HPP

#include <cstddef>
#include <cstdint>
#include <string>

class Socket {
public:
    static constexpr std::size_t kMaxLineLength = 8192;

    Socket();
    explicit Socket(std::uintptr_t nativeHandle);
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;
    ~Socket();

    bool listenOn(const std::string& host, unsigned short port, std::string& error);
    bool acceptClient(Socket& client, std::string& error);
    bool connectTo(const std::string& host, unsigned short port, std::string& error);

    bool sendLine(const std::string& line, std::string& error);
    bool receiveLine(std::string& line, std::string& error);

    void close();
    bool isOpen() const;

private:
    std::uintptr_t handle_;
    std::string recvBuffer_;

    void resetHandle();
    bool sendAll(const char* data, std::size_t length, std::string& error);
};

#endif
