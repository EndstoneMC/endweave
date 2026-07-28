#pragma once

#include <string_view>

namespace endweave {

/**
 * Why a packet stopped part way through a pipeline. Cancelled is the ordinary one and means a
 * handler dropped the packet on purpose.
 *
 * @note endweave-specific: endweave returns std::expected where ViaVersion throws, so what
 * ViaVersion raises out of a handler is an error value here.
 * @see ViaVersion CancelException (Cancelled) and InformativeException (the rest).
 */
enum class PacketError {
    Cancelled,
    Malformed,
    TrailingBytes,
    NoConverter,
};

/**
 * @return What to put in the log line for an error.
 * @see ViaVersion AbstractProtocol#printRemapError, which prints the exception instead.
 */
constexpr std::string_view describe(PacketError error)
{
    switch (error) {
    case PacketError::Cancelled:
        return "cancelled by a handler";
    case PacketError::Malformed:
        return "the body does not match the shape this version declares";
    case PacketError::TrailingBytes:
        return "the body carries bytes the shape does not read";
    case PacketError::NoConverter:
        return "no converter for the shape the packet arrived in";
    }
    return "unknown";
}

} // namespace endweave
