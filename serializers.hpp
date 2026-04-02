#pragma once
// Форматы:
// GraphViz DOT (стандарт для визуализации)
// .edges (формат для Programforyou)
//  JSON-список рёбер (ееееее)
// Для DOT-вывода:
// подсветка вершин заданными цветами (по индексу в палитре)
// выделение компонент рёберной/вершинной двусвязности в виде кластеров
// отображение случайного остова (жёлтым цветом)
// отображение случайного простого цикла (красным цветом)

#include "graph.hpp"

#include <ostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace gd4 {

// Настройки для экспорта в DOT.
struct DotSerializeOptions {
    // Цвета вершин: для каждой вершины индекс цвета из палитры 
    // Если вектор пуст, вершины не раскрашиваются
    std::vector<int> vertex_color;

    // Выделять компоненты рёберной двусвязности как отдельные кластеры (subgraph)
    bool cluster_edge_biconnected = false;
    
    // Выделять блоки вершинной двусвязности как отдельные кластеры (subgraph)
    bool cluster_vertex_biconnected = false;

    // Подсветка случайного остова (жёлтые рёбра) и случайного цикла (красные рёбра)
    bool show_random_spanning_tree = false;
    bool show_random_simple_cycle = false;
    
    // Начальное значение для генератора случайных чисел
    std::uint64_t rng_seed = 42;
};

// Интерфейс для всех сериализаторов
class GraphSerializerBase {
   public:
    virtual ~GraphSerializerBase() = default;
    virtual std::string format_name() const = 0;
    virtual void write(const Graph& g, std::ostream& out) const = 0;
};

// Сериализатор в формат GraphViz DOT
class DotSerializer final : public GraphSerializerBase {
   public:
    explicit DotSerializer(DotSerializeOptions opt = {});

    std::string format_name() const override { return "graphviz_dot"; }

    // Записывает граф в формате DOT в выходной поток
    void write(const Graph& g, std::ostream& out) const override;

   private:
    DotSerializeOptions opt_;
};

// Сериализатор в формат .edges 
// Каждая строка содержит "u -- v" для каждого ребра
class Program4YouEdgesSerializer final : public GraphSerializerBase {
   public:
    std::string format_name() const override { return "program4you_edges"; }
    void write(const Graph& g, std::ostream& out) const override;
};

// Компактный JSON со списком рёбер.
// Пример вывода: {"n":4,"edges":[[0,1],[1,2],[2,3]]} 
class JsonEdgeListSerializer final : public GraphSerializerBase {
   public:
    std::string format_name() const override { return "json_edges"; }
    void write(const Graph& g, std::ostream& out) const override;
};

// Возвращает список рёбер случайного остовного дерева (леса) для каждой компоненты связности
// Алгоритм: случайный DFS (порядок соседей перемешивается с помощью RNG)
std::vector<std::pair<int, int>> random_spanning_tree_edges(const Graph& g, std::uint64_t seed);

// Возвращает список вершин простого цикла (в порядке обхода) или пустой список, если цикл не найден
// Алгоритм: перебирает стартовые вершины в случайном порядке, запускает DFS с перемешиванием соседей
std::vector<int> random_simple_cycle_vertices(const Graph& g, std::uint64_t seed);

}  // namespace gd4
