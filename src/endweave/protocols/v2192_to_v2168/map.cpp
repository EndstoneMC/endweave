#include "map.h"

#include <bedrock/protocol/enum.hpp>
#include <utility>

namespace ew = endweave;

namespace endweave {

void Transformer<bp::MapDecoration_<2192>, bp::MapDecoration_<2168>>::transform(Context<bp::MapDecoration_<2168>> &ctx,
                                                                                bp::MapDecoration_<2192> &&from)
{
    using Type = bp::MapDecoration_<2168>::Type;
    using NewType = bp::MapDecoration_<2192>::Type;
    auto &to = ctx.out();
    // ENDWEAVE: the five markers 2192 added have no 2168 image, so each borrows the closest-looking one.
    switch (from.image) {
    case NewType::AbandonedCamp:
        to.image = Type::WitchHut;
        break;
    case NewType::BuriedAncientCity:
    case NewType::BuriedMineshaft:
        to.image = Type::TrialChambers;
        break;
    case NewType::DesertPyramid:
        to.image = Type::JungleTemple;
        break;
    case NewType::WarmOceanRuins:
        to.image = Type::Monument;
        break;
    default:
        to.image = bp::enum_cast<Type>(bp::enum_name(from.image)).value_or(Type::NoDraw);
        break;
    }
    to.rotation = from.rotation;
    to.x = from.x;
    to.y = from.y;
    to.label = std::move(from.label);
    to.color = from.color;
}

} // namespace endweave
