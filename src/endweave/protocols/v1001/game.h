#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/game.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::legacy::GameRule_<1001>, bp::GameRule> {
    static bp::GameRule transform(bp::legacy::GameRule_<1001> &&from);
};

template <>
struct Transformer<bp::LevelSettings_<1001>, bp::LevelSettings_<2168>> {
    static bp::LevelSettings_<2168> transform(bp::LevelSettings_<1001> &&from);
};

template <>
struct Transformer<bp::BlockEntry, bp::ServerBlockProperty_<2168>> {
    static bp::ServerBlockProperty_<2168> transform(bp::BlockEntry &&from);
};

template <>
struct Transformer<bp::StartGamePacket_<1001>, bp::StartGamePacket_<2168>> {
    static bp::StartGamePacket_<2168> transform(bp::StartGamePacket_<1001> &&from);
};

} // namespace endweave
