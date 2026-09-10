#include "shape.h"

#include <utility>

namespace endweave {
void Transformer<bp::TextDataPayload_<2168>, bp::TextDataPayload_<2192>>::transform(
    Context<bp::TextDataPayload_<2192>> &ctx, bp::TextDataPayload_<2168> &&from)
{
    auto &to = ctx.out();
    to.text = std::move(from.text);
    to.use_rotation = from.use_rotation;
    to.background_color = from.background_color;
    // ENDWEAVE: 2168 sends no line gap, so multi-line text reaches a 2192 client with its lines
    // touching. Nothing on the wire says what the old renderer spaced them by.
    to.line_gap_height = 0.0F;
    to.depth_test = from.depth_test;
    to.show_backface = from.show_backface;
    to.show_text_backface = from.show_text_backface;
}

} // namespace endweave
