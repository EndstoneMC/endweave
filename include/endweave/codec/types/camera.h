#pragma once

/// SplineInstruction compound types for camera packets.

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "endweave/codec/reader.h"
#include "endweave/codec/types/primitives.h"
#include "endweave/codec/writer.h"

namespace endweave {

struct KeyFrame {
    float value = 0.0f;
    float time = 0.0f;
    std::optional<std::int32_t> easing; // absent in v898
};

struct RotationKeyFrame {
    Vec3Value value;
    float time = 0.0f;
    std::optional<std::int32_t> easing; // absent in v898
};

struct SplineInstruction {
    float total_time = 0.0f;
    std::int32_t type = 0;
    std::vector<Vec3Value> curve;
    std::vector<KeyFrame> progress_key_frames;
    std::vector<RotationKeyFrame> rotation_key_frames;
    std::optional<std::string> spline_identifier; // v944+
    std::optional<bool> load_from_json;            // v944+
};

struct spline_instruction_v898 { using value_type = SplineInstruction; };
struct spline_instruction_v924 { using value_type = SplineInstruction; };
struct spline_instruction_v944 { using value_type = SplineInstruction; };

template <> auto PacketReader::read<spline_instruction_v898>() -> std::expected<SplineInstruction, ReadError>;
template <> auto PacketReader::read<spline_instruction_v924>() -> std::expected<SplineInstruction, ReadError>;
template <> auto PacketReader::read<spline_instruction_v944>() -> std::expected<SplineInstruction, ReadError>;
template <> void PacketWriter::write<spline_instruction_v898>(const SplineInstruction& val);
template <> void PacketWriter::write<spline_instruction_v924>(const SplineInstruction& val);
template <> void PacketWriter::write<spline_instruction_v944>(const SplineInstruction& val);

} // namespace endweave
