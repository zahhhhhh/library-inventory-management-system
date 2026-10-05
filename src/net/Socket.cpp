#include "net/Socket.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <utility>

namespace {

#ifdef _WIN32
using NativeHandle = SOCKET;
constexpr NativeHandle kInvalidHandle = INVALID_SOCKET;

class WinsockRuntime {
public:
    WinsockRuntime()
    {
        WSADATA data {};
        ready_ = (WSAStartup(MAKEWORD(2, 2), &data) == 0);
    }

    ~WinsockRuntime()
    {
        if (ready_) {
            WSACleanup();
        }
    }

    bool ready() const { return ready_; }

private:
    bool ready_ = false;
};

bool ensureWinsock(std::string& error)
{
    static WinsockRuntime runtime;
    if (!runtime.ready()) {
        error = "Failed to initialise Winsock.";
        return false;
    }
    return true;
}

void closeNative(NativeHandle handle)
{
    if (handle != kInvalidHandle) {
        closesocket(handle);
    }
}
#else
using NativeHandle = int;
constexpr NativeHandle kInvalidHandle = -1;

bool ensureWinsock(std::string&)
{
    return true;
}

void closeNative(NativeHandle handle)
{
    if (handle != kInvalidHandle) {
        ::close(handle);
    }
}
#endif

NativeHandle toNative(std::uintptr_t handle)
{
    return static_cast<NativeHandle>(handle);
}

std::uintptr_t fromNative(NativeHandle handle)
{
    return static_cast<std::uintptr_t>(handle);
}

bool isInvalid(std::uintptr_t handle)
{
    return toNative(handle) == kInvalidHandle;
}

} // namespace

Socket::Socket() : handle_(fromNative(kInvalidHandle)) {}

Socket::Socket(std::uintptr_t nativeHandle) : handle_(nativeHandle) {}

Socket::Socket(Socket&& other) noexcept
    : handle_(other.handle_), recvBuffer_(std::move(other.recvBuffer_))
{
    other.resetHandle();
}

Socket& Socket::operator=(Socket&& other) noexcept
{
    if (this != &other) {
        close();
        handle_ = other.handle_;
        recvBuffer_ = std::move(other.recvBuffer_);
        other.resetHandle();
    }
    return *this;
}

Socket::~Socket()
{
    close();
}

void Socket::resetHandle()
{
    handle_ = fromNative(kInvalidHandle);
    recvBuffer_.clear();
}

void Socket::close()
{
    if (!isInvalid(handle_)) {
        closeNative(toNative(handle_));
        resetHandle();
    } else {
        recvBuffer_.clear();
    }
}

bool Socket::isOpen() const
{
    return !isInvalid(handle_);
}

bool Socket::listenOn(const std::string& host, unsigned short port, std::string& error)
{
    close();
    if (!ensureWinsock(error)) {
        return false;
    }

    const NativeHandle listener = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == kInvalidHandle) {
        error = "Cannot create listening socket.";
        return false;
    }

    const int enable = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&enable), sizeof(enable));

    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1) {
        closeNative(listener);
        error = "Invalid listen address.";
        return false;
    }

    if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        closeNative(listener);
        error = "Cannot bind to " + host + ":" + std::to_string(port) + ".";
        return false;
    }

    if (listen(listener, 8) != 0) {
        closeNative(listener);
        error = "Cannot listen on " + host + ":" + std::to_string(port) + ".";
        return false;
    }

    handle_ = fromNative(listener);
    return true;
}

bool Socket::acceptClient(Socket& client, std::string& error)
{
    if (!isOpen()) {
        error = "Server socket is not listening.";
        return false;
    }

    const NativeHandle accepted = ::accept(toNative(handle_), nullptr, nullptr);
    if (accepted == kInvalidHandle) {
        error = "Failed to accept client connection.";
        return false;
    }

    client = Socket(fromNative(accepted));
    return true;
}

bool Socket::connectTo(const std::string& host, unsigned short port, std::string& error)
{
    close();
    if (!ensureWinsock(error)) {
        return false;
    }

    const NativeHandle connected = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connected == kInvalidHandle) {
        error = "Cannot create client socket.";
        return false;
    }

    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1) {
        closeNative(connected);
        error = "Invalid server address.";
        return false;
    }

    if (connect(connected, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        closeNative(connected);
        error = "Cannot connect to " + host + ":" + std::to_string(port) + ".";
        return false;
    }

    handle_ = fromNative(connected);
    return true;
}

bool Socket::sendAll(const char* data, std::size_t length, std::string& error)
{
    std::size_t sent = 0;
    while (sent < length) {
#ifdef _WIN32
        const int result = ::send(toNative(handle_), data + sent, static_cast<int>(length - sent), 0);
#else
        const ssize_t result = ::send(toNative(handle_), data + sent, length - sent, 0);
#endif
        if (result <= 0) {
            error = "Failed to send data.";
            return false;
        }
        sent += static_cast<std::size_t>(result);
    }
    return true;
}

bool Socket::sendLine(const std::string& line, std::string& error)
{
    if (!isOpen()) {
        error = "Socket is closed.";
        return false;
    }

    std::string payload = line;
    if (payload.empty() || payload.back() != '\n') {
        payload.push_back('\n');
    }

    if (payload.size() > kMaxLineLength + 1) {
        error = "Line too long.";
        return false;
    }

    return sendAll(payload.data(), payload.size(), error);
}

bool Socket::receiveLine(std::string& line, std::string& error)
{
    line.clear();
    if (!isOpen()) {
        error = "Socket is closed.";
        return false;
    }

    while (true) {
        const std::string::size_type newline = recvBuffer_.find('\n');
        if (newline != std::string::npos) {
            line = recvBuffer_.substr(0, newline);
            recvBuffer_.erase(0, newline + 1);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.size() > kMaxLineLength) {
                error = "Line too long.";
                return false;
            }
            return true;
        }

        if (recvBuffer_.size() > kMaxLineLength) {
            error = "Line too long.";
            return false;
        }

        char buffer[1024];
#ifdef _WIN32
        const int received = ::recv(toNative(handle_), buffer, sizeof(buffer), 0);
#else
        const ssize_t received = ::recv(toNative(handle_), buffer, sizeof(buffer), 0);
#endif
        if (received == 0) {
            error = "Connection closed.";
            return false;
        }
        if (received < 0) {
            error = "Failed to receive data.";
            return false;
        }

        recvBuffer_.append(buffer, static_cast<std::size_t>(received));
    }
}
