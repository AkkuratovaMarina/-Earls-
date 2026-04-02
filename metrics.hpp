#pragma once
// Метрики графа с простым кешем (после вызова invalidate() пересчитываем заново)

#include "graph.hpp"

#include <cstdint>
#include <optional>

namespace gd4 {

class GraphMetrics {
   public:
    explicit GraphMetrics(const Graph& g);

    void invalidate();

    double density() const;
    int diameter() const;
    // Транзитивность: 3*T/W, где T — число треугольников, W = сумма_v C(deg(v),2) (число триад)
    double transitivity() const;
    int components() const;
    int articulation_count() const;
    int bridge_count() const;
    bool is_bipartite() const;
    // Верхняя оценка хроматического числа жадной раскраской (по убыванию степени)
    int greedy_chromatic_upper_bound() const;

   private:
    const Graph& g_;
    mutable std::optional<double> density_;
    mutable std::optional<int> diameter_;
    mutable std::optional<double> transitivity_;
    mutable std::optional<int> components_;
    mutable std::optional<int> articulation_;
    mutable std::optional<int> bridges_;
    mutable std::optional<bool> bipartite_;
    mutable std::optional<int> greedy_chi_;
};

}  // namespace gd4
