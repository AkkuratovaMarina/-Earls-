#pragma once
// «Graph» - единая точка входа для алгоритмов над неориентированным простым графом.
// Операции делегируются IGraphStorage (список смежности или матрица)
// set_backend пересоздаёт хранилище и переносит рёбра (структура графа сохраняется)

#include "storage.hpp"
#include "types.hpp"

#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace gd4 {

enum class StorageKind { AdjacencyList, AdjacencyMatrix };

std::unique_ptr<IGraphStorage> make_storage(StorageKind k);

class Graph {
   public:
    explicit Graph(StorageKind kind = StorageKind::AdjacencyList);

    // Копия графа с тем же типом хранилища
    Graph clone() const;

    StorageKind backend_kind() const { return kind_; }

    void clear();

    VertexId add_vertex();
    void add_vertices(int count);  // добавить счётчик изолированных вершин

    // Добавить ребро, при необходимости расширяет число вершин до max(u,v)+1.
    bool add_edge(VertexId u, VertexId v);

    bool has_vertex(VertexId v) const { return st_->has_vertex(v); }
    bool has_edge(VertexId u, VertexId v) const { return st_->has_edge(u, v); }

    int vertex_count() const { return st_->vertex_count(); }
    long long edge_count() const { return st_->undirected_edge_count(); }

    std::vector<int> neighbors(VertexId v) const { return st_->neighbors(v); }
    int degree(VertexId v) const { return st_->degree(v); }

    bool is_leaf(VertexId v) const;

    // Дизъюнктное объединение (вершины other перенумеровываются)
    void disjoint_union_with(const Graph& other, VertexId offset_for_other);

    // Перенумерация: new_id[v] = образ старой v; размер = vertex_count().
    Graph permute_vertices(const std::vector<int>& new_id_of_old) const;

    // Сменить внутреннюю структуру, сохранив граф изоморфно (копия рёбер в новый бэкенд)
    void set_backend(StorageKind new_kind);

    const IGraphStorage& storage() const { return *st_; }
    IGraphStorage& storage() { return *st_; }

   private:
    StorageKind kind_;
    std::unique_ptr<IGraphStorage> st_;

    void adopt_storage(std::unique_ptr<IGraphStorage> s, StorageKind k);
};

// Обход с настраиваемым порядком соседей (случайный DFS)
using NeighborOrder = std::function<void(std::vector<int>&)>;

}  // namespace gd4
