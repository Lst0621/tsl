#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

template <typename NodeId = int, typename Label = std::monostate,
          typename Weight = int>
class Graph {
   public:
    using EdgeId = int;

    struct Edge {
        NodeId from;
        NodeId to;
        Weight weight;
        Label label;
    };

   private:
    std::vector<Edge> edges_;
    std::vector<bool> edge_active_;
    std::size_t active_edge_count_ = 0;

    std::unordered_map<NodeId, std::vector<EdgeId>> out_edges_;
    std::unordered_map<NodeId, std::vector<EdgeId>> in_edges_;
    std::unordered_set<NodeId> nodes_;

    bool directed_ = false;

    static const std::vector<EdgeId>& empty_edge_list() {
        static const std::vector<EdgeId> empty;
        return empty;
    }

    void require_vertex_exists(const NodeId& id) const {
        if (!has_vertex(id)) {
            throw std::out_of_range("Vertex does not exist");
        }
    }

    void require_edge_id_valid(EdgeId edge_id) const {
        if (edge_id < 0 ||
            static_cast<std::size_t>(edge_id) >= edges_.size()) {
            throw std::out_of_range("EdgeId out of range");
        }
    }

    void require_edge_active(EdgeId edge_id) const {
        require_edge_id_valid(edge_id);
        if (!edge_active_[static_cast<std::size_t>(edge_id)]) {
            throw std::out_of_range("Edge is not active");
        }
    }

    static void erase_edge_id(std::vector<EdgeId>& ids, EdgeId edge_id) {
        ids.erase(std::remove(ids.begin(), ids.end(), edge_id), ids.end());
    }

   public:
    explicit Graph(bool directed = false) : directed_(directed) {
    }

    bool is_directed() const {
        return directed_;
    }

    void add_vertex(NodeId id) {
        nodes_.insert(id);
        out_edges_.try_emplace(id, std::vector<EdgeId>{});
        in_edges_.try_emplace(id, std::vector<EdgeId>{});
    }

    bool has_vertex(NodeId id) const {
        return nodes_.find(id) != nodes_.end();
    }

    void remove_vertex(NodeId id) {
        if (!has_vertex(id)) {
            return;
        }

        std::vector<EdgeId> incident;
        {
            auto it_out = out_edges_.find(id);
            if (it_out != out_edges_.end()) {
                incident.insert(incident.end(), it_out->second.begin(),
                                it_out->second.end());
            }

            auto it_in = in_edges_.find(id);
            if (it_in != in_edges_.end()) {
                incident.insert(incident.end(), it_in->second.begin(),
                                it_in->second.end());
            }
        }

        std::sort(incident.begin(), incident.end());
        incident.erase(std::unique(incident.begin(), incident.end()),
                       incident.end());

        for (EdgeId e : incident) {
            if (has_edge(e)) {
                remove_edge(e);
            }
        }

        out_edges_.erase(id);
        in_edges_.erase(id);
        nodes_.erase(id);
    }

    std::vector<NodeId> vertices() const {
        std::vector<NodeId> result;
        result.reserve(nodes_.size());
        for (const auto& id : nodes_) {
            result.push_back(id);
        }
        return result;
    }

    std::size_t vertex_count() const {
        return nodes_.size();
    }

    EdgeId add_edge(NodeId from, NodeId to, Label label = {},
                    Weight weight = {}) {
        add_vertex(from);
        add_vertex(to);

        EdgeId id = static_cast<EdgeId>(edges_.size());
        edges_.push_back(Edge{from, to, weight, label});
        edge_active_.push_back(true);
        active_edge_count_++;

        if (directed_) {
            out_edges_[from].push_back(id);
            in_edges_[to].push_back(id);
        } else {
            out_edges_[from].push_back(id);
            out_edges_[to].push_back(id);
            in_edges_[from].push_back(id);
            in_edges_[to].push_back(id);
        }

        return id;
    }

    bool has_edge(EdgeId edge_id) const {
        if (edge_id < 0 ||
            static_cast<std::size_t>(edge_id) >= edges_.size()) {
            return false;
        }
        return edge_active_[static_cast<std::size_t>(edge_id)];
    }

    void remove_edge(EdgeId edge_id) {
        require_edge_active(edge_id);

        const Edge& e = edges_[static_cast<std::size_t>(edge_id)];
        edge_active_[static_cast<std::size_t>(edge_id)] = false;
        active_edge_count_--;

        if (directed_) {
            auto it_out = out_edges_.find(e.from);
            if (it_out != out_edges_.end()) {
                erase_edge_id(it_out->second, edge_id);
            }
            auto it_in = in_edges_.find(e.to);
            if (it_in != in_edges_.end()) {
                erase_edge_id(it_in->second, edge_id);
            }
        } else {
            auto it_out_from = out_edges_.find(e.from);
            if (it_out_from != out_edges_.end()) {
                erase_edge_id(it_out_from->second, edge_id);
            }
            auto it_out_to = out_edges_.find(e.to);
            if (it_out_to != out_edges_.end()) {
                erase_edge_id(it_out_to->second, edge_id);
            }
            auto it_in_from = in_edges_.find(e.from);
            if (it_in_from != in_edges_.end()) {
                erase_edge_id(it_in_from->second, edge_id);
            }
            auto it_in_to = in_edges_.find(e.to);
            if (it_in_to != in_edges_.end()) {
                erase_edge_id(it_in_to->second, edge_id);
            }
        }
    }

    Edge& edge(EdgeId edge_id) {
        require_edge_active(edge_id);
        return edges_[static_cast<std::size_t>(edge_id)];
    }

    const Edge& edge(EdgeId edge_id) const {
        require_edge_active(edge_id);
        return edges_[static_cast<std::size_t>(edge_id)];
    }

    std::size_t edge_count() const {
        return active_edge_count_;
    }

    const std::vector<EdgeId>& out_edge_ids(NodeId u) const {
        require_vertex_exists(u);
        auto it = out_edges_.find(u);
        if (it == out_edges_.end()) {
            return empty_edge_list();
        }
        return it->second;
    }

    const std::vector<EdgeId>& in_edge_ids(NodeId v) const {
        require_vertex_exists(v);
        auto it = in_edges_.find(v);
        if (it == in_edges_.end()) {
            return empty_edge_list();
        }
        return it->second;
    }

    NodeId other(EdgeId edge_id, NodeId u) const {
        require_edge_active(edge_id);
        require_vertex_exists(u);

        const Edge& e = edges_[static_cast<std::size_t>(edge_id)];
        if (directed_) {
            throw std::logic_error("other() is only valid for undirected graphs");
        }
        if (e.from == u) {
            return e.to;
        }
        if (e.to == u) {
            return e.from;
        }
        throw std::out_of_range("Vertex is not incident to edge");
    }

    std::vector<EdgeId> edges_between(NodeId u, NodeId v) const {
        require_vertex_exists(u);
        require_vertex_exists(v);

        std::vector<EdgeId> result;
        auto it = out_edges_.find(u);
        if (it == out_edges_.end()) {
            return result;
        }

        const auto& ids = it->second;
        for (EdgeId id : ids) {
            if (!has_edge(id)) {
                continue;
            }
            const Edge& e = edges_[static_cast<std::size_t>(id)];
            if (directed_) {
                if (e.from == u && e.to == v) {
                    result.push_back(id);
                }
            } else {
                if ((e.from == u && e.to == v) || (e.from == v && e.to == u)) {
                    result.push_back(id);
                }
            }
        }
        return result;
    }
};

