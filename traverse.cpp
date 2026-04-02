// Обходы графа (DFS, BFS) с паттерном «посетитель»
// Порядок обхода соседей настраивается через NeighborOrder:
// Если order == nullptr, используется default_neighbor_order (сортировка по возрастанию)

#include "graphdro4/traverse.hpp"
#include <algorithm>

namespace gd4 {

// Функция сортировки соседей по умолчанию по возрастанию
void default_neighbor_order(std::vector<int>& nbrs) {
    std::sort(nbrs.begin(), nbrs.end());
}

namespace {

// Класс Хранит ссылки на граф, посетителя и порядок обхода соседей
// Умеет запускать обход из всех вершин (run_all) или только из заданной (run_from)
class DfsRecursive {
   public:
    DfsRecursive(const Graph& g, DfsVisitor& vis, NeighborOrder* order)
        : g_(g), vis_(vis), order_(order) {
        int n = g.vertex_count();
        seen_.assign(static_cast<size_t>(n), 0);   // массив посещённых вершин
    }

    // Обход всех компонент графа (лес DFS)
    void run_all() {
        int n = g_.vertex_count();
        for (VertexId s = 0; s < n; ++s) {
            if (!seen_[static_cast<size_t>(s)]) {
                vis_.start_vertex(s, g_);      // уведомляем о начале новой компоненты
                dfs(s, -1);
            }
        }
    }

    // Обход только из заданной вершины (в пределах одной компоненты)
    void run_from(VertexId s) {
        vis_.start_vertex(s, g_);
        dfs(s, -1);
    }

   private:
    const Graph& g_;
    DfsVisitor& vis_;
    NeighborOrder* order_;          // порядок обхода соседей (может быть nullptr)
    std::vector<char> seen_;        // метки посещённых вершин

    // Рекурсивная функция DFS
    void dfs(VertexId u, VertexId parent) {
        seen_[static_cast<size_t>(u)] = 1;
        vis_.discover_vertex(u, g_);                // вершина обнаружена

        // Получаем список соседей и упорядочиваем его (сортировка или перемешивание)
        auto nbr = g_.neighbors(u);
        if (order_)
            (*order_)(nbr);                         // применяем пользовательский порядок
        else
            default_neighbor_order(nbr);            // по умолчанию – сортировка

        for (int v : nbr) {
            vis_.examine_edge(u, v, g_);            // рассматриваем ребро (u,v)
            if (v == parent) continue;              // не возвращаемся к родителю

            if (!seen_[static_cast<size_t>(v)]) {
                vis_.tree_edge(u, v, g_);           // ребро входит в DFS-дерево
                dfs(static_cast<VertexId>(v), u);
            } else {
                vis_.back_edge(u, v, g_);            // обратное ребро
            }
        }
        vis_.finish_vertex(u, g_);                  // обработка вершины завершена
    }
};

}  // namespace

// Обход всего графа (лес DFS). Вызывается из каждой непосещённой вершины
void depth_first_search(const Graph& g, DfsVisitor& vis, NeighborOrder* order) {
    DfsRecursive dr(g, vis, order);
    dr.run_all();
}

// DFS только из заданной вершины (одна компонента)
void depth_first_search(const Graph& g, VertexId start, DfsVisitor& vis, NeighborOrder* order) {
    DfsRecursive dr(g, vis, order);
    dr.run_from(start);
}

// BFS из заданной вершины
void breadth_first_search(const Graph& g, VertexId start, BfsVisitor& vis,
                          NeighborOrder* order) {
    int n = g.vertex_count();
    std::vector<char> seen(static_cast<size_t>(n), 0);   // метки посещённых вершин
    std::queue<int> q;
    q.push(start);
    seen[static_cast<size_t>(start)] = 1;
    vis.discover_vertex(start, g);

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        auto nbr = g.neighbors(u);
        if (order)
            (*order)(nbr);
        else
            default_neighbor_order(nbr);

        for (int v : nbr) {
            vis.examine_edge(u, v, g);                 // рассматриваем ребро (u,v)
            if (!seen[static_cast<size_t>(v)]) {
                vis.tree_edge(u, v, g);                // ребро в BFS-дереве
                seen[static_cast<size_t>(v)] = 1;
                vis.discover_vertex(v, g);
                q.push(v);
            }
        }
    }
}

}  // namespace gd4
