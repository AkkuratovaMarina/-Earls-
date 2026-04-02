#pragma once
// Точки сочленения, мосты, компоненты рёберной или вершинной двусвязности (алгоритмы Тарьяна)
#include "graph.hpp"
#include "types.hpp"

#include <utility>
#include <vector>

namespace gd4 {

struct TarjanCutResult {
    std::vector<char> is_articulation;              // по вершинам
    std::vector<std::pair<int, int>> bridges;      // ненаправленные, u<v
    int articulation_count() const;
    int bridge_count() const { return static_cast<int>(bridges.size()); }
};

// Детерминированный DFS Тарьяна (порядок соседей теперь отсортированный).
TarjanCutResult tarjan_cuts(const Graph& g);


//Проверка устойчивости: несколько раз строим изоморфную копию g случайной перестановкой вершин
//и сравниваем множество мостов (в исходных номерах) с tarjan_cuts(g).bridges
//trials — число случайных перестановок; seed — инициализация RNG

bool bridges_invariant_under_random_shuffles(const Graph& g, int trials, std::uint64_t seed);

// Компоненты рёберной двусвязности: вектор множеств вершин (или метки comp_id на рёбрах)
std::vector<std::vector<int>> edge_biconnected_components(const Graph& g);

// Блоки (вершинная двусвязность): списки рёбер каждого блока (каждое ребро вхождение u<v)
std::vector<std::vector<std::pair<int, int>>> vertex_biconnected_blocks(const Graph& g);

// Число компонент связности 
int count_connected_components(const Graph& g);

} 