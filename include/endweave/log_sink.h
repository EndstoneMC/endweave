#pragma once

#include <string_view>

namespace endweave {

// Abstract log seam so the routing core stays Endstone-free. The plugin
// implements it over endstone::Logger; tests use a recording sink; the default
// is a silent no-op.
struct LogSink {
    virtual ~LogSink() = default;
    virtual void debug(std::string_view) {}
    virtual void warning(std::string_view) {}
    virtual void error(std::string_view) {}
};

} // namespace endweave
