#include "endweave/protocols/v2168/diagnostics.h"

#include <bedrock/enum.hpp>
#include <utility>

namespace ew = endweave;

namespace endweave {

bp::MemoryCategoryCounter_<1001> Transformer<bp::MemoryCategoryCounter_<2168>>::downgrade(
    bp::MemoryCategoryCounter_<2168> &&from)
{
    bp::MemoryCategoryCounter_<1001> to;
    // ENDWEAVE: 2168 renumbered MemoryCategory, so the name carries the meaning, not the byte. Its twenty
    // added categories have no 1001 name and land on Unknown rather than on a guess.
    to.category = bp::enum_cast<bp::MemoryCategory_<1001>>(bp::enum_name(from.category))
                      .value_or(bp::MemoryCategory_<1001>::UNKNOWN);
    to.current_bytes = from.current_bytes;
    return to;
}

bp::ServerboundDiagnosticsPacket_<1001> Transformer<bp::ServerboundDiagnosticsPacket_<2168>>::downgrade(
    bp::ServerboundDiagnosticsPacket_<2168> &&from)
{
    bp::ServerboundDiagnosticsPacket_<1001> to;
    to.avg_fps = from.avg_fps;
    to.avg_server_sim_tick_time_ms = from.avg_server_sim_tick_time_ms;
    to.avg_client_sim_tick_time_ms = from.avg_client_sim_tick_time_ms;
    to.avg_begin_frame_time_ms = from.avg_begin_frame_time_ms;
    to.avg_input_time_ms = from.avg_input_time_ms;
    to.avg_render_time_ms = from.avg_render_time_ms;
    to.avg_end_frame_time_ms = from.avg_end_frame_time_ms;
    to.avg_remainder_time_percent = from.avg_remainder_time_percent;
    to.avg_unaccounted_time_percent = from.avg_unaccounted_time_percent;
    to.memory_category_values = ew::downgrade(from.memory_category_values);
    to.entity_diagnostics = std::move(from.entity_diagnostics);
    // ENDWEAVE: system_categories is dropped; 1001 reads these timings by index alone.
    to.system_diagnostics = std::move(from.system_diagnostics);
    to.whisker_scopes = std::move(from.whisker_scopes);
    return to;
}

} // namespace endweave
