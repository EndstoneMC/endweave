#include "endweave/protocols/v2168/diagnostics.h"

#include <bedrock/enum.hpp>
#include <optional>
#include <utility>

namespace ew = endweave;

namespace endweave {

void Transformer<bp::MemoryCategoryCounter_<2168>, bp::MemoryCategoryCounter_<1001>>::transform(
    Context<bp::MemoryCategoryCounter_<1001>> &ctx, bp::MemoryCategoryCounter_<2168> &&from)
{
    auto &to = ctx.out();
    // ENDWEAVE: 2168 renumbered MemoryCategory, so the name carries the meaning, not the byte. Its twenty
    // added categories have no 1001 name and land on Unknown rather than on a guess.
    to.category = bp::enum_cast<bp::MemoryCategory_<1001>>(bp::enum_name(from.category))
                      .value_or(bp::MemoryCategory_<1001>::UNKNOWN);
    to.current_bytes = from.current_bytes;
}

void Transformer<bp::ServerboundDiagnosticsPacket_<2168>, bp::ServerboundDiagnosticsPacket_<1001>>::transform(
    Context<bp::ServerboundDiagnosticsPacket_<1001>> &ctx, bp::ServerboundDiagnosticsPacket_<2168> &&from)
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
    to.entity_timings = std::move(from.entity_timings);
    // ENDWEAVE: system_categories is dropped; 1001 reads these timings by index alone.
    to.system_timings = std::move(from.system_timings);
    to.whisker_data = std::move(from.whisker_data);
}

void Transformer<bp::MemoryCategoryCounter_<2168>, bp::MemoryCategoryCounter_<2192>>::transform(
    Context<bp::MemoryCategoryCounter_<2192>> &ctx, bp::MemoryCategoryCounter_<2168> &&from)
{
    auto &to = ctx.out();
    // ENDWEAVE: 2192 dropped Persona_Textures and shifted every category above it down one, so the name
    // carries the meaning. The dropped one has no 2192 counterpart and lands on Unknown.
    to.category = bp::enum_cast<bp::MemoryCategory_<2192>>(bp::enum_name(from.category))
                      .value_or(bp::MemoryCategory_<2192>::UNKNOWN);
    to.current_bytes = from.current_bytes;
}

void Transformer<bp::EntityDiagnosticTimingInfo_<2168>, bp::EntityDiagnosticTimingInfo_<2192>>::transform(
    Context<bp::EntityDiagnosticTimingInfo_<2192>> &ctx, bp::EntityDiagnosticTimingInfo_<2168> &&from)
{
    auto &to = ctx.out();
    to.display_name = std::move(from.display_name);
    to.entity = std::move(from.entity);
    to.time_in_ns = from.time_in_ns;
    to.percent_of_total = from.percent_of_total;
    // ENDWEAVE: 2168 locates the timed entity by nothing but its name, and absent is what 2192 writes
    // when the client has no position to offer.
    to.position = std::nullopt;
    to.dimension = std::nullopt;
}

void Transformer<bp::ServerboundDiagnosticsPacket_<2168>, bp::ServerboundDiagnosticsPacket_<2192>>::transform(
    Context<bp::ServerboundDiagnosticsPacket_<2192>> &ctx, bp::ServerboundDiagnosticsPacket_<2168> &&from)
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
