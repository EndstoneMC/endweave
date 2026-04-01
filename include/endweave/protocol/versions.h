#pragma once

/// Central registry of known Bedrock protocol versions.

#include <string>
#include <unordered_map>

namespace endweave {

struct ProtocolVersion {
    int protocol;
    std::string minecraft_version;
};

// Registry
inline const ProtocolVersion v1_21_120{859, "1.21.120"};
inline const ProtocolVersion v1_21_124{860, "1.21.124"};
inline const ProtocolVersion v1_21_130{898, "1.21.130"};
inline const ProtocolVersion v1_26_0{924, "1.26.0"};
inline const ProtocolVersion v1_26_10{944, "1.26.10"};

inline const std::unordered_map<int, const ProtocolVersion*>& get_versions() {
    static const std::unordered_map<int, const ProtocolVersion*> versions = {
        {859, &v1_21_120},
        {860, &v1_21_124},
        {898, &v1_21_130},
        {924, &v1_26_0},
        {944, &v1_26_10},
    };
    return versions;
}

} // namespace endweave
