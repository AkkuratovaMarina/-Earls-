// AdjListStorage:
//   - Хранит для каждой вершины список её соседей (std::vector<std::vector<int>>)
//   - Количество рёбер m_ обновляется при успешном добавлении
//   - Проверка ребра (has_edge) выполняется линейным поиском в списке (O(deg))
//   - Добавление ребра проверяет существование, чтобы избежать кратных рёбер
// AdjMatrixStorage:
//   - Использует симметричную булеву матрицу (std::vector<std::vector<bool>>)
//   - Проверка ребра – O(1), обход соседей – O(n)
//   - При добавлении вершины матрица расширяется до (n+1)×(n+1)
//   - ensure_vertices расширяет матрицу до нужного размера (если need > n)

// clone_prototype возвращает пустое хранилище того же типа (для смены бэкенда).
// clone_graph возвращает полную копию текущего хранилища.

#include "graphdro4/storage.hpp"

#include <algorithm>

namespace gd4 {

//AdjListStorage

// Очищает хранилище: удаляет все вершины и рёбра
void AdjListStorage::clear() {
    adj_.clear();
    m_ = 0;
}

// Добавляет одну изолированную вершину
// Возвращает индекс новой вершины (равен предыдущему размеру adj_)
VertexId AdjListStorage::add_vertex() {
    adj_.emplace_back();
    return static_cast<VertexId>(adj_.size() - 1);
}

// Гарантирует, что число вершин не меньше need
// Если need больше текущего размера, добавляет недостающие изолированные вершины
void AdjListStorage::ensure_vertices(int need) {
    if (need < 0) return;
    while (static_cast<int>(adj_.size()) < need) {
        adj_.emplace_back();
    }
}

// Добавляет b в список соседей a, если b там ещё нет
void AdjListStorage::add_half_edge(std::vector<std::vector<int>>& g, int a, int b) {
    auto& row = g[static_cast<size_t>(a)];
    if (std::find(row.begin(), row.end(), b) == row.end()) row.push_back(b);
}

// Добавляет ребро (u, v) в граф
// Возвращает true, если ребро добавлено (не было петли и кратного ребра)
bool AdjListStorage::add_edge(VertexId u, VertexId v) {
    if (u == v) return false;                     // петли запрещены
    if (!has_vertex(u) || !has_vertex(v)) return false;
    if (has_edge(u, v)) return false;             // кратное ребро не добавляем
    add_half_edge(adj_, u, v);
    add_half_edge(adj_, v, u);
    ++m_;
    return true;
}

// Проверяет существование вершины
bool AdjListStorage::has_vertex(VertexId v) const {
    return v >= 0 && v < static_cast<VertexId>(adj_.size());
}

// Проверяет существование ребра (u, v)
bool AdjListStorage::has_edge(VertexId u, VertexId v) const {
    if (!has_vertex(u) || !has_vertex(v) || u == v) return false;
    const auto& row = adj_[static_cast<size_t>(u)];
    return std::find(row.begin(), row.end(), v) != row.end();
}

// Возвращает список соседей вершины v
std::vector<int> AdjListStorage::neighbors(VertexId v) const {
    if (!has_vertex(v)) return {};
    return adj_[static_cast<size_t>(v)];
}

// Степень вершины v (количество соседей)
int AdjListStorage::degree(VertexId v) const {
    return has_vertex(v) ? static_cast<int>(adj_[static_cast<size_t>(v)].size()) : 0;
}

// Создаёт пустое хранилище того же типа (для смены бэкенда)
std::unique_ptr<IGraphStorage> AdjListStorage::clone_prototype() const {
    return std::make_unique<AdjListStorage>();
}

// Создаёт полную копию текущего хранилища
std::unique_ptr<IGraphStorage> AdjListStorage::clone_graph() const {
    auto p = std::make_unique<AdjListStorage>();
    p->adj_ = adj_;
    p->m_ = m_;
    return p;
}

//AdjMatrixStorage

// Очищает хранилище: удаляет матрицу, обнуляет счётчики
void AdjMatrixStorage::clear() {
    mat_.clear();
    n_ = 0;
    m_ = 0;
}

// Добавляет одну изолированную вершину
// Увеличивает матрицу до (n+1)×(n+1), копируя старые значения
VertexId AdjMatrixStorage::add_vertex() {
    int old = n_;
    std::vector<std::vector<bool>> nm(static_cast<size_t>(n_ + 1),
                                        std::vector<bool>(static_cast<size_t>(n_ + 1), false));
    for (int i = 0; i < n_; ++i) {
        for (int j = 0; j < n_; ++j) {
            nm[static_cast<size_t>(i)][static_cast<size_t>(j)] =
                mat_[static_cast<size_t>(i)][static_cast<size_t>(j)];
        }
    }
    mat_ = std::move(nm);
    ++n_;
    return old;
}

// Гарантирует, что число вершин не меньше need
// Расширяет матрицу до need×need, если need > n
void AdjMatrixStorage::ensure_vertices(int need) {
    if (need <= n_) return;
    std::vector<std::vector<bool>> nm(static_cast<size_t>(need),
                                        std::vector<bool>(static_cast<size_t>(need), false));
    for (int i = 0; i < n_; ++i) {
        for (int j = 0; j < n_; ++j) {
            nm[static_cast<size_t>(i)][static_cast<size_t>(j)] =
                mat_[static_cast<size_t>(i)][static_cast<size_t>(j)];
        }
    }
    mat_ = std::move(nm);
    n_ = need;
}

// Добавляет ребро (u, v) в граф
// Возвращает true, если ребро добавлено (не было петли и кратного ребра)
bool AdjMatrixStorage::add_edge(VertexId u, VertexId v) {
    if (u == v) return false;
    if (!has_vertex(u) || !has_vertex(v)) return false;
    if (mat_[static_cast<size_t>(u)][static_cast<size_t>(v)]) return false; // уже есть
    mat_[static_cast<size_t>(u)][static_cast<size_t>(v)] = true;
    mat_[static_cast<size_t>(v)][static_cast<size_t>(u)] = true;
    ++m_;
    return true;
}

// Проверяет существование вершины
bool AdjMatrixStorage::has_vertex(VertexId v) const {
    return v >= 0 && v < n_;
}

// Проверяет существование ребра 
bool AdjMatrixStorage::has_edge(VertexId u, VertexId v) const {
    if (!has_vertex(u) || !has_vertex(v) || u == v) return false;
    return mat_[static_cast<size_t>(u)][static_cast<size_t>(v)];
}

// Возвращает список соседей вершины v (проход по всей строке матрицы)
std::vector<int> AdjMatrixStorage::neighbors(VertexId v) const {
    std::vector<int> out;
    if (!has_vertex(v)) return out;
    for (int u = 0; u < n_; ++u) {
        if (u != v && mat_[static_cast<size_t>(v)][static_cast<size_t>(u)]) out.push_back(u);
    }
    return out;
}

// Степень вершины v (количество ненулевых элементов в строке v)
int AdjMatrixStorage::degree(VertexId v) const {
    if (!has_vertex(v)) return 0;
    int d = 0;
    for (int u = 0; u < n_; ++u) {
        if (u != v && mat_[static_cast<size_t>(v)][static_cast<size_t>(u)]) ++d;
    }
    return d;
}

// Создаёт пустое хранилище того же типа (для смены бэкенда)
std::unique_ptr<IGraphStorage> AdjMatrixStorage::clone_prototype() const {
    return std::make_unique<AdjMatrixStorage>();
}

// Создаёт полную копию текущего хранилища
std::unique_ptr<IGraphStorage> AdjMatrixStorage::clone_graph() const {
    auto p = std::make_unique<AdjMatrixStorage>();
    p->n_ = n_;
    p->m_ = m_;
    p->mat_ = mat_;
    return p;
}

}  // namespace gd4
