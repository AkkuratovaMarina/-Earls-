// Набор юнит-тестов для GraphoDro4 с использованием Catch2
// Тесты не оставят Вас равнодушными, потому что проверяют:
//   - базовые операции с графом (добавление вершин/рёбер, листья)
//   - смену бэкенда (список <-> матрица)
//   - корректность генераторов графов (полный, двудольный, дерево, цикл, звезда, колесо итп)
//   - вычисление метрик (плотность, диаметр, транзитивность, двудольность, хроматическое число)
//   - алгоритмы связности (мосты, точки сочленения, компоненты)
//   - парсеры форматов (список рёбер, матрица, DIMACS, SNAP)
//   - сериализаторы (DOT, .edges, JSON) с проверкой ключевых элементов
//   - случайные остов и цикл (инварианты, разнообразие при разных seed)
//   - JSON-сериализатор!!!

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "graphdro4/connectivity.hpp"
#include "graphdro4/generators.hpp"
#include "graphdro4/graph.hpp"
#include "graphdro4/metrics.hpp"
#include "graphdro4/parsers.hpp"
#include "graphdro4/serializers.hpp"
#include "graphdro4/traverse.hpp"
#include "graphdro4/types.hpp"

#include <set>
#include <sstream>
#include <unordered_set>

using namespace gd4;

namespace {

// Проверка связности графа (BFS от вершины 0)
bool is_connected(const Graph& g) {
    int n = g.vertex_count();
    if (n == 0) return true;
    std::vector<char> vis(static_cast<size_t>(n), 0);
    std::vector<int> st{0};
    vis[0] = 1;
    while (!st.empty()) {
        int u = st.back();
        st.pop_back();
        for (int v : g.neighbors(u))
            if (!vis[static_cast<size_t>(v)]) {
                vis[static_cast<size_t>(v)] = 1;
                st.push_back(v);
            }
    }
    for (char c : vis)
        if (!c) return false;
    return true;
}

// Преобразует список вершин цикла в множество канонических рёбер (для сравнения)
std::set<Edge> cycle_edges_normalized(const std::vector<int>& c) {
    std::set<Edge> s;
    if (c.size() < 3) return s;
    for (size_t i = 0; i + 1 < c.size(); ++i)
        s.insert(norm_edge(c[i], c[i + 1]));
    s.insert(norm_edge(c.front(), c.back()));
    return s;
}

// Проверяет, является ли список вершин простым циклом в графе
bool is_simple_cycle_in_graph(const Graph& g, const std::vector<int>& c) {
    if (c.size() < 3) return false;
    std::unordered_set<int> uniq(c.begin(), c.end());
    if (uniq.size() != c.size()) return false;
    for (size_t i = 0; i + 1 < c.size(); ++i)
        if (!g.has_edge(c[i], c[i + 1])) return false;
    return g.has_edge(c.back(), c.front());
}

// Сравнение двух графов (одинаковая нумерация вершин)
bool same_undirected_simple_graph(const Graph& a, const Graph& b) {
    if (a.vertex_count() != b.vertex_count()) return false;
    if (a.edge_count() != b.edge_count()) return false;
    int n = a.vertex_count();
    for (int u = 0; u < n; ++u)
        for (int v : a.neighbors(u))
            if (v > u && !b.has_edge(u, v)) return false;
    return true;
}

}  // namespace

// Граф: базовые операции
TEST_CASE("Graph: базовые операции и листья", "[graph]") {
    Graph g;
    g.add_vertices(4);
    REQUIRE(g.vertex_count() == 4);
    REQUIRE(!g.add_edge(0, 0));          // петли не добавляются
    REQUIRE(g.add_edge(0, 1));           // добавляем ребро
    REQUIRE(!g.add_edge(0, 1));          // повторное добавление не меняет граф
    REQUIRE(g.add_edge(0, 2));
    REQUIRE(g.add_edge(0, 3));
    REQUIRE(g.edge_count() == 3);
    REQUIRE(g.is_leaf(1));               // вершина 1 – лист (степень 1)
    REQUIRE(!g.is_leaf(0));              // центр звезды – не лист
}

