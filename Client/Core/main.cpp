#include "Core/DevelopmentClient.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fp = fantasy::protocol;

namespace {

std::uint16_t parsePort(const std::string& value) {
    const int parsed = std::stoi(value);
    if (parsed < 1 || parsed > 65535) throw std::runtime_error("port must be between 1 and 65535");
    return static_cast<std::uint16_t>(parsed);
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 1) {
            std::cout << "Fantasy Client 0.1.0\n";
            std::cout << "usage: fantasy-client --connect <ipv4|localhost> <port> [character]\n";
            return 0;
        }

        if (argc < 4 || std::string(argv[1]) != "--connect") {
            throw std::runtime_error("usage: fantasy-client --connect <ipv4|localhost> <port> [character]");
        }

        const std::string host = argv[2];
        const std::uint16_t port = parsePort(argv[3]);
        const std::string character = argc >= 5 ? argv[4] : "Development Hero";

        auto client = fantasy::client::DevelopmentClient::connectIpv4(host, port, 1);
        client.handshake();
        client.login(character);

        std::size_t tileCount = 0;
        for (const auto& received : client.chunks()) tileCount += received.chunk.tiles.size();

        std::cout << "state=IN_WORLD\n";
        std::cout << "entity=" << client.entityId().value() << '\n';
        std::cout << "position=" << client.position().x << ',' << client.position().y << ',' << client.position().z << '\n';
        std::cout << "chunks=" << client.chunks().size() << '\n';
        std::cout << "tiles=" << tileCount << '\n';

        const auto moved = client.move(fp::MoveDirection::East);
        std::cout << "moved=" << moved.x << ',' << moved.y << ',' << moved.z << '\n';

        client.disconnect("headless client complete");
        std::cout << "state=DISCONNECTED\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Fantasy Client error: " << error.what() << '\n';
        return 1;
    }
}
