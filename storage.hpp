#pragma once
// Абстрактный интерфейс хранения графа и его реализации (список смежности/матрица)

#include "types.hpp"

#include <memory>
#include <vector>

namespace gd4 {

// Интерфейс, описывающий контракт любого хранилища графа
// Все операции работают с неориентированным простым графом (без петель и кратных рёбер)
class IGraphStorage {
   public:
    virtual ~IGraphStorage() = default;

    // Удаляет все вершины и рёбра
    virtual void clear() = 0;

    // Добавляет одну изолированную вершину, возвращает её индекс
    virtual VertexId add_vertex() = 0;

    // Гарантирует, что количество вершин не меньше need
    // Если need больше текущего числа, добавляет недостающие вершины (изолированные)
    // Используется при построении графа, когда заранее не известны все вершины
    virtual void ensure_vertices(int need) = 0;

    // Добавляет ребро между u и v, петли игнорируются
    // Если ребро уже существует, ничего не меняет и возвращает false
    // Возвращает true, если ребро было добавлено
    virtual bool add_edge(VertexId u, VertexId v) = 0;

    virtual bool has_vertex(VertexId v) const = 0;
    virtual bool has_edge(VertexId u, VertexId v) const = 0;

    virtual int vertex_count() const = 0;
    virtual long long undirected_edge_count() const = 0;   // каждое ребро считается один раз

    // Соседи вершины v (список в произвольном порядке
    virtual std::vector<int> neighbors(VertexId v) const = 0;
    virtual int degree(VertexId v) const = 0;

    // Создаёт пустое хранилище того же типа (используется при смене бэкенда)
    virtual std::unique_ptr<IGraphStorage> clone_prototype() const = 0;

    // Создаёт точную копию текущего хранилища со всем содержимым
    virtual std::unique_ptr<IGraphStorage> clone_graph() const = 0;
};

// Реализация на основе списков смежности
class AdjListStorage final : public IGraphStorage {
   public:
    void clear() override;
    VertexId add_vertex() override;
    void ensure_vertices(int need) override;
    bool add_edge(VertexId u, VertexId v) override;
    bool has_vertex(VertexId v) const override;
    bool has_edge(VertexId u, VertexId v) const override;
    int vertex_count() const override { return static_cast<int>(adj_.size()); }
    long long undirected_edge_count() const override { return m_; }
    std::vector<int> neighbors(VertexId v) const override;
    int degree(VertexId v) const override;
    std::unique_ptr<IGraphStorage> clone_prototype() const override;
    std::unique_ptr<IGraphStorage> clone_graph() const override;

   private:
    std::vector<std::vector<int>> adj_;   // adj_[v] — список соседей вершины v
    long long m_{0};                      // количество рёбер

    // Добавляет b в список соседей a, если b там ещё нет
    static void add_half_edge(std::vector<std::vector<int>>& g, int a, int b);
};

// Реализация на основе матрицы смежности (bool-матрица)
// Проверка ребра O(1), обход соседей O(n), память O(n²)
class AdjMatrixStorage final : public IGraphStorage {
   public:
    void clear() override;
    VertexId add_vertex() override;
    void ensure_vertices(int need) override;
    bool add_edge(VertexId u, VertexId v) override;
    bool has_vertex(VertexId v) const override;
    bool has_edge(VertexId u, VertexId v) const override;
    int vertex_count() const override { return n_; }
    long long undirected_edge_count() const override { return m_; }
    std::vector<int> neighbors(VertexId v) const override;
    int degree(VertexId v) const override;
    std::unique_ptr<IGraphStorage> clone_prototype() const override;
    std::unique_ptr<IGraphStorage> clone_graph() const override;

   private:
    int n_{0};                       // текущее количество вершин
    long long m_{0};                 // количество рёбер
    std::vector<std::vector<bool>> mat_;   // симметричная матрица
};

}  // namespace gd4
