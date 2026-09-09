#include "endweave/protocols/v2168_to_v2192/shape.h"

#include <type_traits>
#include <utility>
#include <variant>

namespace ew = endweave;

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

void Transformer<bp::PrimitiveShapeDataPayload_<2168>, bp::PrimitiveShapeDataPayload_<2192>>::transform(
    Context<bp::PrimitiveShapeDataPayload_<2192>> &ctx, bp::PrimitiveShapeDataPayload_<2168> &&from)
{
    auto &to = ctx.out();
    to.network_id = from.network_id;
    to.shape_type = from.shape_type;
    to.location = from.location;
    to.scale = from.scale;
    to.rotation = from.rotation;
    to.time_left_total_sec = from.time_left_total_sec;
    to.max_render_distance = from.max_render_distance;
    to.color = from.color;
    to.dimension_id = from.dimension_id;
    to.attached_to_id = from.attached_to_id;
    // ENDWEAVE: text is the one arm that moved; the other nine are one type at both versions and the
    // assignment places each back in its own case.
    std::visit(
        [&to, &ctx](auto &alt) {
            if constexpr (std::is_same_v<std::remove_cvref_t<decltype(alt)>, bp::TextDataPayload_<2168>>) {
                to.extra_data_payload = ew::transform_to<bp::TextDataPayload_<2192>>(ctx, std::move(alt));
            }
            else {
                to.extra_data_payload = std::move(alt);
            }
        },
        from.extra_data_payload);
}

} // namespace endweave
