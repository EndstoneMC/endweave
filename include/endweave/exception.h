#pragma once

/// Structured error context for protocol translation failures.
///
/// See Also:
///     com.viaversion.viaversion.exception.InformativeException

#include <string>
#include <utility>
#include <vector>

namespace endweave {

class InformativeException {
public:
    explicit InformativeException(const std::string& cause) : cause_(cause) {}

    InformativeException& set(const std::string& key, const std::string& value) {
        entries_.emplace_back(key, value.size() > 256 ? value.substr(0, 256) + "..." : value);
        return *this;
    }

    [[nodiscard]] bool should_be_printed() const { return should_print_; }
    void set_should_be_printed(bool v) { should_print_ = v; }

    [[nodiscard]] std::string message() const {
        std::string context;
        for (auto& [k, v] : entries_) {
            if (!context.empty()) context += ", ";
            context += k + ": " + v;
        }
        if (context.empty()) context = "(no context)";
        return "Please report this on the Endweave GitHub repository\n"
             + context + ", Cause: " + cause_;
    }

private:
    std::string cause_;
    std::vector<std::pair<std::string, std::string>> entries_;
    bool should_print_ = true;
};

} // namespace endweave
