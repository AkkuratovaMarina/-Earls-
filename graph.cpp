// Реализация основных операций графа
// Класс Graph делегирует хранение рёбер конкретному бэкенду (список смежности или матрица), все операции с вершинами и рёбрами проходят через st_
// add_edge:
// петли игнорируются
// перед добавлением ребра вызывается ensure_vertices, чтобы расширить хранилище до max(u,v)+1 
// само добавление выполняет бэкенд (он же проверяет наличие кратных рёбер)
// set_backend:
// создаёт временный граф с новым типом хранилища, копирует в него все рёбра (в каноническом виде u < v), затем заменяет st_
// permute_vertices:
// принимает перестановку old_vertex -> new_vertex и строит новый граф, в котором вершины перенумерованы согласно этой перестановке
// перед использованием проверяет, что new_id_of_old задаёт корректную перестановку
// disjoint_union_with:
// добавляет к текущему графу копию other, начиная с номера offset_for_other
// используется для объединения двух графов с непересекающимися множествами вершин

#include "graphdro4/graph.hpp"

#include <stdexcept>

namespace gd4 {

// Создаёт пустое хранилище заданного типа
std::unique_ptr<IGraphStorage> make_storage(StorageKind k) {
    switch (k) {
        case StorageKind::AdjacencyList:
            return std::make_unique<AdjListStorage>();
        case StorageKind::AdjacencyMatrix:
            return std::make_unique<AdjMatrixStorage>();
    }
    return std::make_unique<AdjListStorage>();
}

// Конструктор: создаёт граф с пустым хранилищем указанного типа
Graph::Graph(StorageKind kind) : kind_(kind), st_(make_storage(kind)) {}

// Полная копия графа (новое хранилище того же типа, с теми же данными)
Graph Graph::clone() const {
    Graph g(kind_);
    g.st_ = st_->clone_graph();
    return g;
}

// Заменяет внутреннее хранилище на переданное
void Graph::adopt_storage(std::unique_ptr<IGraphStorage> s, StorageKind k) {
    st_ = std::move(s);
    kind_ = k;
}

// Удаляет все вершины и рёбра
void Graph::clear() { st_->clear(); }

// Добавляет одну изолированную вершину
VertexId Graph::add_vertex() { return st_->add_vertex(); }

// Добавляет count изолированных вершин
void Graph::add_vertices(int count) {
    for (int i = 0; i < count; ++i) st_->add_vertex();
}

// Добавляет ребро между u и v (сначала расширяет хранилище, если нужно)
bool Graph::add_edge(VertexId u, VertexId v) {
    if (u == v) return false; // петли не разрешены
    int need = std::max(u, v) + 1; // необходимое число вершин
    st_->ensure_vertices(need); // дорасширяем при необходимости
    return st_->add_edge(u, v); // бэкенд сам проверит кратность
}

// Проверяет, является ли вершина листом (степень = 1)
bool Graph::is_leaf(VertexId v) const {
    return has_vertex(v) && degree(v) == 1;
}

// Дизъюнктное объединение с другим графом: копирует рёбра other, смещая все номера вершин на offset_for_other
void Graph::disjoint_union_with(const Graph& other, VertexId offset_for_other) {
    int on = other.vertex_count();
    int need = static_cast<int>(offset_for_other) + on;
    st_->ensure_vertices(need);
    for (VertexId u = 0; u < on; ++u) {
        for (int v : other.neighbors(u)) {
            if (v < static_cast<int>(u)) continue; // каждое ребро добавляем один раз
            st_->add_edge(offset_for_other + u, offset_for_other + v);
        }
    }
}

// Перенумеровывает вершины согласно перестановке new_id_of_old
// Возвращает новый граф с новыми номерами
Graph Graph::permute_vertices(const std::vector<int>& new_id_of_old) const {
    int n = vertex_count();
    if (static_cast<int>(new_id_of_old.size()) != n) {
        throw std::runtime_error("permute_vertices: размер отображения != числу вершин");
    }
    // Проверяем, что new_id_of_old задаёт перестановку (все числа от 0 до n-1, без повторений)
    std::vector<char> seen(static_cast<size_t>(n), 0);
    for (int x : new_id_of_old) {
        if (x < 0 || x >= n) throw std::runtime_error("permute_vertices: неверный новый id");
        if (seen[static_cast<size_t>(x)]) throw std::runtime_error("permute_vertices: не перестановка");
        seen[static_cast<size_t>(x)] = 1;
    }
    Graph g(kind_);
    g.st_->ensure_vertices(n);
    // Копируем рёбра, преобразуя вершины согласно перестановке
    for (VertexId u = 0; u < n; ++u) {
        for (int v : neighbors(u)) {
            if (v <= static_cast<int>(u)) continue; // каждое ребро один раз
            int nu = new_id_of_old[static_cast<size_t>(u)];
            int nv = new_id_of_old[static_cast<size_t>(v)];
            g.add_edge(nu, nv);
        }
    }
    return g;
}

// Смена внутреннего хранилища на другой тип (список -> матрица или наоборот)
void Graph::set_backend(StorageKind new_kind) {
    if (new_kind == kind_) return; // ничего не делаем, если тип не меняется
    Graph tmp(new_kind); // временный граф с новым типом
    int n = vertex_count();
    tmp.st_->ensure_vertices(n); // выделяем память под n вершин
    for (VertexId u = 0; u < n; ++u) {
        for (int v : neighbors(u)) {
            if (v > static_cast<int>(u)) // добавляем каждое ребро один раз
                tmp.st_->add_edge(u, v);
        }
    }
    adopt_storage(std::move(tmp.st_), new_kind);
}

}  // namespace gd4