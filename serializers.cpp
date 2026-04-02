#include "graphdro4/serializers.hpp"

#include "graphdro4/connectivity.hpp"
#include "graphdro4/traverse.hpp"

#include <functional>
#include <iomanip>
#include <set>
#include <sstream>

namespace gd4 {

// Палитра цветов для раскраски вершин 
static const char* kPalette[] = {
    "#e41a1c", "#377eb8", "#4daf4a", "#984ea3", "#ff7f00",
    "#ffff33", "#a65628", "#f781bf", "#999999", "#66c2a5"};
static const int kPaletteN = sizeof(kPalette) / sizeof(kPalette[0]);

// Случайное остовное дерево (лес) для каждой компоненты связности
// Алгоритм:
//   1. Случайно выбираем стартовую вершину (для связного графа можно любую)
//   2. Запускаем DFS, на каждом шаге перемешивая список соседей
//   3. Все рёбра, по которым мы впервые переходим в новую вершину, включаем в остов
//   4. Если граф несвязный, повторяем для оставшихся непосещённых вершин
// Результат – множество рёбер, образующее остовный лес
std::vector<std::pair<int, int>> random_spanning_tree_edges(const Graph& g, std::uint64_t seed) {
    std::vector<std::pair<int, int>> tree;
    int n = g.vertex_count();
    if (n == 0) return tree;
    std::mt19937_64 rng(seed);
    std::vector<char> vis(static_cast<size_t>(n), 0);
    std::function<void(int)> dfs = [&](int u) {
        vis[static_cast<size_t>(u)] = 1;
        auto nbr = g.neighbors(u);
        shuffle_inplace(nbr, rng);      // случайный порядок соседей
        for (int v : nbr) {
            if (!vis[static_cast<size_t>(v)]) {
                tree.push_back(norm_edge(u, v));
                dfs(v);
            }
        }
    };
    // Стартовая вершина выбирается случайно (но для связного графа результат не зависит от этого)
    int start = static_cast<int>(rng() % static_cast<std::uint64_t>(std::max(1, n)));
    dfs(start);
    // Запускаем DFS для всех непосещённых вершин (если граф несвязный)
    for (int s = 0; s < n; ++s) {
        if (!vis[static_cast<size_t>(s)]) dfs(s);
    }
    return tree;
}

namespace {

// Вспомогательная функция для поиска цикла: рекурсивный DFS, который при нахождении обратного ребра записывает концы цикла и возвращает true
// parent_arr хранит родителя в дереве DFS, по нему потом восстанавливается путь
bool dfs_cycle_collect(int v, int parent, const Graph& g, std::vector<char>& vis,
                       std::vector<int>& parent_arr, int& cycle_end, int& cycle_start,
                       std::mt19937_64& rng) {
    vis[static_cast<size_t>(v)] = 1;
    parent_arr[static_cast<size_t>(v)] = parent;
    std::vector<int> nbr = g.neighbors(v);
    shuffle_inplace(nbr, rng);          // случайный порядок соседей
    for (int to : nbr) {
        if (to == parent) continue;
        if (vis[static_cast<size_t>(to)]) {
            // Нашли обратное ребро: to уже посещена и не является родителем.
            cycle_end = v;
            cycle_start = to;
            return true;
        }
        if (dfs_cycle_collect(to, v, g, vis, parent_arr, cycle_end, cycle_start, rng)) return true;
    }
    return false;
}

}  // namespace

// Поиск случайного простого цикла в графе:
//   1. Случайно перемешиваем список вершин – они будут стартовыми точками поиска
//   2. Для каждой стартовой вершины запускаем DFS со случайным порядком соседей
//   3. Как только находим обратное ребро (не к родителю), восстанавливаем путь от cycle_end до cycle_start по parent_arr
//   4. Возвращаем список вершин цикла в порядке обхода (от cycle_start до cycle_end и обратно)
// Такой подход гарантирует, что на полном графе мы не всегда будем возвращать один и тот же треугольник, а будем находить разные циклы в зависимости от сида
std::vector<int> random_simple_cycle_vertices(const Graph& g, std::uint64_t seed) {
    int n = g.vertex_count();
    std::vector<int> order(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) order[static_cast<size_t>(i)] = i;
    std::mt19937_64 rng(seed);
    shuffle_inplace(order, rng);        // случайный порядок стартовых вершин
    for (int s : order) {
        std::vector<char> vis(static_cast<size_t>(n), 0);
        std::vector<int> parent_arr(static_cast<size_t>(n), -2);
        int ce = -1, cs = -1;
        if (dfs_cycle_collect(s, -1, g, vis, parent_arr, ce, cs, rng)) {
            // Восстанавливаем путь от ce к cs (обратный)
            std::vector<int> cyc;
            int x = ce;
            cyc.push_back(x);
            while (x != cs) {
                x = parent_arr[static_cast<size_t>(x)];
                cyc.push_back(x);
            }
            // Переворачиваем, чтобы получить порядок от cs к ce
            std::reverse(cyc.begin(), cyc.end());
            return cyc;
        }
    }
    return {};   // цикл не найден 
}

// Конструктор сериализатора DOT с заданными опциями
DotSerializer::DotSerializer(DotSerializeOptions opt) : opt_(std::move(opt)) {}

// Генерация DOT-описания графа
// Дополнительные возможности:
//   - Раскраска вершин (если задан vertex_color)
//   - Выделение кластеров рёберной/вершинной двусвязности (subgraph cluster_*)
//   - Подсветка случайного остова (золотистый цвет, penwidth=2)
//   - Подсветка случайного цикла (красный цвет, penwidth=2)
void DotSerializer::write(const Graph& g, std::ostream& out) const {
    out << "strict graph G {\n";
    out << "  graph [splines=true overlap=false];\n";
    int n = g.vertex_count();
    // Заголовки вершин с возможной заливкой
    for (int v = 0; v < n; ++v) {
        out << "  " << v << " [label=\"" << v << "\"";
        if (static_cast<int>(opt_.vertex_color.size()) > v) {
            int ci = opt_.vertex_color[static_cast<size_t>(v)] % kPaletteN;
            out << ", fillcolor=" << kPalette[ci] << ", style=filled, fontcolor=white";
        }
        out << "];\n";
    }

    // Множества рёбер для подсветки остова и цикла
    std::set<std::pair<int, int>> stree, cyc_edges;
    if (opt_.show_random_spanning_tree) {
        auto te = random_spanning_tree_edges(g, opt_.rng_seed);
        for (auto& e : te) stree.insert(e);
    }
    if (opt_.show_random_simple_cycle) {
        // Для цикла используем другой seed (сдвиг), чтобы не совпадал с остовом 
        auto cyc = random_simple_cycle_vertices(g, opt_.rng_seed ^ 0x9E3779B97F4A7C15ULL);
        if (cyc.size() >= 2) {
            for (size_t i = 0; i + 1 < cyc.size(); ++i) {
                cyc_edges.insert(norm_edge(cyc[i], cyc[i + 1]));
            }
            cyc_edges.insert(norm_edge(cyc.front(), cyc.back()));
        }
    }

    // Лямбда для вывода ребра с дополнительными атрибутами (цвет, толщина)
    auto emit_edge = [&](int u, int v, const std::string& extra) {
        int a = u, b = v;
        if (a > b) std::swap(a, b);
        out << "  " << a << " -- " << b;
        if (!extra.empty()) out << " [" << extra << "]";
        out << ";\n";
    };

    // Кластеры рёберной двусвязности
    if (opt_.cluster_edge_biconnected) {
        auto comps = edge_biconnected_components(g);
        int cid = 0;
        for (const auto& comp : comps) {
            out << "  subgraph cluster_ebcc" << cid++ << " {\n";
            out << "    style=filled; color=lightgrey; label=\"ребёрно-двусвязный класс\";\n";
            for (int v : comp) out << "    " << v << ";\n";
            out << "  }\n";
        }
    }

    // Кластеры вершинной двусвязности (блоки)
    if (opt_.cluster_vertex_biconnected) {
        auto blocks = vertex_biconnected_blocks(g);
        int bid = 0;
        for (const auto& block : blocks) {
            std::set<int> verts;
            for (auto e : block) {
                verts.insert(e.first);
                verts.insert(e.second);
            }
            out << "  subgraph cluster_vbcc" << bid++ << " {\n";
            out << "    style=dashed; color=blue; label=\"блок вершинной 2-связности\";\n";
            for (int v : verts) out << "    " << v << ";\n";
            out << "  }\n";
        }
    }

    // Вывод всех рёбер графа с учётом атрибутов
    for (int u = 0; u < n; ++u) {
        for (int v : g.neighbors(u)) {
            if (v < u) continue;   // каждое ребро выводим один раз
            std::string attrs;
            if (stree.count(norm_edge(u, v))) {
                attrs += "color=goldenrod, penwidth=2";
            }
            if (cyc_edges.count(norm_edge(u, v))) {
                if (!attrs.empty()) attrs += ", ";
                attrs += "color=red, penwidth=2";
            }
            emit_edge(u, v, attrs);
        }
    }
    out << "}\n";
}

// Сериализатор в формат .edges
// Простой список рёбер в виде "u -- v" (каждая пара на новой строке)
void Program4YouEdgesSerializer::write(const Graph& g, std::ostream& out) const {
    for (int u = 0; u < g.vertex_count(); ++u) {
        for (int v : g.neighbors(u)) {
            if (v < u) continue;
            out << u << " -- " << v << "\n";
        }
    }
}

// Сериализатор в JSON (список рёбер).
// Удобен для импорта в скрипты (питончееек)
// Формат: {"n": количество_вершин, "edges": [[u1,v1], [u2,v2], ...]}
void JsonEdgeListSerializer::write(const Graph& g, std::ostream& out) const {
    out << "{\"n\":" << g.vertex_count() << ",\"edges\":[";
    bool first = true;
    for (int u = 0; u < g.vertex_count(); ++u) {
        for (int v : g.neighbors(u)) {
            if (v < u) continue;
            if (!first) out << ',';
            first = false;
            out << '[' << u << ',' << v << ']';
        }
    }
    out << "]}\n";
}

}  // namespace gd4