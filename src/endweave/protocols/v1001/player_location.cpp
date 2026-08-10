#include "endweave/protocols/v1001/player_location.h"

namespace endweave {

void Transformer<bp::PlayerUpdateEntityOverridesPacket_<1001>, bp::PlayerUpdateEntityOverridesPacket_<2168>>::transform(
    Context<bp::PlayerUpdateEntityOverridesPacket_<2168>> &ctx, bp::PlayerUpdateEntityOverridesPacket_<1001> &&from)
{
    using Update = bp::PlayerUpdateEntityOverridesPacket_<2168>;
    auto &to = ctx.out();
    to.id = from.id;
    to.property_index = from.property_index;
    // ENDWEAVE: 2168 writes the type as the variant tag and again name-coded in the payload; the enumerators
    // run in case order, so the type picks its own case.
    switch (from.update_type) {
    case bp::UpdateType::REMOVE_OVERRIDE:
        to.update = Update::RemoveOverride{from.update_type};
        break;
    case bp::UpdateType::SET_INT_OVERRIDE:
        to.update = Update::IntOverride{from.update_type, from.int_value};
        break;
    case bp::UpdateType::SET_FLOAT_OVERRIDE:
        to.update = Update::FloatOverride{from.update_type, from.float_value};
        break;
    case bp::UpdateType::CLEAR_OVERRIDES:
    default:
        // ENDWEAVE: 1001 reads the type as a raw uint8; a value outside the four has no case and no
        // name 2168 could write, so it clamps to the clear that needs no payload.
        to.update = Update::ClearOverride{bp::UpdateType::CLEAR_OVERRIDES};
        break;
    }
}

void Transformer<bp::PlayerLocationPacket_<1001>, bp::PlayerLocationPacket_<2168>>::transform(
    Context<bp::PlayerLocationPacket_<2168>> &ctx, bp::PlayerLocationPacket_<1001> &&from)
{
    using Location = bp::PlayerLocationPacket_<2168>;
    auto &to = ctx.out();
    to.id = from.id;
    // ENDWEAVE: 2168 moved the type behind the id and writes it as both tag and payload; Coordinates is the
    // case for anything but Hide, and it carries the position.
    if (from.type == bp::PlayerLocationPacket_<1001>::Type::PLAYER_LOCATION_HIDE) {
        to.location = Location::HiddenLocation{from.type};
    }
    else {
        to.location = Location::CoordinatesLocation{from.type, from.pos};
    }
}

} // namespace endweave
