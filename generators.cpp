// Детерминированные и случайные генераторы графов для тестов
// Детерминированные генераторы (Complete, Tree, Star, Cycle, Path, Wheel, CompleteBipartite)
// GenErdosRenyi – классическая модель Эрдёша–Реньи G(n,p): каждое из C(n,2) рёбер присутствует независимо с вероятностью p
// GenRandomCubic – строит случайный кубический граф (все степени 3) на чётном числе вершин
// Использует три раунда случайных совершенных паросочетаний.
// Если после 500 попыток не удалось избежать кратных рёбер или получить правильные степени, возвращает пустой граф на n вершинах
// GenComponents – разбивает n вершин на k компонент, каждая из которых – полный граф
// GenExactBridges – строит связный граф с заданным числом мостов (цепочка треугольников)
// GenExactArticulations – связный граф с заданным числом точек сочленения (цепочка K₃)
// GenExactEdgeBiconnectedComponents – граф с заданным числом компонент рёберной двусвязности (блоки K₄, соединённые мостами)
// GenHalin – граф Халина (колесо хд)

#include "graphdro4/generators.hpp"

#include "graphdro4/connectivity.hpp"
#include "graphdro4/types.hpp"

#include <functional>
#include <numeric>
#include <queue>
#include <random>

namespace gd4 {

// Полный граф K_n 
Graph GenComplete::generate(StorageKind backend) const {
    Graph g(backend);
    if (n_ <= 0) return g;
    g.add_vertices(n_);
    for (int i = 0; i < n_; ++i)
        for (int j = i + 1; j < n_; ++j) g.add_edge(i, j);
    return g;
}

// Полный двудольный граф K_{n1,n2}
Graph GenCompleteBipartite::generate(StorageKind backend) const {
    Graph g(backend);
    int n = n1_ + n2_;
    if (n <= 0) return g;
    g.add_vertices(n);
    for (int i = 0; i < n1_; ++i)
        for (int j = 0; j < n2_; ++j) g.add_edge(i, n1_ + j);
    return g;
}

// Дерево (путь) 
// Для n=1 граф состоит из одной вершины без рёбер
Graph GenTree::generate(StorageKind backend) const {
    Graph g(backend);
    if (n_ <= 0) return g;
    g.add_vertices(n_);
    for (int i = 1; i < n_; ++i) g.add_edge(i - 1, i);
    return g;
}

// Звезда S_n 
Graph GenStar::generate(StorageKind backend) const {
    Graph g(backend);
    if (n_ <= 0) return g;
    g.add_vertices(n_);
    if (n_ == 1) return g;
    for (int i = 1; i < n_; ++i) g.add_edge(0, i);
    return g;
}

// Цикл C_n 
// Если n < 3, граф остаётся без рёбер (просто набор изолированных вершин)
Graph GenCycle::generate(StorageKind backend) const {
    Graph C(backend);
    if (n_ < 3) {
        if (n_ > 0) C.add_vertices(n_);
        return C;
    }
    C.add_vertices(n_);
    for (int i = 0; i < n_; ++i) C.add_edge(i, (i + 1) % n_);
    return C;
}

// Путь P_n 
Graph GenPath::generate(StorageKind backend) const {
    Graph P(backend);
    if (n_ <= 0) return P;
    P.add_vertices(n_);
    for (int i = 0; i + 1 < n_; ++i) P.add_edge(i, i + 1);
    return P;
}

// Колесо W_n 
// Колесо: цикл из n-1 вершины (вершины 1..n-1) и центральная вершина 0, соединённая со всеми вершинами цикла
// Требование: n >= 4, иначе возвращается пустой граф.
Graph GenWheel::generate(StorageKind backend) const {
    Graph W(backend);
    if (n_ < 4) return W;
    W.add_vertices(n_);
    for (int i = 1; i < n_; ++i) W.add_edge(i, i == n_ - 1 ? 1 : i + 1);
    // Центральная вершина 0 соединена со всеми
    for (int i = 1; i < n_; ++i) W.add_edge(0, i);
    return W;
}

// Случайный граф Эрдёша–Реньи 
// Каждая пара вершин (i, j) независимо включается с вероятностью p
// Генератор случайных чисел инициализируется сидом для воспроизводимости
Graph GenErdosRenyi::generate(StorageKind backend) const {
    Graph G(backend);
    if (n_ <= 0) return G;
    G.add_vertices(n_);
    std::mt19937_64 rng(seed_);
    std::uniform_real_distribution<double> U(0.0, 1.0);
    for (int i = 0; i < n_; ++i)
        for (int j = i + 1; j < n_; ++j)
            if (U(rng) < p_) G.add_edge(i, j);
    return G;
}

// Случайный кубический граф 
// Алгоритм: пытается построить три случайных совершенных паросочетания на n вершинах
// Если n нечётное, сразу возвращает граф без рёбер
Graph GenRandomCubic::generate(StorageKind backend) const {
    Graph g(backend);
    if (n_ <= 0) return g;
    if (n_ % 2 != 0) {
        g.add_vertices(n_);
        return g;
    }
    std::mt19937_64 rng(seed_);
    for (int attempt = 0; attempt < 500; ++attempt) {
        (void)attempt;
        g.clear();
        g.add_vertices(n_);
        bool bad = false;
        // Три раунда паросочетаний
        for (int round = 0; round < 3; ++round) {
            (void)round;
            std::vector<int> perm(static_cast<size_t>(n_));
            std::iota(perm.begin(), perm.end(), 0);
            shuffle_inplace(perm, rng);
            for (int i = 0; i < n_; i += 2) {
                int a = perm[static_cast<size_t>(i)];
                int b = perm[static_cast<size_t>(i + 1)];
                if (g.has_edge(a, b) || a == b) { // кратные рёбра или петля
                    bad = true;
                    break;
                }
                g.add_edge(a, b);
            }
            if (bad) break;
        }
        if (bad) continue;
        // Проверяем, что все вершины имеют степень 3
        bool ok = true;
        for (int v = 0; v < n_; ++v)
            if (g.degree(v) != 3) ok = false;
        if (ok) return g;
    }
    // Если не удалось, возвращаем граф без рёбер
    g.clear();
    g.add_vertices(n_);
    return g;
}

// Граф с заданным числом компонент 
// Разбиваем n вершин на k компонент
// Внутри каждой компоненты строим полный граф
Graph GenComponents::generate(StorageKind backend) const {
    Graph g(backend);
    if (n_ <= 0) return g;
    int k = std::max(1, k_);
    g.add_vertices(n_);
    std::vector<int> sz(static_cast<size_t>(k), n_ / k);
    for (int i = 0; i < n_ % k; ++i) ++sz[static_cast<size_t>(i)];
    int off = 0;
    for (int c = 0; c < k; ++c) {
        int s = sz[static_cast<size_t>(c)];
        for (int i = 0; i < s; ++i)
            for (int j = i + 1; j < s; ++j) g.add_edge(off + i, off + j);
        off += s;
    }
    return g;
}

// Связный граф с заданным числом мостов
// Строит цепочку из (b+1) треугольников (K₃), соединённых последовательно одним ребром
// Каждое соединительное ребро является мостом, остальные рёбра – внутри треугольников
// Общее число вершин: need = 3*(b+1). Если n больше need, остаток добавляется в виде хвоста
Graph GenExactBridges::generate(StorageKind backend) const {
    Graph g(backend);
    int b = std::max(0, b_);
    int need = 3 * (b + 1);
    int n = std::max(n_, need);
    g.add_vertices(n);
    int off = 0;
    for (int i = 0; i <= b; ++i) {
        // Треугольник: вершины a, c, d
        int a = off, c = off + 1, d = off + 2;
        g.add_edge(a, c);
        g.add_edge(c, d);
        g.add_edge(a, d);
        // Мост к следующему треугольнику (если не последний)
        if (i < b) g.add_edge(d, off + 3);
        off += 3;
    }
    // Добавляем оставшиеся вершины в виде пути, присоединённого к последней вершине
    for (int v = need; v < n; ++v) g.add_edge(v - 1, v);
    return g;
}

// Связный граф с заданным числом точек сочленения
// Строит цепочку из (a+1) клик K₃, где соседние клики пересекаются по одной вершине
// Каждая общая вершина является точкой сочленения
// Если n не хватает для всех треугольников, добавляется хвост
Graph GenExactArticulations::generate(StorageKind backend) const {
    Graph g(backend);
    int a = std::max(0, a_);
    if (n_ <= 0) return g;
    g.add_vertices(n_);
    if (n_ < 3) return g;
    int v = 0;
    // Первый треугольник: вершины 0,1,2
    g.add_edge(0, 1);
    g.add_edge(1, 2);
    g.add_edge(0, 2);
    int last_corner = 2;
    v = 3;
    // Создаём a треугольников, каждый пересекается с предыдущим по вершине last_corner
    for (int i = 0; i < a && v + 1 < n_; ++i) {
        int x = v;
        int y = v + 1;
        g.add_edge(last_corner, x);
        g.add_edge(last_corner, y);
        g.add_edge(x, y);
        last_corner = x;
        v += 2;
    }
    // Оставшиеся вершины цепляем хвостом
    while (v < n_) {
        g.add_edge(v - 1, v);
        ++v;
    }
    return g;
}

// Граф с заданным числом компонент рёберной двусвязности
// Строит цепочку из t блоков (K₄ – полный граф на 4 вершинах), соединённых мостами
// Каждый блок – рёберно-двусвязен, мост между блоками является единственной связью
// Общее число вершин: need = t*4 + (t-1). Если n больше, остаток цепляется хвостом
Graph GenExactEdgeBiconnectedComponents::generate(StorageKind backend) const {
    Graph g(backend);
    int t = std::max(1, t_);
    int per = 4;               // размер блока – 4 вершины
    int need = t * per + (t - 1);
    int n = std::max(n_, need);
    g.add_vertices(n);
    int off = 0;
    for (int i = 0; i < t; ++i) {
        for (int u = 0; u < per; ++u)
            for (int v = u + 1; v < per; ++v) g.add_edge(off + u, off + v);
        // Мост к следующему блоку (кроме последнего)
        if (i + 1 < t) g.add_edge(off + per - 1, off + per);
        off += per;
    }
    // Оставшиеся вершины прикрепляем к последней вершине предыдущего блока
    for (int v = off; v < n; ++v) g.add_edge(off - 1, v);
    return g;
}

// Граф Халина
// при sample_ = true или n < 4 возвращается колесо
// В общем случае (sample_ = false) также возвращается колесо
GenHalin::GenHalin(int n, bool use_sample, std::uint64_t seed) : n_(n), sample_(use_sample), seed_(seed) {
    (void)seed_;
}

Graph GenHalin::generate(StorageKind backend) const {
    if (sample_ || n_ < 4) return GenWheel(std::max(4, n_)).generate(backend);
    // Для общего случая тоже возвращаем колесо (но тут мы запутались и не доделали)
    return GenWheel(std::max(4, n_)).generate(backend);
}

}  // namespace gd4