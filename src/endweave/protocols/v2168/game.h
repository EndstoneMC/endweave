#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/game.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::ExperimentToggle> {
    static bp::ExperimentData downgrade(bp::ExperimentToggle &&from);
};

template <>
struct Transformer<bp::LevelSettings_<2168>> {
    static bp::LevelSettings_<1001> downgrade(bp::LevelSettings_<2168> &&from);
};

template <>
struct Transformer<bp::ServerBlockProperty_<2168>> {
    static bp::BlockEntry downgrade(bp::ServerBlockProperty_<2168> &&from);
};

template <>
struct Transformer<bp::StartGamePacket_<2168>> {
    static bp::StartGamePacket_<1001> downgrade(bp::StartGamePacket_<2168> &&from);
};

} // namespace endweave
