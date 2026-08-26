#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/game.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::GameRule, bp::legacy::GameRule_<1001>> {
    static void transform(Context<bp::legacy::GameRule_<1001>> &ctx, bp::GameRule &&from);
};

template <>
struct Transformer<bp::LevelSettings_<2168>, bp::LevelSettings_<1001>> {
    static void transform(Context<bp::LevelSettings_<1001>> &ctx, bp::LevelSettings_<2168> &&from);
};

template <>
struct Transformer<bp::ServerBlockProperty_<2168>, bp::BlockEntry> {
    static void transform(Context<bp::BlockEntry> &ctx, bp::ServerBlockProperty_<2168> &&from);
};

template <>
struct Transformer<bp::StartGamePacket_<2168>, bp::StartGamePacket_<1001>> {
    static void transform(Context<bp::StartGamePacket_<1001>> &ctx, bp::StartGamePacket_<2168> &&from);
};

} // namespace endweave
