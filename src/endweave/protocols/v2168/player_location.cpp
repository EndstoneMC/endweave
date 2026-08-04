#include "endweave/protocols/v2168/player_location.h"

#include <variant>

namespace endweave {

bp::PlayerUpdateEntityOverridesPacket_<1001> Transformer<bp::PlayerUpdateEntityOverridesPacket_<2168>>::downgrade(
    bp::PlayerUpdateEntityOverridesPacket_<2168> &&from)
{
    using Update = bp::PlayerUpdateEntityOverridesPacket_<2168>;
    bp::PlayerUpdateEntityOverridesPacket_<1001> to;
    to.id = from.id;
    to.property_index = from.property_index;
    // ENDWEAVE: 2168 writes the type as the variant tag and again name-coded in the payload; the payload is
    // what the handler reads, so 1001's single type field comes from there.
    std::visit(
        [&to](const auto &update) {
            to.update_type = update.update_type;
        },
        from.update);
    // ENDWEAVE: 1001 reads a value only for the int and float types; the other two leave both untouched.
    if (const auto *int_override = std::get_if<Update::IntOverride>(&from.update)) {
        to.int_value = int_override->value;
    }
    if (const auto *float_override = std::get_if<Update::FloatOverride>(&from.update)) {
        to.float_value = float_override->value;
    }
    return to;
}

bp::PlayerLocationPacket_<1001> Transformer<bp::PlayerLocationPacket_<2168>>::downgrade(
    bp::PlayerLocationPacket_<2168> &&from)
{
    using Location = bp::PlayerLocationPacket_<2168>;
    bp::PlayerLocationPacket_<1001> to;
    // ENDWEAVE: 1001 leads with the type, 2168 with the id and writes the type as both tag and payload; the
    // payload is what the handler reads, so the type comes from there.
    std::visit(
        [&to](const auto &location) {
            to.type = location.packet_type;
        },
        from.location);
    to.id = from.id;
    // ENDWEAVE: 1001 reads a position only for the coordinates type; Hidden leaves it unwritten.
    if (const auto *coordinates = std::get_if<Location::CoordinatesLocation>(&from.location)) {
        to.pos = coordinates->pos;
    }
    return to;
}

} // namespace endweave
