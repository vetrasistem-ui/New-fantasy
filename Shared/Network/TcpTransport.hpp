#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace fantasy::net {

class TcpListener;

class TcpStream {
public:
    TcpStream() = default;
    ~TcpStream();

    TcpStream(const TcpStream&) = delete;
    TcpStream& operator=(const TcpStream&) = delete;
    TcpStream(TcpStream&& other) noexcept;
    TcpStream& operator=(TcpStream&& other) noexcept;

    static TcpStream connectIpv4(const std::string& host, std::uint16_t port);

    bool valid() const;
    void close();
    void sendAll(std::span<const std::uint8_t> bytes);
    std::vector<std::uint8_t> receiveExact(std::size_t size);

private:
    explicit TcpStream(std::uintptr_t nativeHandle);
    static constexpr std::uintptr_t invalidHandle() { return static_cast<std::uintptr_t>(-1); }

    std::uintptr_t handle_ = invalidHandle();
    friend class TcpListener;
};

class TcpListener {
public:
    TcpListener() = default;
    ~TcpListener();

    TcpListener(const TcpListener&) = delete;
    TcpListener& operator=(const TcpListener&) = delete;
    TcpListener(TcpListener&& other) noexcept;
    TcpListener& operator=(TcpListener&& other) noexcept;

    // Binds 127.0.0.1. Port 0 asks the OS for an ephemeral port.
    static TcpListener listenLoopback(std::uint16_t port, int backlog = 8);

    bool valid() const;
    std::uint16_t localPort() const;
    TcpStream acceptOne();
    void close();

private:
    explicit TcpListener(std::uintptr_t nativeHandle);
    static constexpr std::uintptr_t invalidHandle() { return static_cast<std::uintptr_t>(-1); }

    std::uintptr_t handle_ = invalidHandle();
};

} // namespace fantasy::net
