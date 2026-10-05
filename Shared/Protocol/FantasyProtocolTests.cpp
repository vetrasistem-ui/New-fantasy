#include "Shared/Protocol/FantasyProtocol.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace fantasy::protocol;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename Message, typename Decode>
void requireRoundtrip(std::uint32_t sequence, const Message& expected, Decode decode) {
    const Frame source = makeFrame(sequence, expected);
    const Bytes wire = encodeFrame(source);
    const Frame reopened = decodeFrame(wire);
    require(reopened == source, "frame roundtrip mismatch");
    require(decode(reopened) == expected, "message roundtrip mismatch");
}

} // namespace

int main() {
    try {
        const Hello hello{0x11223344u, 1};
        const Frame helloFrame = makeFrame(0xA1B2C3D4u, hello);
        const Bytes wire = encodeFrame(helloFrame);

        require(wire.size() == kEnvelopeBytes + 6, "Hello wire size mismatch");
        require(wire[0] == 'F' && wire[1] == 'N' && wire[2] == 'T' && wire[3] == 'Y', "magic mismatch");
        require(wire[4] == 0x01 && wire[5] == 0x00, "protocol version is not little-endian uint16");
        require(wire[6] == 0x01 && wire[7] == 0x00, "Hello message type is not little-endian uint16");
        require(wire[8] == 0x06 && wire[9] == 0x00 && wire[10] == 0x00 && wire[11] == 0x00,
            "payload length is not little-endian uint32");
        require(wire[12] == 0xD4 && wire[13] == 0xC3 && wire[14] == 0xB2 && wire[15] == 0xA1,
            "sequence is not little-endian uint32");

        require(!expectedFrameSize(std::span<const std::uint8_t>(wire.data(), 15)).has_value(),
            "partial envelope should not expose frame size");
        require(expectedFrameSize(wire).value() == wire.size(), "expected frame size mismatch");
        require(decodeFrame(wire) == helloFrame, "Hello frame decode mismatch");
        require(decodeHello(decodeFrame(wire)) == hello, "Hello payload decode mismatch");

        requireRoundtrip(2, HelloAck{77, 1}, decodeHelloAck);
        requireRoundtrip(3, LoginDev{"Development Hero"}, decodeLoginDev);
        requireRoundtrip(4, LoginOk{0x0102030405060708ull}, decodeLoginOk);
        requireRoundtrip(5, EnterWorld{42, -120, 345, 7}, decodeEnterWorld);
        requireRoundtrip(6, MapChunk{"development", 96, 96, 4, 0, 7, 12, Bytes{1, 2, 3, 4, 5}}, decodeMapChunk);
        requireRoundtrip(7, MoveRequest{MoveDirection::East}, decodeMoveRequest);
        requireRoundtrip(8, EntityAdd{42, "player", 100, 100, 7}, decodeEntityAdd);
        requireRoundtrip(9, EntityMove{42, 101, 99, 7, MoveDirection::East}, decodeEntityMove);
        requireRoundtrip(10, EntityRemove{42}, decodeEntityRemove);
        requireRoundtrip(11, ErrorMessage{9001, "development error"}, decodeError);
        requireRoundtrip(12, Disconnect{"test complete"}, decodeDisconnect);

        const Frame moveIntent = makeFrame(13, MoveRequest{MoveDirection::North});
        require(moveIntent.payload.size() == 1, "MoveRequest must carry direction intent only");

        Bytes badMagic = wire;
        badMagic[0] = 'X';
        bool badMagicRejected = false;
        try { (void)decodeFrame(badMagic); } catch (...) { badMagicRejected = true; }
        require(badMagicRejected, "bad magic must be rejected");

        Bytes badVersion = wire;
        badVersion[4] = 2;
        badVersion[5] = 0;
        bool badVersionRejected = false;
        try { (void)decodeFrame(badVersion); } catch (...) { badVersionRejected = true; }
        require(badVersionRejected, "unsupported protocol version must be rejected");

        Bytes unknownType = wire;
        unknownType[6] = 0xEE;
        unknownType[7] = 0x7F;
        bool unknownTypeRejected = false;
        try { (void)decodeFrame(unknownType); } catch (...) { unknownTypeRejected = true; }
        require(unknownTypeRejected, "unknown message type must be rejected");

        Bytes truncated = wire;
        truncated.pop_back();
        bool truncatedRejected = false;
        try { (void)decodeFrame(truncated); } catch (...) { truncatedRejected = true; }
        require(truncatedRejected, "truncated frame must be rejected");

        Frame oversized;
        oversized.messageType = MessageType::Hello;
        oversized.payload.resize(static_cast<std::size_t>(kMaxPayloadBytes) + 1u);
        bool oversizedRejected = false;
        try { (void)encodeFrame(oversized); } catch (...) { oversizedRejected = true; }
        require(oversizedRejected, "oversized payload must be rejected");

        Frame invalidDirection = makeFrame(14, MoveRequest{MoveDirection::North});
        invalidDirection.payload[0] = 255;
        bool invalidDirectionRejected = false;
        try { (void)decodeMoveRequest(invalidDirection); } catch (...) { invalidDirectionRejected = true; }
        require(invalidDirectionRejected, "invalid move direction must be rejected");

        Frame trailing = makeFrame(15, LoginDev{"Hero"});
        trailing.payload.push_back(0xFF);
        bool trailingRejected = false;
        try { (void)decodeLoginDev(trailing); } catch (...) { trailingRejected = true; }
        require(trailingRejected, "typed payload trailing bytes must be rejected");

        std::cout << "FantasyProtocolTests PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FantasyProtocolTests FAIL: " << error.what() << '\n';
        return 1;
    }
}
