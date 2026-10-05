#include "Shared/Network/TcpTransport.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

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
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace fantasy::net {
namespace {

#ifdef _WIN32
using NativeSocket = SOCKET;

class SocketRuntime {
public:
    SocketRuntime() {
        WSADATA data{};
        const int result = WSAStartup(MAKEWORD(2, 2), &data);
        if (result != 0) throw std::runtime_error("WSAStartup failed: " + std::to_string(result));
    }
    ~SocketRuntime() { WSACleanup(); }
};

void ensureSocketRuntime() {
    static SocketRuntime runtime;
    (void)runtime;
}

NativeSocket asNative(std::uintptr_t handle) { return static_cast<SOCKET>(handle); }
std::uintptr_t fromNative(NativeSocket handle) { return static_cast<std::uintptr_t>(handle); }

std::string socketError(const std::string& operation) {
    return operation + " failed with WSA error " + std::to_string(WSAGetLastError());
}

void closeNative(NativeSocket handle) {
    if (handle != INVALID_SOCKET) closesocket(handle);
}
#else
using NativeSocket = int;

void ensureSocketRuntime() {}
NativeSocket asNative(std::uintptr_t handle) { return static_cast<int>(handle); }
std::uintptr_t fromNative(NativeSocket handle) { return static_cast<std::uintptr_t>(handle); }

std::string socketError(const std::string& operation) {
    return operation + " failed: " + std::string(std::strerror(errno));
}

void closeNative(NativeSocket handle) {
    if (handle >= 0) ::close(handle);
}
#endif

sockaddr_in loopbackAddress(std::uint16_t port) {
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    return address;
}

int boundedIoRequest(std::size_t remaining) {
    const auto maxInt = static_cast<std::size_t>(std::numeric_limits<int>::max());
    return static_cast<int>((remaining < maxInt) ? remaining : maxInt);
}

} // namespace

TcpStream::TcpStream(std::uintptr_t nativeHandle) : handle_(nativeHandle) {}

TcpStream::~TcpStream() {
    close();
}

TcpStream::TcpStream(TcpStream&& other) noexcept : handle_(std::exchange(other.handle_, invalidHandle())) {}

TcpStream& TcpStream::operator=(TcpStream&& other) noexcept {
    if (this != &other) {
        close();
        handle_ = std::exchange(other.handle_, invalidHandle());
    }
    return *this;
}

TcpStream TcpStream::connectIpv4(const std::string& requestedHost, std::uint16_t port) {
    ensureSocketRuntime();
    if (port == 0) throw std::runtime_error("TCP connect port cannot be zero");

    const std::string host = requestedHost == "localhost" ? "127.0.0.1" : requestedHost;
    const NativeSocket socketHandle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef _WIN32
    if (socketHandle == INVALID_SOCKET) throw std::runtime_error(socketError("socket"));
#else
    if (socketHandle < 0) throw std::runtime_error(socketError("socket"));
#endif

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1) {
        closeNative(socketHandle);
        throw std::runtime_error("TCP transport currently requires an IPv4 address or localhost");
    }

    if (::connect(socketHandle, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
        const auto message = socketError("connect");
        closeNative(socketHandle);
        throw std::runtime_error(message);
    }

    return TcpStream(fromNative(socketHandle));
}

bool TcpStream::valid() const {
    return handle_ != invalidHandle();
}

void TcpStream::close() {
    if (!valid()) return;
    closeNative(asNative(handle_));
    handle_ = invalidHandle();
}

void TcpStream::sendAll(std::span<const std::uint8_t> bytes) {
    if (!valid()) throw std::runtime_error("sendAll called on closed TCP stream");
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const std::size_t remaining = bytes.size() - offset;
        const int request = boundedIoRequest(remaining);
#ifdef _WIN32
        const int sent = ::send(asNative(handle_), reinterpret_cast<const char*>(bytes.data() + offset), request, 0);
        if (sent == SOCKET_ERROR) throw std::runtime_error(socketError("send"));
#else
        const int sent = static_cast<int>(::send(asNative(handle_), bytes.data() + offset, static_cast<std::size_t>(request), 0));
        if (sent < 0) throw std::runtime_error(socketError("send"));
#endif
        if (sent == 0) throw std::runtime_error("TCP peer closed during send");
        offset += static_cast<std::size_t>(sent);
    }
}

