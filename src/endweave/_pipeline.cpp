// The translation engine's Python surface. Only bytes, ints and strings cross here: nothing in
// this module names an Endstone type, which is what keeps it clear of the pybind11 module that
// binds them.

#include "endweave/protocol/session.h"

#include <bedrock/protocol/network.h>
#include <exception>
#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <new>
#include <optional>
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

std::optional<nb::bytes> translate(endweave::Session &session, const endweave::PacketHandlers &handlers, int packet_id,
                                   nb::bytes payload)
{
    const endweave::PacketHandler handler = handlers.get(packet_id);
    if (handler == nullptr) {
        // The caller reads the action table before calling, so reaching here means it asked for a
        // packet with nothing to do. Hand the payload back rather than inventing an error.
        return payload;
    }

    std::string translated;
    bp::BinaryWriter out{translated};
    bp::BinaryReader in{std::string_view{payload.c_str(), payload.size()}};
    bool cancelled = false;
    const auto result = handler(session, cancelled, in, out);
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

    m.attr("PASSTHROUGH") = static_cast<int>(endweave::Action::Passthrough);
    m.attr("TRANSLATE") = static_cast<int>(endweave::Action::Translate);
    m.attr("CANCEL") = static_cast<int>(endweave::Action::Cancel);
    m.attr("ACTION_TABLE_SIZE") = static_cast<int>(endweave::kActionTableSize);
    m.attr("UNKNOWN") = static_cast<int>(endweave::ProtocolVersion::UNKNOWN);

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
                                  "One connection's two directions, resolved once when the client's version "
                                  "is known.")
        .def(
            "__init__",
            [](endweave::Session *self, int client_version, int server_version) {
                new (self) endweave::Session(endweave::ProtocolVersions::getProtocolVersion(client_version),
                                             endweave::ProtocolVersions::getProtocolVersion(server_version));
            },
            "client_version"_a, "server_version"_a)
        .def_prop_ro("client_version",
                     [](const endweave::Session &self) {
                         return static_cast<int>(self.getClientVersion());
                     })
        .def_prop_ro("server_version",
                     [](const endweave::Session &self) {
                         return static_cast<int>(self.getServerVersion());
                     })
        .def_prop_ro(
            "serverbound_actions",
            [](const endweave::Session &self) {
                const std::string table = endweave::actionTable(self.getServerboundHandlers());
                return nb::bytes(table.data(), table.size());
            },
            "One byte per packet id: PASSTHROUGH, TRANSLATE or CANCEL. Index it before calling in; "
            "a passthrough packet must not be touched at all, since assigning the payload back "
            "makes the server rebuild the frame.")
        .def_prop_ro("clientbound_actions",
                     [](const endweave::Session &self) {
                         const std::string table = endweave::actionTable(self.getClientboundHandlers());
                         return nb::bytes(table.data(), table.size());
                     })
        .def(
            "translate_serverbound",
            [](endweave::Session &self, int packet_id, nb::bytes payload) {
                return translate(self, self.getServerboundHandlers(), packet_id, std::move(payload));
            },
            "packet_id"_a, "payload"_a,
            "The payload as the server should read it, or None where a transform refused it.")
        .def(
            "translate_clientbound",
            [](endweave::Session &self, int packet_id, nb::bytes payload) {
                return translate(self, self.getClientboundHandlers(), packet_id, std::move(payload));
            },
            "packet_id"_a, "payload"_a,
            "The payload as the client should read it, or None where a transform refused it.");
}
