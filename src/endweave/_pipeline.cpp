// Python bindings for the translation engine. Only bytes, ints and strings cross here, so it
// doesn't depend on Endstone's bindings.

#include "endweave/protocol/handler.h"
#include "endweave/protocol/session.h"

#include <bedrock/protocol/network.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace nb = nanobind;
namespace bp = bedrock::protocol;

using namespace nb::literals;

namespace {

// A raw pointer, so no destructor runs after the interpreter is gone.
PyObject *translation_error = nullptr;

/** Raised when a packet fails to decode, has bytes left over, or doesn't read back. A cancelled
 * packet returns None instead. */
[[noreturn]] void raiseTranslationError(int packet_id, const char *stage, const std::error_code &error)
{
    nb::object exception = nb::borrow(translation_error)(nb::str(error.message().c_str()));
    exception.attr("packet_id") = packet_id;
    exception.attr("stage") = nb::str(stage);
    PyErr_SetObject(translation_error, exception.ptr());
    throw nb::python_error();
}

/** One direction of translation. `actions` lists the packet ids that need work, so the caller
 * can skip every other id without calling in. */
struct Translator {
    using Actions = nb::typed<nb::mapping, int, endweave::Action>;

    Translator(int from_protocol, int to_protocol)
    {
        const auto from = endweave::ProtocolVersions::getProtocolVersion(from_protocol);
        const auto to = endweave::ProtocolVersions::getProtocolVersion(to_protocol);
        if (!from || !to) {
            throw nb::value_error("endweave: the engine does not translate one of these protocol versions");
        }

        engine = endweave::getTranslator(*from, *to);
        from_version = *from;
        to_version = *to;

        nb::dict verdicts;
        for (int id = 0; std::cmp_less(id, engine.size()); ++id) {
            const endweave::Action action = engine.getAction(id);
            if (action != endweave::Action::Passthrough) {
                verdicts[nb::int_(id)] = nb::cast(action);
            }
        }
        actions = nb::borrow<Actions>(nb::module_::import_("types").attr("MappingProxyType")(verdicts));
    }

    endweave::Translator engine;
    Actions actions;
    int from_version = 0;
    int to_version = 0;
};

std::optional<nb::bytes> translate(const Translator &translator, endweave::Session &session, int packet_id,
                                   nb::bytes payload)
{
    const endweave::PacketHandler handler = translator.engine.get(packet_id);
    if (handler == nullptr) {
        // Nothing to do for this id.
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

    nb::enum_<endweave::Action>(m, "Action",
                                "How a translator handles a packet id. Leave ids not in `actions` alone: assigning "
                                "the payload back makes the server rebuild the frame.")
        .value("TRANSLATE", endweave::Action::Translate)
        .value("CANCEL", endweave::Action::Cancel);

    m.def(
        "supported_versions",
        [] {
            const auto &versions = endweave::ProtocolVersions::SUPPORTED_VERSIONS;
            return std::vector<int>(versions.begin(), versions.end());
        },
        "The protocol versions the engine translates between, oldest first.");

    m.def(
        "packet_name",
        [](int packet_id) -> std::optional<std::string> {
            const std::string_view name = bp::enum_name(static_cast<bp::MinecraftPacketIds>(packet_id));
            if (name.empty()) {
                return std::nullopt;
            }
            return std::string(name);
        },
        "packet_id"_a, "The packet's name, or None if no version defines that id.");

    nb::class_<endweave::Session>(m, "Session",
                                  "Per-connection state. Pass the same session to both of a connection's translators.")
        .def(nb::init<>());

    nb::class_<Translator>(m, "Translator", "The translation from one protocol version to another.")
        .def(nb::init<int, int>(), "from_version"_a, "to_version"_a)
        .def_prop_ro("from_version",
                     [](const Translator &self) {
                         return self.from_version;
                     })
        .def_prop_ro("to_version",
                     [](const Translator &self) {
                         return self.to_version;
                     })
        .def_prop_ro(
            "actions",
            [](const Translator &self) {
                return self.actions;
            },
            "The action for each packet id that needs one. Other ids pass through untouched.")
        .def("translate", &translate, "session"_a, "packet_id"_a, "payload"_a,
             "The translated payload, or None if the packet was cancelled.");
}
