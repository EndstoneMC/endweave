#include "endweave/protocols/v1001/diagnostics.h"

#include <bedrock/enum.hpp>
#include <optional>
#include <utility>

namespace ew = endweave;

namespace endweave {

bp::MemoryCategoryCounter_<2168> Transformer<bp::MemoryCategoryCounter_<1001>, bp::MemoryCategoryCounter_<2168>>::
    transform(bp::MemoryCategoryCounter_<1001> &&from)
{
    bp::MemoryCategoryCounter_<2168> to;
    // ENDWEAVE: 2168 renumbered MemoryCategory, so the name carries the meaning, not the byte. 1001's
    // Persona became six Persona* categories and cannot be split across them, so it lands on Unknown.
    to.category = bp::enum_cast<bp::MemoryCategory_<2168>>(bp::enum_name(from.category))
                      .value_or(bp::MemoryCategory_<2168>::UNKNOWN);
    to.current_bytes = from.current_bytes;
    return to;
}

bp::ServerboundDiagnosticsPacket_<2168> Transformer<
    bp::ServerboundDiagnosticsPacket_<1001>,
    bp::ServerboundDiagnosticsPacket_<2168>>::transform(bp::ServerboundDiagnosticsPacket_<1001> &&from)
{
    bp::ServerboundDiagnosticsPacket_<2168> to;
    to.avg_fps = from.avg_fps;
    to.avg_server_sim_tick_time_ms = from.avg_server_sim_tick_time_ms;
    to.avg_client_sim_tick_time_ms = from.avg_client_sim_tick_time_ms;
    to.avg_begin_frame_time_ms = from.avg_begin_frame_time_ms;
    to.avg_input_time_ms = from.avg_input_time_ms;
    to.avg_render_time_ms = from.avg_render_time_ms;
    to.avg_end_frame_time_ms = from.avg_end_frame_time_ms;
    to.avg_remainder_time_percent = from.avg_remainder_time_percent;
    to.avg_unaccounted_time_percent = from.avg_unaccounted_time_percent;
    to.category_counters = ew::transform(std::move(from.category_counters));
    to.entity_timings = std::move(from.entity_timings);
    to.system_timings = std::move(from.system_timings);
    // ENDWEAVE: 1001 sends no list naming the systems system_diagnostics indexes, and absent is what 2168
    // writes when the client has none to offer.
    to.system_categories = std::nullopt;
    to.whisker_data = std::move(from.whisker_data);
    return to;
}

} // namespace endweave
