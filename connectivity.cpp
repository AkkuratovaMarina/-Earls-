#include "graphdro4/connectivity.hpp"

#include <algorithm>
#include <functional>
#include <numeric>
#include <random>
#include <set>
#include <stack>

namespace gd4 {

// Считаем количество точек сочленения (ненулевых флагов)
int TarjanCutResult::articulation_count() const {
    int c = 0;
    for (char x : is_articulation) c += (x != 0);
    return c;
}

// Алгоритм Тарьяна: поиск мостов и точек сочленения
TarjanCutResult tarjan_cuts(const Graph& g) {
    int n = g.vertex_count();
    TarjanCutResult r;
    r.is_articulation.assign(static_cast<size_t>(n), 0);
    if (n == 0) return r;

    std::vector<int> tin(static_cast<size_t>(n), -1);  // время входа в вершину
    std::vector<int> low(static_cast<size_t>(n), -1);  // минимальный tin среди достижимых по обратному ребру
    int timer = 0;

    // Рекурсивный DFS с обновлением tin/low
    // parent - вершина, из которой пришли, или -1 для корня
    std::function<void(int, int)> dfs = [&](int v, int parent) {
        tin[static_cast<size_t>(v)] = low[static_cast<size_t>(v)] = timer++;
        int children = 0;
        for (int to : g.neighbors(v)) {
            if (to == parent) continue; // не идём назад к родителю

            if (tin[static_cast<size_t>(to)] != -1) { // обратное ребро к уже посещённой вершине
                low[static_cast<size_t>(v)] = std::min(low[static_cast<size_t>(v)], tin[static_cast<size_t>(to)]);
            } else { // древесное ребро
                dfs(to, v);
                low[static_cast<size_t>(v)] = std::min(low[static_cast<size_t>(v)], low[static_cast<size_t>(to)]);

                // Условие моста: из поддерева to нет обратного ребра выше v
                if (low[static_cast<size_t>(to)] > tin[static_cast<size_t>(v)]) {
                    r.bridges.push_back(norm_edge(v, to));
                }

                // Условие точки сочленения для не-корня из поддерева to нет обратного ребра выше v (включая v)
                if (low[static_cast<size_t>(to)] >= tin[static_cast<size_t>(v)] && parent != -1) {
                    r.is_articulation[static_cast<size_t>(v)] = 1;
                }
                ++children;
            }
        }
        // Корень – точка сочленения, если у него больше одного ребёнка
        if (parent == -1 && children > 1) r.is_articulation[static_cast<size_t>(v)] = 1;
    };

    // Запускаем DFS из каждой непосещённой вершины (граф может быть несвязным)
    for (int s = 0; s < n; ++s) {
        if (tin[static_cast<size_t>(s)] == -1) dfs(s, -1);
    }

    // Приводим список мостов к каноническому виду (u<v) и убираем дубликаты (не уверена, что это необходимо)
    std::sort(r.bridges.begin(), r.bridges.end());
    r.bridges.erase(std::unique(r.bridges.begin(), r.bridges.end()), r.bridges.end());
    return r;
}

// Проверка инварианта: мосты не должны меняться при случайных изоморфизмах
bool bridges_invariant_under_random_shuffles(const Graph& g, int trials, std::uint64_t seed) {
    auto base = tarjan_cuts(g).bridges;   // исходное множество мостов
    std::mt19937_64 rng(seed);
    for (int t = 0; t < trials; ++t) {
        (void)t;
        // Строим случайную перестановку вершин p, затем обратную inv
        std::vector<int> p(static_cast<size_t>(g.vertex_count()));
        std::iota(p.begin(), p.end(), 0);
        shuffle_inplace(p, rng);
        std::vector<int> inv(static_cast<size_t>(p.size()));
        for (size_t i = 0; i < p.size(); ++i)
            inv[static_cast<size_t>(p[static_cast<size_t>(i)])] = static_cast<int>(i);

        // Создаём изоморфный граф h с теми же рёбрами, но с перенумерованными вершинами
        Graph h(StorageKind::AdjacencyList);
        int n = g.vertex_count();
        h.add_vertices(n);
        for (int u = 0; u < n; ++u) {
            for (int v : g.neighbors(u)) {
                if (v < u) continue; // добавляем каждое ребро один раз
                int nu = p[static_cast<size_t>(u)];
                int nv = p[static_cast<size_t>(v)];
                h.add_edge(nu, nv);
            }
        }

        // Находим мосты в h, переводим их обратно в исходную нумерацию
        auto br2 = tarjan_cuts(h).bridges;
        std::vector<std::pair<int, int>> mapped;
        mapped.reserve(br2.size());
        for (auto e : br2) {
            int u = inv[static_cast<size_t>(e.first)];
            int v = inv[static_cast<size_t>(e.second)];
            mapped.push_back(norm_edge(u, v));
        }
        std::sort(mapped.begin(), mapped.end());
        mapped.erase(std::unique(mapped.begin(), mapped.end()), mapped.end());

        // Если множества различаются, то инвариант нарушен
        if (mapped != base) return false;
    }
    return true;
}

// Количество компонент связности (обычный BFS/DFS)
int count_connected_components(const Graph& g) {
    int n = g.vertex_count();
    std::vector<char> seen(static_cast<size_t>(n), 0);
    int comp = 0;
    for (int s = 0; s < n; ++s) {
        if (seen[static_cast<size_t>(s)]) continue;
        ++comp;
        std::stack<int> st;
        st.push(s);
        seen[static_cast<size_t>(s)] = 1;
        while (!st.empty()) {
            int u = st.top();
            st.pop();
            for (int v : g.neighbors(u)) {
                if (!seen[static_cast<size_t>(v)]) {
                    seen[static_cast<size_t>(v)] = 1;
                    st.push(v);
                }
            }
        }
    }
    return comp;
}

// Компоненты рёберной двусвязности (получаются удалением всех мостов)
std::vector<std::vector<int>> edge_biconnected_components(const Graph& g) {
    auto cuts = tarjan_cuts(g);
    std::set<std::pair<int, int>> br(cuts.bridges.begin(), cuts.bridges.end()); // множество мостов
    int n = g.vertex_count();
    std::vector<char> vis(static_cast<size_t>(n), 0);
    std::vector<std::vector<int>> out;
    for (int s = 0; s < n; ++s) {
        if (vis[static_cast<size_t>(s)]) continue;
        std::vector<int> comp;
        std::stack<int> st;
        st.push(s);
        vis[static_cast<size_t>(s)] = 1;
        while (!st.empty()) {
            int u = st.top();
            st.pop();
            comp.push_back(u);
            for (int v : g.neighbors(u)) {
                // Если ребро – мост, не переходим по нему
                if (br.count(norm_edge(u, v))) continue;
                if (!vis[static_cast<size_t>(v)]) {
                    vis[static_cast<size_t>(v)] = 1;
                    st.push(v);
                }
            }
        }
        out.push_back(std::move(comp));
    }
    return out;
}

// Вершинно двусвязные блоки (через стек рёбер)
std::vector<std::vector<std::pair<int, int>>> vertex_biconnected_blocks(const Graph& g) {
    int n = g.vertex_count();
    std::vector<std::vector<std::pair<int, int>>> blocks;
    if (n == 0) return blocks;

    std::vector<char> used(static_cast<size_t>(n), 0); // посещена ли вершина
    std::vector<int> tin(static_cast<size_t>(n), -1);
    std::vector<int> up(static_cast<size_t>(n), -1); // low-значение
    int timer = 0;
    std::stack<std::pair<int, int>> st; // стек рёбер текущего блока

    std::function<void(int, int)> dfs = [&](int v, int from) {
        used[static_cast<size_t>(v)] = 1;
        tin[static_cast<size_t>(v)] = up[static_cast<size_t>(v)] = timer++;
        for (int to : g.neighbors(v)) {
            if (to == from) continue;

            if (used[static_cast<size_t>(to)]) {
                // обратное ребро
                up[static_cast<size_t>(v)] = std::min(up[static_cast<size_t>(v)], tin[static_cast<size_t>(to)]);
                // кладём ребро в стек только если оно не уже было обработано (tin[to] < tin[v])
                if (tin[static_cast<size_t>(to)] < tin[static_cast<size_t>(v)]) {
                    st.push(norm_edge(v, to));
                }
            } else {
                st.push(norm_edge(v, to));
                dfs(to, v);
                up[static_cast<size_t>(v)] = std::min(up[static_cast<size_t>(v)], up[static_cast<size_t>(to)]);

                // Если up[to] >= tin[v], то v – точка сочленения (или корень)
                if (up[static_cast<size_t>(to)] >= tin[static_cast<size_t>(v)]) {
                    std::vector<std::pair<int, int>> block;
                    while (!st.empty()) {
                        auto e = st.top();
                        st.pop();
                        block.push_back(e);
                        if (e == norm_edge(v, to)) break;
                    }
                    if (!block.empty()) blocks.push_back(std::move(block));
                }
            }
        }
    };

    for (int s = 0; s < n; ++s) {
        if (used[static_cast<size_t>(s)]) continue;
        timer = 0;
        dfs(s, -1);
        // Если после обхода в стеке остались рёбра, то это последний блок
        if (!st.empty()) {
            std::vector<std::pair<int, int>> block;
            while (!st.empty()) {
                block.push_back(st.top());
                st.pop();
            }
            if (!block.empty()) blocks.push_back(std::move(block));
        }
    }
    return blocks;
}

}  // namespace gd4
