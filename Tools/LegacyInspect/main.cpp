#include "Shared/Assets/Legacy/OtbReader.hpp"
#include "Shared/Assets/Legacy/SprReader.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

namespace {

void printUsage() {
    std::cout << "Usage:\n"
              << "  fantasy-legacy-inspect --spr <file.spr>\n"
              << "  fantasy-legacy-inspect --otb <items.otb>\n"
              << "  fantasy-legacy-inspect --spr <file.spr> --sprite <id>\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 3) {
            printUsage();
            return 2;
        }

        fs::path sprPath;
        fs::path otbPath;
        std::uint32_t spriteId = 0;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--spr" && i + 1 < argc) {
                sprPath = argv[++i];
            } else if (arg == "--otb" && i + 1 < argc) {
                otbPath = argv[++i];
            } else if (arg == "--sprite" && i + 1 < argc) {
                spriteId = static_cast<std::uint32_t>(std::stoul(argv[++i]));
            } else {
                throw std::runtime_error("Unknown or incomplete argument: " + arg);
            }
        }

        if (!sprPath.empty()) {
            const fantasy::assets::legacy::SprReader spr(sprPath);
            std::cout << "SPR signature=0x" << std::hex << spr.info().signature << std::dec
                      << " sprites=" << spr.info().spriteCount << '\n';
            if (spriteId != 0) {
                const auto sprite = spr.readSprite(spriteId);
                std::size_t opaque = 0;
                for (std::size_t i = 3; i < sprite.pixels.size(); i += 4) {
                    if (sprite.pixels[i] != 0) ++opaque;
                }
                std::cout << "sprite=" << spriteId
                          << " present=" << (spr.hasSprite(spriteId) ? "yes" : "no")
                          << " opaque_pixels=" << opaque << '\n';
            }
        }

        if (!otbPath.empty()) {
            const fantasy::assets::legacy::OtbReader otb(otbPath);
            std::size_t mapped = 0;
            for (const auto& item : otb.items()) {
                if (item.serverId.has_value() && item.clientId.has_value()) ++mapped;
            }
            std::cout << "OTB version=" << otb.version().major << '.' << otb.version().minor
                      << '.' << otb.version().build
                      << " description=\"" << otb.version().description << "\""
                      << " nodes=" << otb.items().size()
                      << " mapped_server_client=" << mapped << '\n';
        }

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Fantasy legacy inspect error: " << error.what() << '\n';
        return 1;
    }
}