// Смена бэкенда
TEST_CASE("Backend: смена список <-> матрица сохраняет структуру", "[backend]") {
    Graph g = GenWheel(6).generate(StorageKind::AdjacencyList);
    auto m0 = g.edge_count();
    g.set_backend(StorageKind::AdjacencyMatrix);
    REQUIRE(g.edge_count() == m0);
    g.set_backend(StorageKind::AdjacencyList);
    REQUIRE(g.edge_count() == m0);
}

// Проверка генераторов
TEST_CASE("Генераторы: известные соотношения", "[gen]") {
    int n = 7;
    Graph kn = GenComplete(n).generate();
    REQUIRE(kn.edge_count() == n * (n - 1) / 2);

    Graph kmn = GenCompleteBipartite(3, 4).generate();
    REQUIRE(kmn.edge_count() == 12);                // 12 рёбер

    Graph tree = GenTree(10).generate();
    REQUIRE(tree.edge_count() == 9);                // дерево на 10 вершинах имеет 9 рёбер
    REQUIRE(count_connected_components(tree) == 1); // дерево связно

    Graph cyc = GenCycle(5).generate();
    REQUIRE(cyc.edge_count() == 5);

    Graph path = GenPath(1).generate();
    REQUIRE(path.edge_count() == 0);                // путь из одной вершины без рёбер

    Graph w = GenWheel(5).generate();
    REQUIRE(w.vertex_count() == 5);
}

// Метрики
TEST_CASE("Метрики: K_n и путь", "[metrics]") {
    Graph k4 = GenComplete(4).generate();
    GraphMetrics mk(k4);
    REQUIRE(mk.density() == Approx(1.0));
    REQUIRE(mk.diameter() == 1);
    REQUIRE(mk.is_bipartite() == false);

    Graph p = GenPath(6).generate();
    GraphMetrics mp(p);
    REQUIRE(mp.diameter() == 5);
    REQUIRE(mp.is_bipartite() == true);
    REQUIRE(mp.bridge_count() == 5);               // все рёбра в пути – мосты
}

// Мосты и точки сочленения 
TEST_CASE("Мосты и точки сочленения: ручной граф", "[cuts]") {
    Graph g;
    g.add_vertices(4);
    g.add_edge(0, 1);
    g.add_edge(1, 2);
    g.add_edge(2, 0);                              // треугольник (0-1-2)
    g.add_edge(2, 3);                              // хвост
    auto r = tarjan_cuts(g);
    REQUIRE(r.articulation_count() == 1);          // точка сочленения – вершина 2
    REQUIRE(r.bridge_count() == 1);                // мост – ребро (2,3)
}

TEST_CASE("Инвариант мостов при случайных изоморфизмах меток", "[cuts][random]") {
    Graph g = GenExactBridges(20, 4).generate();
    REQUIRE(bridges_invariant_under_random_shuffles(g, 15, 4242u));
}

// Двудольность и жадная раскраска
TEST_CASE("Двудольность и жадная хроматика", "[color]") {
    Graph k3 = GenComplete(3).generate();
    GraphMetrics m(k3);
    REQUIRE(m.is_bipartite() == false);
    REQUIRE(m.greedy_chromatic_upper_bound() >= 3); // K₃ требует 3 цвета

    Graph c4 = GenCycle(4).generate();
    GraphMetrics m2(c4);
    REQUIRE(m2.is_bipartite() == true);
    REQUIRE(m2.greedy_chromatic_upper_bound() <= 2); // чётный цикл двудольный
}

