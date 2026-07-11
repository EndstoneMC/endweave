#pragma once

#include "endweave/protocol/direction.h"

#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace endweave {

class Protocol;
class ProtocolManager;
class UserConnection;

// Outcome of running a packet body through a pipeline.
struct PipelineRunResult {
    enum class Status {
        Unchanged,
        Rewritten,
        Cancelled,
        Failed
    };
    Status status = Status::Unchanged;
    std::string payload;                    // valid for Unchanged / Rewritten
    const Protocol *failed_stage = nullptr; // valid for Failed
    std::error_code error;                  // valid for Failed
};

// Thread a packet body through each stage's Protocol::transform, in order. A fresh
// reader/writer per stage feeds the previous stage's output into the next -- the
// analogue of ViaVersion's per-stage fresh PacketWrapper.
PipelineRunResult run_pipeline(std::span<const Protocol *const> pipeline, Direction direction, int packet_id,
                               UserConnection &connection, std::string_view payload);

// ViaVersion's _get_pipeline: resolve, and cache on the connection, the serverbound
// pipeline (base + chain) and the clientbound pipeline (base + reversed(chain)).
// Returns the serverbound pipeline.
std::vector<const Protocol *> resolve_pipeline(ProtocolManager &manager, UserConnection &connection);

} // namespace endweave
