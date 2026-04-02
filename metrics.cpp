// Метрики графа: плотность, диаметр, транзитивность, компоненты связности, точки сочленения, мосты, двудольность, верхняя оценка хроматического числа
// Реализация с ленивым кэшированием: результат вычисления сохраняется в std::optional
// При повторном запросе (без изменений графа) возвращается кэшированное значение

#include "graphdro4/metrics.hpp"
#include "graphdro4/connectivity.hpp"
#include <algorithm>
#include <queue>

namespace gd4 {

GraphMetrics::GraphMetrics(const Graph& g) : g_(g) {}

// Сбрасывает все кэшированные значения
void GraphMetrics::invalidate() {
    density_.reset();
    diameter_.reset();
    transitivity_.reset();
    components_.reset();
    articulation_.reset();
    bridges_.reset();
    bipartite_.reset();
    greedy_chi_.reset();
}

// Плотность графа = 2*m / (n*(n-1))
// Для n <= 1 плотность = 0
double GraphMetrics::density() const {
    if (density_) return *density_;
    long long n = g_.vertex_count();
    if (n <= 1) return density_.emplace(0.0);
    long long m = g_.edge_count();
    double denom = static_cast<double>(n) * (n - 1) / 2.0;
    return density_.emplace(static_cast<double>(m) / denom);
}

// Диаметр графа - максимальное расстояние между любыми двумя вершинами
// Для каждой вершины запускает BFS и находит максимальное расстояние
int GraphMetrics::diameter() const {
    if (diameter_) return *diameter_;
    int n = g_.vertex_count();
    if (n == 0) return diameter_.emplace(0);
    int best = 0;
    for (int s = 0; s < n; ++s) {
        std::vector<int> dist(static_cast<size_t>(n), -1);
        std::queue<int> q;
        dist[static_cast<size_t>(s)] = 0;
        q.push(s);
        while (!q.empty()) {
            int u = q.front();
            q.pop();
            for (int v : g_.neighbors(u)) {
                if (dist[static_cast<size_t>(v)] == -1) {
                    dist[static_cast<size_t>(v)] = dist[static_cast<size_t>(u)] + 1;
                    q.push(v);
                }
            }
        }
        for (int d : dist) best = std::max(best, d);
    }
    return diameter_.emplace(best);
}

// Транзитивность = 3*(число треугольников) / (число триад)
// Триада – пара смежных рёбер (вершина с двумя соседями)
// Подсчёт треугольников: для каждой вершины v перебираем пары её соседей (u, w), если u и w смежны – найден треугольник (v,u,w). Каждый треугольник будет учтён 3 раза, поэтому в конце умножаем на 3
double GraphMetrics::transitivity() const {
    if (transitivity_) return *transitivity_;
    int n = g_.vertex_count();
    long long triads = 0;
    long long triangles = 0;
    for (int v = 0; v < n; ++v) {
        int deg = g_.degree(v);
        triads += 1LL * deg * (deg - 1) / 2;          // количество триад с центром v
        // Считаем треугольники, в которых v – наименьший номер 
        for (int u : g_.neighbors(v)) {
            if (u <= v) continue;
            for (int w : g_.neighbors(v)) {
                if (w <= u) continue;
                if (g_.has_edge(u, w)) ++triangles;
            }
        }
    }
    if (triads == 0) return transitivity_.emplace(0.0);
    return transitivity_.emplace(static_cast<double>(3 * triangles) / static_cast<double>(triads));
}

// Число компонент связности 
int GraphMetrics::components() const {
    if (components_) return *components_;
    return components_.emplace(count_connected_components(g_));
}

// Число точек сочленения (шарниров) – через алгоритм Тарьяна
int GraphMetrics::articulation_count() const {
    if (articulation_) return *articulation_;
    return articulation_.emplace(tarjan_cuts(g_).articulation_count());
}

// Число мостов – через алгоритм Тарьяна
int GraphMetrics::bridge_count() const {
    if (bridges_) return *bridges_;
    return bridges_.emplace(tarjan_cuts(g_).bridge_count());
}

// Проверка двудольности графа (BFS-раскраска в два цвета)
bool GraphMetrics::is_bipartite() const {
    if (bipartite_) return *bipartite_;
    int n = g_.vertex_count();
    std::vector<int> color(static_cast<size_t>(n), -1);
    for (int s = 0; s < n; ++s) {
        if (color[static_cast<size_t>(s)] != -1) continue;
        color[static_cast<size_t>(s)] = 0;
        std::queue<int> q;
        q.push(s);
        while (!q.empty()) {
            int u = q.front();
            q.pop();
            for (int v : g_.neighbors(u)) {
                if (color[static_cast<size_t>(v)] == -1) {
                    color[static_cast<size_t>(v)] = 1 - color[static_cast<size_t>(u)];
                    q.push(v);
                } else if (color[static_cast<size_t>(v)] == color[static_cast<size_t>(u)]) {
                    return bipartite_.emplace(false); // конфликт цветов – граф не двудольный
                }
            }
        }
    }
    return bipartite_.emplace(true);
}

// Верхняя оценка хроматического числа жадной раскраской (алгоритм Уэлша–Пауэлла)
// Сортируем вершины по убыванию степени, затем раскрашиваем по очереди, каждую вершину красим в минимальный доступный цвет (не как у соседей)
int GraphMetrics::greedy_chromatic_upper_bound() const {
    if (greedy_chi_) return *greedy_chi_;
    int n = g_.vertex_count();
    if (n == 0) return greedy_chi_.emplace(0);
    // Сортировка вершин по убыванию степени
    std::vector<int> order(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) order[static_cast<size_t>(i)] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        return g_.degree(a) > g_.degree(b);
    });
    std::vector<int> col(static_cast<size_t>(n), -1);
    int maxc = 0;
    for (int v : order) {
        std::vector<char> used(static_cast<size_t>(n + 3), 0); // метки занятых цветов
        for (int u : g_.neighbors(v)) {
            int c = col[static_cast<size_t>(u)];
            if (c >= 0 && c < static_cast<int>(used.size())) used[static_cast<size_t>(c)] = 1;
        }
        int c = 0;
        while (c < static_cast<int>(used.size()) && used[static_cast<size_t>(c)]) ++c;
        col[static_cast<size_t>(v)] = c;
        maxc = std::max(maxc, c);
    }
    return greedy_chi_.emplace(maxc + 1);
}

}  // namespace gd4