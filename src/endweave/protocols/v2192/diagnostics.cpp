#include "endweave/protocols/v2192/diagnostics.h"

#include <bedrock/enum.hpp>
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
                      .value_or(bp::MemoryCategory_<2168>::UNKNOWN);
    to.current_bytes = from.current_bytes;
}

void Transformer<bp::EntityDiagnosticTimingInfo_<2192>, bp::EntityDiagnosticTimingInfo_<2168>>::transform(
    Context<bp::EntityDiagnosticTimingInfo_<2168>> &ctx, bp::EntityDiagnosticTimingInfo_<2192> &&from)
{
    auto &to = ctx.out();
    to.display_name = std::move(from.display_name);
    to.entity = std::move(from.entity);
    to.time_in_ns = from.time_in_ns;
    // ENDWEAVE: position and dimension are dropped; 2168 names the timed entity and nothing more.
    to.percent_of_total = from.percent_of_total;
}

void Transformer<bp::ServerboundDiagnosticsPacket_<2192>, bp::ServerboundDiagnosticsPacket_<2168>>::transform(
    Context<bp::ServerboundDiagnosticsPacket_<2168>> &ctx, bp::ServerboundDiagnosticsPacket_<2192> &&from)
{
    auto &to = ctx.out();
    to.avg_fps = from.avg_fps;
    to.avg_server_sim_tick_time_ms = from.avg_server_sim_tick_time_ms;
    to.avg_client_sim_tick_time_ms = from.avg_client_sim_tick_time_ms;
    to.avg_begin_frame_time_ms = from.avg_begin_frame_time_ms;
    to.avg_input_time_ms = from.avg_input_time_ms;
    to.avg_render_time_ms = from.avg_render_time_ms;
    to.avg_end_frame_time_ms = from.avg_end_frame_time_ms;
    to.avg_remainder_time_percent = from.avg_remainder_time_percent;
    to.avg_unaccounted_time_percent = from.avg_unaccounted_time_percent;
    to.category_counters = ew::transform(ctx, std::move(from.category_counters));
    to.entity_timings = ew::transform(ctx, std::move(from.entity_timings));
    to.system_timings = std::move(from.system_timings);
    to.system_categories = std::move(from.system_categories);
    to.whisker_data = std::move(from.whisker_data);
}

} // namespace endweave
