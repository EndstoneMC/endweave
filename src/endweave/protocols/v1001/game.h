#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/game.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::legacy::GameRule_<1001>, bp::GameRule> {
    static void transform(Context<bp::GameRule> &ctx, bp::legacy::GameRule_<1001> &&from);
};

template <>
struct Transformer<bp::LevelSettings_<1001>, bp::LevelSettings_<2168>> {
    static void transform(Context<bp::LevelSettings_<2168>> &ctx, bp::LevelSettings_<1001> &&from);
};

template <>
struct Transformer<bp::BlockEntry, bp::ServerBlockProperty_<2168>> {
    static void transform(Context<bp::ServerBlockProperty_<2168>> &ctx, bp::BlockEntry &&from);
};

template <>
struct Transformer<bp::StartGamePacket_<1001>, bp::StartGamePacket_<2168>> {
    static void transform(Context<bp::StartGamePacket_<2168>> &ctx, bp::StartGamePacket_<1001> &&from);
};

} // namespace endweave