// DFS visitor
TEST_CASE("DFS visitor: фиксируем число древесных рёбер в дереве", "[dfs]") {
    Graph t = GenTree(12).generate();
    struct V : DfsVisitor {
        int tree = 0;
        void tree_edge(VertexId u, VertexId v, const Graph&) override {
            (void)u; (void)v;
            ++tree;
        }
    } vis;
    depth_first_search(t, vis, nullptr);
    REQUIRE(vis.tree == t.vertex_count() - 1); // в дереве n-1 рёбер
}

// Парсеры
TEST_CASE("Парсер списка рёбер", "[parse]") {
    std::istringstream in("0 1\n1 -- 2\n");
    EdgeListParser p;
    Graph g = p.parse(in);
    REQUIRE(g.has_edge(0, 1));
    REQUIRE(g.has_edge(1, 2));
}

TEST_CASE("Парсер матрицы смежности", "[parse]") {
    std::istringstream in("3\n0 1 0\n1 0 1\n0 1 0\n");
    AdjacencyMatrixParser p;
    Graph g = p.parse(in);
    REQUIRE(g.edge_count() == 2);                 // рёбра: 0-1, 1-2
}

TEST_CASE("Парсер DIMACS (p edge / вершины 1..n)", "[parse][dimacs]") {
    std::istringstream in(
        "c test graph\n"
        "p edge 4 3\n"
        "e 1 2\n"
        "e 2 3\n"
        "e 3 4\n");
    DimacsParser p;
    Graph g = p.parse(in);
    REQUIRE(g.vertex_count() == 4);
    REQUIRE(g.edge_count() == 3);
    REQUIRE(g.has_edge(0, 1));
    REQUIRE(g.has_edge(2, 3));                   // вершины 3 и 4 стали 2 и 3
}

TEST_CASE("Парсер SNAP (комментарий #, таб и пробел)", "[parse][snap]") {
    std::istringstream in("# header\n0\t1\n1 2\n");
    SnapParser p;
    Graph g = p.parse(in);
    REQUIRE(g.vertex_count() >= 3);
    REQUIRE(g.has_edge(0, 1));
    REQUIRE(g.has_edge(1, 2));
}

// Сериализаторы
TEST_CASE("DOT: кластеры 2-связных компонент (маркеры subgraph)", "[export][dot]") {
    Graph g;
    g.add_vertices(4);
    g.add_edge(0, 1);
    g.add_edge(1, 2);
    g.add_edge(2, 0);   // треугольник
    g.add_edge(2, 3);   // мост к вершине 3
    DotSerializeOptions o;
    o.cluster_edge_biconnected = true;
    o.cluster_vertex_biconnected = true;
    std::ostringstream out;
    DotSerializer ds(o);
    ds.write(g, out);
    std::string s = out.str();
    REQUIRE(s.find("subgraph cluster_ebcc") != std::string::npos);
    REQUIRE(s.find("subgraph cluster_vbcc") != std::string::npos);
}

TEST_CASE("Генератор ebcc: связность, K4 в первом блоке, мосты между блоками", "[gen][ebcc]") {
    const int t = 4;
    Graph g = GenExactEdgeBiconnectedComponents(30, t).generate();
    REQUIRE(count_connected_components(g) == 1);           // граф связный
    REQUIRE(g.degree(0) == 3);                             // в K₄ степень 3
    auto cuts = tarjan_cuts(g);
    REQUIRE(cuts.bridge_count() >= t - 1);                 // минимум t-1 мостов
}

TEST_CASE("Сериализация DOT и .edges содержат ожидаемые маркеры", "[export]") {
    Graph g = GenCycle(4).generate();
    std::ostringstream d1, d2, ej;
    DotSerializer ds1({});
    ds1.write(g, d1);
    REQUIRE(d1.str().find("strict graph") != std::string::npos);

    DotSerializeOptions o;
    o.show_random_spanning_tree = true;
    o.rng_seed = 1;
    DotSerializer ds2(o);
    ds2.write(g, d2);

    Program4YouEdgesSerializer es;
    es.write(g, ej);
    REQUIRE(ej.str().find("--") != std::string::npos);

    // При фиксированном RNG результат должен быть одинаковым
    std::ostringstream d3;
    DotSerializer ds3(o);
    ds3.write(g, d3);
    REQUIRE(d2.str() == d3.str());
}

