#include "map.h"

#include <bedrock/protocol/enum.hpp>
#include <utility>

namespace ew = endweave;

namespace endweave {
void Transformer<bp::MapDecoration_<2168>, bp::MapDecoration_<2192>>::transform(Context<bp::MapDecoration_<2192>> &ctx,
                                                                                bp::MapDecoration_<2168> &&from)
{
    using Type = bp::MapDecoration_<2192>::Type;
    auto &to = ctx.out();
    // ENDWEAVE: 2192 appended five structure markers ahead of Count, so Count itself is renumbered and
    // passing the byte through would turn it into AbandonedCamp.
    to.image = bp::enum_cast<Type>(bp::enum_name(from.image)).value_or(Type::NoDraw);
    to.rotation = from.rotation;
    to.x = from.x;
    to.y = from.y;
    to.label = std::move(from.label);
    to.color = from.color;
}

} // namespace endweave
