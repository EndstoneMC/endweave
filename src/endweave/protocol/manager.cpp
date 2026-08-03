#include "endweave/protocol/manager.h"

#include "endweave/protocols/base/protocol.h"
#include "endweave/protocols/v1001/protocol.h"
#include "endweave/protocols/v975/protocol.h"

#include <deque>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace endweave {
namespace {

/**
 * A vertex reached by the search, and how it was reached.
 */
struct PathNode {
    const PathNode *parent = nullptr;
    int version = 0;
    const AbstractProtocol *protocol = nullptr;
    Step step = Step::Upgrade;
    int depth = 0;
};

} // namespace

void ProtocolManager::registerProtocol(std::unique_ptr<AbstractProtocol> protocol)
{
    if (protocol->isBaseProtocol() || !protocol->getVersion().has_value()) {
        throw std::invalid_argument("protocol " + protocol->getName() + " is not a version node");
    }

    const int version = protocol->getVersion().value();
    if (nodes_.contains(version)) {
        throw std::invalid_argument("protocol " + std::to_string(version) + " is already registered");
    }

    protocol->initialize();

    if (!nodes_.empty()) {
        const int previous = nodes_.rbegin()->first;
        if (version <= previous) {
            throw std::invalid_argument("protocols must be registered in ascending version order: " +
                                        std::to_string(version) + " came after " + std::to_string(previous));
        }
        protocol->setPreviousVersion(previous);
        nodes_[previous].edges.push_back({version, protocol.get(), Step::Upgrade});
        nodes_[version].edges.push_back({previous, protocol.get(), Step::Downgrade});
    }

    nodes_[version].protocol = protocol.get();
    owned_.push_back(std::move(protocol));
    path_cache_.clear();
    refreshInteresting();
}

void ProtocolManager::registerBaseProtocol(std::unique_ptr<AbstractProtocol> base_protocol)
{
    if (!base_protocol->isBaseProtocol()) {
        throw std::invalid_argument("protocol " + base_protocol->getName() + " is not a base protocol");
    }
    base_protocol->initialize();
    base_protocols_.push_back(base_protocol.get());
    owned_base_.push_back(std::move(base_protocol));
    refreshInteresting();
}

void ProtocolManager::refreshInteresting()
{
    // Slot 0 is Serverbound and Upgrade, slot 1 Clientbound and Downgrade, so both axes are covered.
    const auto mark = [this](const AbstractProtocol &protocol) {
        for (int packet_id = 0; packet_id < kPacketIdCount; ++packet_id) {
            if (protocol.hasMapping(0, packet_id) || protocol.hasMapping(1, packet_id)) {
                interesting_.set(static_cast<std::size_t>(packet_id));
            }
        }
    };

    interesting_.reset();
    for (const AbstractProtocol *protocol : base_protocols_) {
        mark(*protocol);
    }
    for (const auto &[version, node] : nodes_) {
        mark(*node.protocol);
    }
}

void ProtocolManager::registerProtocols()
{
    registerBaseProtocol(std::make_unique<InitialBaseProtocol>());
    registerProtocol<ProtocolVersion::v26_20>();
    registerProtocol<ProtocolVersion::v26_30>();
}

const AbstractProtocol *ProtocolManager::getProtocol(int version) const
{
    const auto it = nodes_.find(version);
    return it == nodes_.end() ? nullptr : it->second.protocol;
}

std::optional<ProtocolPath> ProtocolManager::getProtocolPath(int client_protocol_version, int server_protocol_version)
{
    if (client_protocol_version == server_protocol_version) {
        return ProtocolPath{};
    }

    const ProtocolPathKey key{client_protocol_version, server_protocol_version};
    if (const auto cached = path_cache_.find(key); cached != path_cache_.end()) {
        return cached->second;
    }

    auto path = calculateProtocolPath(client_protocol_version, server_protocol_version);
    path_cache_.emplace(key, path);
    return path;
}

std::optional<ProtocolPath> ProtocolManager::calculateProtocolPath(int from, int to) const
{
    if (!nodes_.contains(from) || !nodes_.contains(to)) {
        return std::nullopt;
    }

    std::deque<PathNode> nodes;
    std::unordered_set<int> visited{from};
    std::deque<const PathNode *> queue;
    queue.push_back(&nodes.emplace_back(PathNode{nullptr, from, nullptr, Step::Upgrade, 0}));

    const PathNode *found = nullptr;
    while (!queue.empty() && found == nullptr) {
        const PathNode *current = queue.front();
        queue.pop_front();
        if (current->depth > max_protocol_path_size_) {
            continue; // fail-safe: too deep
        }

        for (const Edge &edge : nodes_.at(current->version).edges) {
            if (edge.to == to) {
                found = &nodes.emplace_back(PathNode{current, edge.to, edge.protocol, edge.step, current->depth + 1});
                break;
            }
            if (visited.contains(edge.to)) {
                continue;
            }
            visited.insert(edge.to);
            queue.push_back(
                &nodes.emplace_back(PathNode{current, edge.to, edge.protocol, edge.step, current->depth + 1}));
        }
    }

    if (found == nullptr) {
        return std::nullopt;
    }

    ProtocolPath path(static_cast<std::size_t>(found->depth));
    for (const PathNode *node = found; node->protocol != nullptr; node = node->parent) {
        path[static_cast<std::size_t>(node->depth) - 1] = {node->protocol, node->step, node->version};
    }
    return path;
}

void ProtocolManager::refreshVersions(int server_protocol_version)
{
    supported_versions_.clear();
    supported_versions_.insert(server_protocol_version);
    for (const auto &[version, node] : nodes_) {
        auto path = getProtocolPath(version, server_protocol_version);
        if (!path) {
            continue;
        }
        supported_versions_.insert(version);
        for (const ProtocolPathEntry &entry : path.value()) {
            supported_versions_.insert(entry.output_protocol_version);
        }
    }
}

} // namespace endweave
