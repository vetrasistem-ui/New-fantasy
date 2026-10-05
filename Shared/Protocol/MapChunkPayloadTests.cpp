#include "Shared/Formats/FMAP/FmapCore.hpp"
#include "Shared/Protocol/MapChunkPayload.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#ifndef FANTASY_REPO_ROOT
#error FANTASY_REPO_ROOT must be defined for MapChunkPayloadTests
#endif

namespace fs = std::filesystem;
namespace fp = fantasy::protocol;
namespace fm = fantasy::fmap;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

} // namespace

int main() {
    try {
        const fs::path repoRoot = fs::weakly_canonical(fs::path(FANTASY_REPO_ROOT));
        const fm::World world = fm::loadFmap(repoRoot / "Game/Maps/World/world.fmap.json");
        require(world.regions.size() == 1, "development world region count mismatch");
        const auto& region = world.regions.front();
        require(region.chunks.size() == 4, "development world chunk count mismatch");

        for (const auto& chunk : region.chunks) {
            const fp::MapChunk message = fp::makeProtocolMapChunk(region, chunk, 7);
            require(message.regionId == region.id, "region id mismatch");
            require(message.regionOriginX == region.origin.x && message.regionOriginY == region.origin.y,
                "region origin mismatch");
            require(message.revision == 7, "revision mismatch");
            require(!message.payload.empty(), "semantic chunk payload cannot be empty");

            const fm::Chunk reopened = fp::decodeFmapChunkPayload(message);
            require(reopened == chunk, "FMAP semantic chunk payload roundtrip mismatch");

            const fp::Frame frame = fp::makeFrame(1, message);
            const fp::MapChunk reopenedMessage = fp::decodeMapChunk(fp::decodeFrame(fp::encodeFrame(frame)));
            require(reopenedMessage.regionOriginX == region.origin.x && reopenedMessage.regionOriginY == region.origin.y,
                "full protocol region origin roundtrip mismatch");
            require(fp::decodeFmapChunkPayload(reopenedMessage) == chunk,
                "full protocol MapChunk roundtrip mismatch");
        }

        fp::MapChunk invalid = fp::makeProtocolMapChunk(region, region.chunks.front());
        invalid.payload.at(0) = 'X';
        bool badMagicRejected = false;
        try { (void)fp::decodeFmapChunkPayload(invalid); } catch (...) { badMagicRejected = true; }
        require(badMagicRejected, "invalid FMCP magic must be rejected");

        fp::MapChunk trailing = fp::makeProtocolMapChunk(region, region.chunks.front());
        trailing.payload.push_back(0xFF);
        bool trailingRejected = false;
        try { (void)fp::decodeFmapChunkPayload(trailing); } catch (...) { trailingRejected = true; }
        require(trailingRejected, "trailing semantic payload bytes must be rejected");

        std::cout << "MapChunkPayloadTests PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "MapChunkPayloadTests FAIL: " << error.what() << '\n';
        return 1;
    }
}
