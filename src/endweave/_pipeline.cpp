// The translation engine's Python surface. Only bytes, ints and strings cross here: nothing in
// this module names an Endstone type, which is what keeps it clear of the pybind11 module that
// binds them.

#include "endweave/protocol/handler.h"
#include "endweave/protocol/session.h"

#include <bedrock/protocol/network.h>
#include <cstddef>
#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace nb = nanobind;
namespace bp = bedrock::protocol;

using namespace nb::literals;

namespace {

// Held for the life of the module rather than in an nb::object with static storage, whose
// destructor would run against an interpreter that may already be gone.
PyObject *translation_error = nullptr;

/** Raised where a packet could not be carried: it failed to decode at the source, left bytes
 * unread, or did not read back at the destination. Distinct from a refusal, which is not an
 * error and comes back as None. */
[[noreturn]] void raiseTranslationError(int packet_id, const char *stage, const std::error_code &error)
{
    nb::object exception = nb::borrow(translation_error)(nb::str(error.message().c_str()));
    exception.attr("packet_id") = packet_id;
    exception.attr("stage") = nb::str(stage);
    PyErr_SetObject(translation_error, exception.ptr());
    throw nb::python_error();
}

/** One direction's translation, with the engine's per-id verdicts built once as a mapping the
 * caller holds. An id the mapping does not name costs nothing, so the common packet is a miss
 * and its payload never crosses into the engine at all. */
struct Translator {
    using Actions = nb::typed<nb::mapping, int, endweave::Action>;

    Translator(int from_version, int to_version)
    {
        const endweave::ProtocolVersion from = endweave::ProtocolVersions::getProtocolVersion(from_version);
        const endweave::ProtocolVersion to = endweave::ProtocolVersions::getProtocolVersion(to_version);
        if (from == endweave::ProtocolVersion::UNKNOWN || to == endweave::ProtocolVersion::UNKNOWN) {
            throw nb::value_error("endweave: the engine does not translate one of these protocol versions");
        }

        engine = endweave::getTranslator(from, to);
        from_version_ = static_cast<int>(from);
        to_version_ = static_cast<int>(to);

        nb::dict verdicts;
        const std::span<const endweave::Action> table = engine.getActions();
        for (std::size_t id = 0; id < table.size(); ++id) {
            if (table[id] != endweave::Action::Passthrough) {
                verdicts[nb::int_(static_cast<int>(id))] = nb::cast(table[id]);
            }
        }
        actions = nb::borrow<Actions>(nb::module_::import_("types").attr("MappingProxyType")(verdicts));
    }

    endweave::Translator engine;
    Actions actions;
    int from_version_ = 0;
    int to_version_ = 0;
};

std::optional<nb::bytes> translate(const Translator &translator, endweave::Session &session, int packet_id,
                                   nb::bytes payload)
{
    switch (translator.engine.getAction(packet_id)) {
    case endweave::Action::Passthrough:
        // The caller reads `actions` before calling, so reaching here means it asked for a packet
        // with nothing to do. Hand the payload back rather than inventing an error.
        return payload;
    case endweave::Action::Cancel:
        return std::nullopt;
    case endweave::Action::Translate:
        break;
    }

    std::string translated;
    bp::BinaryWriter out{translated};
    bp::BinaryReader in{std::string_view{payload.c_str(), payload.size()}};
    bool cancelled = false;
    const auto result = translator.engine.get(packet_id)(session, cancelled, in, out);
    if (!result) {
        raiseTranslationError(packet_id, "translate", result.error());
    }
    if (cancelled) {
        return std::nullopt;
    }
    return nb::bytes(translated.data(), translated.size());
}

} // namespace

NB_MODULE(_pipeline, m)
{
    m.doc() = "Compile-time protocol translation between Bedrock versions.";

    translation_error = PyErr_NewException("endweave._pipeline.TranslationError", PyExc_RuntimeError, nullptr);
    m.attr("TranslationError") = nb::borrow(translation_error);

    m.attr("UNKNOWN") = static_cast<int>(endweave::ProtocolVersion::UNKNOWN);

    nb::enum_<endweave::Action>(m, "Action",
                                "What a packet costs on a translator. An id the translator does not name costs "
                                "nothing and must not be touched at all, since assigning the payload back makes "
                                "the server rebuild the frame.")
        .value("TRANSLATE", endweave::Action::Translate)
        .value("CANCEL", endweave::Action::Cancel);

    m.def(
        "supported_versions",
        [] {
            std::vector<int> versions;
            versions.reserve(endweave::ProtocolVersions::SUPPORTED_VERSIONS.size());
            for (const endweave::ProtocolVersion version : endweave::ProtocolVersions::SUPPORTED_VERSIONS) {
                versions.push_back(static_cast<int>(version));
            }
            return versions;
        },
        "The protocol versions the engine translates between, oldest first.");

    m.def(
        "resolve",
        [](int protocol_version) {
            return static_cast<int>(endweave::ProtocolVersions::getProtocolVersion(protocol_version));
        },
        "protocol_version"_a,
        "The version a protocol id is translated as, applying the wire-identical aliases, or "
        "UNKNOWN where the engine does not translate it.");

    m.def(
        "packet_name",
        [](int packet_id) -> std::optional<std::string> {
            const std::string_view name = bp::enum_name(static_cast<bp::MinecraftPacketIds>(packet_id));
            if (name.empty()) {
                return std::nullopt;
            }
            return std::string(name);
        },
        "packet_id"_a, "The packet's name, or None where no version names that id.");

    nb::class_<endweave::Session>(m, "Session",
                                  "What one connection carries across its packets. Both of a connection's "
                                  "translators are handed the same session.")
        .def(nb::init<>());

    nb::class_<Translator>(m, "Translator", "The translation from one protocol version to another.")
        .def(nb::init<int, int>(), "from_version"_a, "to_version"_a)
        .def_prop_ro("from_version",
                     [](const Translator &self) {
                         return self.from_version_;
                     })
        .def_prop_ro("to_version",
                     [](const Translator &self) {
                         return self.to_version_;
                     })
        .def_prop_ro(
            "actions",
            [](const Translator &self) {
                return self.actions;
            },
            "The verdict for every packet id that needs one. Read it before calling in; an id it "
            "does not name is carried untouched.")
        .def("translate", &translate, "session"_a, "packet_id"_a, "payload"_a,
             "The payload as the other side should read it, or None where the packet was refused.");
}
