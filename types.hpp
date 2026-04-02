#pragma once
// Базовые типы и вспомогательные функции для работы с графами
// Граф неориентированный, простой (без петель и кратных рёбер)

#include <algorithm>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace gd4 {

// Тип для идентификатора вершины (целое число)
using VertexId = int;

// По соглашению, если где-то хранится ребро как пара (u, v), то u всегда меньше v (чтобы избежать дублирования).
using Edge = std::pair<VertexId, VertexId>;

// Приводит ребро к каноническому виду (u < v)
inline Edge norm_edge(VertexId u, VertexId v) {
    if (u > v) std::swap(u, v);
    return {u, v};
}

// Перемешивает элементы вектора a с помощью генератора случайных чисел rng
// Используется для случайного порядка обхода в DFS (например, для случайного остова или цикла)
inline void shuffle_inplace(std::vector<int>& a, std::mt19937_64& rng) {
    std::shuffle(a.begin(), a.end(), rng);
}

}  // namespace gd4
