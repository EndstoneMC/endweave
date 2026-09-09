#include "map.h"

#include <bedrock/protocol/enum.hpp>
#include <utility>

namespace ew = endweave;

namespace endweave {

void Transformer<bp::MapDecoration_<2192>, bp::MapDecoration_<2168>>::transform(Context<bp::MapDecoration_<2168>> &ctx,
                                                                                bp::MapDecoration_<2192> &&from)
{
    using Type = bp::MapDecoration_<2168>::Type;
    auto &to = ctx.out();
    // ENDWEAVE: the five markers 2192 added have no 2168 image, and its Count sits where AbandonedCamp
    // does, so an unknown decoration draws nothing rather than indexing off the end of the atlas.
    to.image = bp::enum_cast<Type>(bp::enum_name(from.image)).value_or(Type::NoDraw);
    to.rotation = from.rotation;
    to.x = from.x;
    to.y = from.y;
    to.label = std::move(from.label);
    to.color = from.color;
}

} // namespace endweave
