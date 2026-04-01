#pragma once

/// Debug packet logging with per-packet-type filtering.
///
/// See Also:
///     com.viaversion.viaversion.api.debug.DebugHandler

#include <unordered_set>

#include "endweave/protocol/packet_ids.h"

namespace endweave {

class DebugHandler {
public:
    DebugHandler() = default;
    DebugHandler(bool enabled, std::unordered_set<int> filter = {},
                 bool log_pre = true, bool log_post = false)
        : enabled_(enabled), log_pre_(log_pre), log_post_(log_post)
        , filter_(std::move(filter)) {}

    [[nodiscard]] bool enabled() const { return enabled_; }
    [[nodiscard]] bool log_pre_packet_transform() const { return enabled_ && log_pre_; }
    [[nodiscard]] bool log_post_packet_transform() const { return enabled_ && log_post_; }

    [[nodiscard]] bool should_log(int packet_id) const {
        if (!enabled_) return false;
        if (filter_.empty()) return true;
        return filter_.contains(packet_id);
    }

private:
    bool enabled_ = false;
    bool log_pre_ = true;
    bool log_post_ = false;
    std::unordered_set<int> filter_;
};

} // namespace endweave
