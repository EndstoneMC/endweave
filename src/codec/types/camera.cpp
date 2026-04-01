/// SplineInstruction codec implementation.

#include "endweave/codec/types/camera.h"

namespace endweave {

namespace {

enum class SplineVersion { V898, V924, V944 };

std::expected<SplineInstruction, ReadError> read_spline(PacketReader& r, SplineVersion ver) {
    SplineInstruction s;
    { auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); s.total_time = *v; }
    { auto v = r.read_varint(); if (!v) return std::unexpected(v.error()); s.type = *v; }

    // Curve: uvarint count + Vec3 entries
    { auto count = r.read_uvarint(); if (!count) return std::unexpected(count.error());
      s.curve.reserve(*count);
      for (std::uint32_t i = 0; i < *count; ++i) {
          auto v = r.read<vec3>(); if (!v) return std::unexpected(v.error());
          s.curve.push_back(*v);
      }
    }

    // Progress key frames
    { auto count = r.read_uvarint(); if (!count) return std::unexpected(count.error());
      s.progress_key_frames.reserve(*count);
      for (std::uint32_t i = 0; i < *count; ++i) {
          KeyFrame kf;
          if (ver == SplineVersion::V898) {
              // v898: Vec2 (value, time), no easing
              auto x = r.read_float_le(); if (!x) return std::unexpected(x.error()); kf.value = *x;
              auto y = r.read_float_le(); if (!y) return std::unexpected(y.error()); kf.time = *y;
          } else {
              auto v = r.read_float_le(); if (!v) return std::unexpected(v.error()); kf.value = *v;
              auto t = r.read_float_le(); if (!t) return std::unexpected(t.error()); kf.time = *t;
              auto e = r.read_int_le(); if (!e) return std::unexpected(e.error()); kf.easing = *e;
          }
          s.progress_key_frames.push_back(kf);
      }
    }

    // Rotation key frames
    { auto count = r.read_uvarint(); if (!count) return std::unexpected(count.error());
      s.rotation_key_frames.reserve(*count);
      for (std::uint32_t i = 0; i < *count; ++i) {
          RotationKeyFrame rkf;
          auto v = r.read<vec3>(); if (!v) return std::unexpected(v.error()); rkf.value = *v;
          auto t = r.read_float_le(); if (!t) return std::unexpected(t.error()); rkf.time = *t;
          if (ver != SplineVersion::V898) {
              auto e = r.read_int_le(); if (!e) return std::unexpected(e.error()); rkf.easing = *e;
          }
          s.rotation_key_frames.push_back(rkf);
      }
    }

    // v944+: splineIdentifier + loadFromJson
    if (ver == SplineVersion::V944) {
        auto id = r.read_string(); if (!id) return std::unexpected(id.error()); s.spline_identifier = std::move(*id);
        auto lfj = r.read_bool(); if (!lfj) return std::unexpected(lfj.error()); s.load_from_json = *lfj;
    }

    return s;
}

void write_spline(PacketWriter& w, const SplineInstruction& s, SplineVersion ver) {
    w.write_float_le(s.total_time);
    w.write_varint(s.type);

    w.write_uvarint(static_cast<std::uint32_t>(s.curve.size()));
    for (auto& v : s.curve) w.write<vec3>(v);

    w.write_uvarint(static_cast<std::uint32_t>(s.progress_key_frames.size()));
    for (auto& kf : s.progress_key_frames) {
        if (ver == SplineVersion::V898) {
            w.write_float_le(kf.value);
            w.write_float_le(kf.time);
        } else {
            w.write_float_le(kf.value);
            w.write_float_le(kf.time);
            w.write_int_le(kf.easing.value_or(0));
        }
    }

    w.write_uvarint(static_cast<std::uint32_t>(s.rotation_key_frames.size()));
    for (auto& rkf : s.rotation_key_frames) {
        w.write<vec3>(rkf.value);
        w.write_float_le(rkf.time);
        if (ver != SplineVersion::V898) {
            w.write_int_le(rkf.easing.value_or(0));
        }
    }

    if (ver == SplineVersion::V944) {
        w.write_string(s.spline_identifier.value_or(""));
        w.write_bool(s.load_from_json.value_or(false));
    }
}

} // namespace

template <> auto PacketReader::read<spline_instruction_v898>() -> std::expected<SplineInstruction, ReadError> {
    return read_spline(*this, SplineVersion::V898);
}
template <> auto PacketReader::read<spline_instruction_v924>() -> std::expected<SplineInstruction, ReadError> {
    return read_spline(*this, SplineVersion::V924);
}
template <> auto PacketReader::read<spline_instruction_v944>() -> std::expected<SplineInstruction, ReadError> {
    return read_spline(*this, SplineVersion::V944);
}
template <> void PacketWriter::write<spline_instruction_v898>(const SplineInstruction& val) {
    write_spline(*this, val, SplineVersion::V898);
}
template <> void PacketWriter::write<spline_instruction_v924>(const SplineInstruction& val) {
    write_spline(*this, val, SplineVersion::V924);
}
template <> void PacketWriter::write<spline_instruction_v944>(const SplineInstruction& val) {
    write_spline(*this, val, SplineVersion::V944);
}

} // namespace endweave