// Случайный остов 
TEST_CASE("Случайный остов: связность, |E|=n-1, ацикличность (дерево)", "[spanning][random]") {
    Graph g = GenComplete(8).generate();
    auto e1 = random_spanning_tree_edges(g, 1);
    auto e2 = random_spanning_tree_edges(g, 2);
    REQUIRE(e1.size() == static_cast<size_t>(g.vertex_count() - 1));
    REQUIRE(e2.size() == e1.size());

    // Разные seed дают разные деревья (надеемся)
    std::sort(e1.begin(), e1.end());
    std::sort(e2.begin(), e2.end());
    REQUIRE(e1 != e2);

    Graph t(StorageKind::AdjacencyList);
    t.add_vertices(g.vertex_count());
    for (auto uv : e1) t.add_edge(uv.first, uv.second);
    REQUIRE(is_connected(t));
    REQUIRE(t.edge_count() == t.vertex_count() - 1);
}

// Случайный цикл
TEST_CASE("Случайный простой цикл: простой замкнутый и >1 варианта рёбер при переборе сидов", "[cycle]") {
    Graph g = GenComplete(8).generate();
    std::set<std::set<Edge>> distinct;
    for (std::uint64_t s = 1; s <= 48; ++s) {
        auto c = random_simple_cycle_vertices(g, s);
        REQUIRE(is_simple_cycle_in_graph(g, c));
        distinct.insert(cycle_edges_normalized(c));
        if (distinct.size() >= 2) break;
    }
    REQUIRE(distinct.size() >= 2);   // хотя бы два разных цикла
}

// Генератор компонент связности 
TEST_CASE("Компоненты связности генератор", "[gen][components]") {
    Graph g = GenComponents(12, 3).generate();
    REQUIRE(count_connected_components(g) == 3);
}

// Кубический граф
TEST_CASE("Кубический граф: степени либо 0 (провал) либо все 3", "[gen][cubic]") {
    Graph h = GenRandomCubic(8, 12345678).generate();
    bool ok = true;
    for (int v = 0; v < h.vertex_count(); ++v)
        if (h.degree(v) != 3) ok = false;
    // Если генератор вернул 8 вершин, все должны иметь степень 3 (либо он вернул пустой граф)
    if (h.vertex_count() == 8) REQUIRE(ok);
}

// G(n,p) – разные seed дают разные графы
TEST_CASE("G(n,p): разные сиды при тех же n и p дают разные графы", "[gen][random][gnp]") {
    Graph g1 = GenErdosRenyi(22, 0.45, 9001u).generate();
    Graph g2 = GenErdosRenyi(22, 0.45, 9002u).generate();
    REQUIRE(g1.vertex_count() == 22);
    REQUIRE(g1.edge_count() > 0);
    REQUIRE(!same_undirected_simple_graph(g1, g2));
}

//  Кубический граф – разные сиды 
TEST_CASE("Случайный кубический: разные сиды — различные графы на n=12", "[gen][random][cubic]") {
    Graph c1 = GenRandomCubic(12, 77001u).generate();
    Graph c2 = GenRandomCubic(12, 77002u).generate();
    REQUIRE(c1.vertex_count() == 12);
    REQUIRE(c2.vertex_count() == 12);
    for (int v = 0; v < 12; ++v) {
        REQUIRE(c1.degree(v) == 3);
        REQUIRE(c2.degree(v) == 3);
    }
    REQUIRE(!same_undirected_simple_graph(c1, c2));
}

// JSON-сериализатор 
TEST_CASE("JSON bonus сериализатор", "[json]") {
    Graph g = GenPath(3).generate();
    std::ostringstream o;
    JsonEdgeListSerializer js;
    js.write(g, o);
    REQUIRE(o.str().find("\"edges\"") != std::string::npos);
}