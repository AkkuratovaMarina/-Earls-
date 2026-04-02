#pragma once
// Универсальные обходы графа: DFS и BFS с паттерном «посетитель» 
// Позволяют легко расширять поведение, переопределяя нужные методы в классах-наследниках.

#include "graph.hpp"

#include <queue>
#include <vector>

namespace gd4 {

// Базовый класс для посетителя DFS. Все методы имеют пустую реализацию по умолчанию
struct DfsVisitor {
    virtual ~DfsVisitor() = default;
    virtual void start_vertex(VertexId s, const Graph& g) { (void)s; (void)g; } // перед запуском DFS из вершины s
    virtual void discover_vertex(VertexId u, const Graph& g) { (void)u; (void)g; } // при первом посещении вершины
    virtual void examine_edge(VertexId u, VertexId v, const Graph& g) { (void)u; (void)v; (void)g; } // при рассмотрении ребра (u->v)
    virtual void tree_edge(VertexId u, VertexId v, const Graph& g) { (void)u; (void)v; (void)g; } // ребро, вошедшее в DFS-дерево
    virtual void back_edge(VertexId u, VertexId v, const Graph& g) { (void)u; (void)v; (void)g; } // обратное ребро (уже посещённая вершина)
    virtual void finish_vertex(VertexId u, const Graph& g) { (void)u; (void)g; } // после обработки всех потомков вершины
};

// Функция, сортирующая список соседей по возрастанию 
void default_neighbor_order(std::vector<int>& nbrs);

// Запускает DFS на всём графе (лес из каждой непосещённой вершины)
// gvis посетитель, у которого вызываются колбэки
// order если nullptr – соседи просматриваются в отсортированном порядке (default_neighbor_order)
void depth_first_search(const Graph& g, DfsVisitor& vis, NeighborOrder* order = nullptr);

// DFS только в одной компоненте, начиная с вершины start
void depth_first_search(const Graph& g, VertexId start, DfsVisitor& vis,
                        NeighborOrder* order = nullptr);

// Базовый класс для посетителя BFS
struct BfsVisitor {
    virtual ~BfsVisitor() = default;
    virtual void discover_vertex(VertexId u, const Graph& g) { (void)u; (void)g; } // при обнаружении вершины (первый раз)
    virtual void examine_edge(VertexId u, VertexId v, const Graph& g) { (void)u; (void)v; (void)g; } // при рассмотрении ребра (u->v)
    virtual void tree_edge(VertexId u, VertexId v, const Graph& g) { (void)u; (void)v; (void)g; }    // ребро, вошедшее в BFS-дерево
};

// Запускает BFS из заданной вершины start
void breadth_first_search(const Graph& g, VertexId start, BfsVisitor& vis,
                          NeighborOrder* order = nullptr);

}  // namespace gd4
