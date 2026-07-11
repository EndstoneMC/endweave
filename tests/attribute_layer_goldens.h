#pragma once

#include <initializer_list>
#include <string>

// Shared ClientboundAttributeLayerSync body goldens, used by both the converter
// test and the dispatch test so there is one source of truth for the wire bytes.

inline std::string bytes(std::initializer_list<int> raw)
{
    std::string out;
    for (int b : raw) {
        out.push_back(static_cast<char>(b));
    }
    return out;
}

// A case-2 (UpdateEnvironmentAttributes) packet carrying one bool attribute --
// the same body the bedrock-protocol goldens use: CloudburstMC/Protocol's wire
// structure with the `operation` name-code patched to BDS-verbatim UPPERCASE
// (1.26.20 binary, IDA-confirmed).
inline const std::string golden_975 = bytes({
    0x02,                                              // payload type = UpdateEnvironmentAttributes
    0x03, 0x77, 0x65, 0x74,                            // layer name "wet"
    0x00,                                              // dimension = 0
    0x01,                                              // attribute count = 1
    0x04, 0x74, 0x65, 0x6d, 0x70,                      // attribute name "temp"
    0x00,                                              // from_attribute absent
    0x00,                                              //   attribute type = bool
    0x01,                                              //   value = true
    0x08, 0x4f, 0x56, 0x45, 0x52, 0x52, 0x49, 0x44, 0x45,  //   operation "OVERRIDE" (CloudburstMC lowercase patched to BDS verbatim)
    0x00,                                              // to_attribute absent
    0x00, 0x00, 0x00, 0x00,                            // current_transition_ticks = 0
    0x00, 0x00, 0x00, 0x00,                            // total_transition_ticks = 0
    0x06, 0x6c, 0x69, 0x6e, 0x65, 0x61, 0x72,          // easing "linear"
});

// v1001 = v975 plus the two 976-step EnvironmentAttributeData fields, at neutral
// defaults -- so the upgrade polyfill reproduces it exactly and the downgrade
// drops back to golden_975.
inline const std::string golden_1001 = golden_975 + bytes({
    0x00, 0x00, 0x00, 0x00,  // local_transition_ticks = 0
    0x00,                    // noise_transition = false
});