std::vector<std::uint8_t> TcpStream::receiveExact(std::size_t size) {
    if (!valid()) throw std::runtime_error("receiveExact called on closed TCP stream");
    std::vector<std::uint8_t> result(size);
    std::size_t offset = 0;
    while (offset < size) {
        const std::size_t remaining = size - offset;
        const int request = boundedIoRequest(remaining);
#ifdef _WIN32
        const int received = ::recv(asNative(handle_), reinterpret_cast<char*>(result.data() + offset), request, 0);
        if (received == SOCKET_ERROR) throw std::runtime_error(socketError("recv"));
#else
        const int received = static_cast<int>(::recv(asNative(handle_), result.data() + offset, static_cast<std::size_t>(request), 0));
        if (received < 0) throw std::runtime_error(socketError("recv"));
#endif
        if (received == 0) throw std::runtime_error("TCP peer closed during receive");
        offset += static_cast<std::size_t>(received);
    }
    return result;
}

TcpListener::TcpListener(std::uintptr_t nativeHandle) : handle_(nativeHandle) {}

TcpListener::~TcpListener() {
    close();
}

TcpListener::TcpListener(TcpListener&& other) noexcept : handle_(std::exchange(other.handle_, invalidHandle())) {}

TcpListener& TcpListener::operator=(TcpListener&& other) noexcept {
    if (this != &other) {
        close();
        handle_ = std::exchange(other.handle_, invalidHandle());
    }
    return *this;
}

TcpListener TcpListener::listenLoopback(std::uint16_t port, int backlog) {
    ensureSocketRuntime();
    if (backlog <= 0) throw std::runtime_error("TCP listener backlog must be positive");

    const NativeSocket socketHandle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef _WIN32
    if (socketHandle == INVALID_SOCKET) throw std::runtime_error(socketError("socket"));
#else
    if (socketHandle < 0) throw std::runtime_error(socketError("socket"));
#endif

    int reuse = 1;
#ifdef _WIN32
    (void)setsockopt(socketHandle, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
#else
    (void)setsockopt(socketHandle, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif

    const sockaddr_in address = loopbackAddress(port);
    if (::bind(socketHandle, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
        const auto message = socketError("bind");
        closeNative(socketHandle);
        throw std::runtime_error(message);
    }
    if (::listen(socketHandle, backlog) != 0) {
        const auto message = socketError("listen");
        closeNative(socketHandle);
        throw std::runtime_error(message);
    }

    return TcpListener(fromNative(socketHandle));
}

bool TcpListener::valid() const {
    return handle_ != invalidHandle();
}

std::uint16_t TcpListener::localPort() const {
    if (!valid()) throw std::runtime_error("localPort called on closed TCP listener");
    sockaddr_in address{};
#ifdef _WIN32
    int length = sizeof(address);
#else
    socklen_t length = sizeof(address);
#endif
    if (::getsockname(asNative(handle_), reinterpret_cast<sockaddr*>(&address), &length) != 0) {
        throw std::runtime_error(socketError("getsockname"));
    }
    return ntohs(address.sin_port);
}

TcpStream TcpListener::acceptOne() {
    if (!valid()) throw std::runtime_error("acceptOne called on closed TCP listener");
    const NativeSocket accepted = ::accept(asNative(handle_), nullptr, nullptr);
#ifdef _WIN32
    if (accepted == INVALID_SOCKET) throw std::runtime_error(socketError("accept"));
#else
    if (accepted < 0) throw std::runtime_error(socketError("accept"));
#endif
    return TcpStream(fromNative(accepted));
}

void TcpListener::close() {
    if (!valid()) return;
    closeNative(asNative(handle_));
    handle_ = invalidHandle();
}

} // namespace fantasy::net
