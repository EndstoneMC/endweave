#include "endweave/protocols/v2192_to_v2168/diagnostics.h"

#include <bedrock/protocol/enum.hpp>
#include <utility>

namespace ew = endweave;

namespace endweave {

void Transformer<bp::MemoryCategoryCounter_<2192>, bp::MemoryCategoryCounter_<2168>>::transform(
    Context<bp::MemoryCategoryCounter_<2168>> &ctx, bp::MemoryCategoryCounter_<2192> &&from)
{
    auto &to = ctx.out();
    // ENDWEAVE: 2192 dropped Persona_Textures and shifted every category above it down one, so the name
    // carries the meaning, not the byte.
    to.category = bp::enum_cast<bp::MemoryCategory_<2168>>(bp::enum_name(from.category))
                      .value_or(bp::MemoryCategory_<2168>::Unknown);
    to.current_bytes = from.current_bytes;
}

} // namespace endweave
