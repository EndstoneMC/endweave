#pragma once

#include "endweave/log_sink.h"

#include <string>
#include <string_view>
#include <vector>

// A LogSink that records messages, for asserting on side-effect logging in tests.
struct RecordingLogSink : endweave::LogSink {
    std::vector<std::string> debugs;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    void debug(std::string_view message) override
    {
        debugs.emplace_back(message);
    }
    void warning(std::string_view message) override
    {
        warnings.emplace_back(message);
    }
    void error(std::string_view message) override
    {
        errors.emplace_back(message);
    }
};
