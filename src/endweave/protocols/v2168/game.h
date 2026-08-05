#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/game.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::ExperimentToggle, bp::ExperimentData> {
    static bp::ExperimentData transform(bp::ExperimentToggle &&from);
};

template <>
struct Transformer<bp::GameRule, bp::legacy::GameRule_<1001>> {
    static bp::legacy::GameRule_<1001> transform(bp::GameRule &&from);
};

template <>
struct Transformer<bp::LevelSettings_<2168>, bp::LevelSettings_<1001>> {
    static bp::LevelSettings_<1001> transform(bp::LevelSettings_<2168> &&from);
};

template <>
struct Transformer<bp::ServerBlockProperty_<2168>, bp::BlockEntry> {
    static bp::BlockEntry transform(bp::ServerBlockProperty_<2168> &&from);
};

template <>
struct Transformer<bp::StartGamePacket_<2168>, bp::StartGamePacket_<1001>> {
    static bp::StartGamePacket_<1001> transform(bp::StartGamePacket_<2168> &&from);
};

} // namespace endweave
