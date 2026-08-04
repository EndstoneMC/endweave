#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/game.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::ExperimentData> {
    static bp::ExperimentToggle upgrade(bp::ExperimentData &&from);
};

template <>
struct Transformer<bp::GameRule_<1001>> {
    static bp::GameRule_<2168> upgrade(bp::GameRule_<1001> &&from);
};

template <>
struct Transformer<bp::LevelSettings_<1001>> {
    static bp::LevelSettings_<2168> upgrade(bp::LevelSettings_<1001> &&from);
};

template <>
struct Transformer<bp::BlockEntry> {
    static bp::ServerBlockProperty_<2168> upgrade(bp::BlockEntry &&from);
};

template <>
struct Transformer<bp::StartGamePacket_<1001>> {
    static bp::StartGamePacket_<2168> upgrade(bp::StartGamePacket_<1001> &&from);
};

} // namespace endweave
