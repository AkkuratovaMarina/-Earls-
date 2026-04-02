// Разбор входных форматов: список рёбер, матрица смежности, DIMACS, SNAP
// Все парсеры наследуются от GraphParserBase и должны возвращать объект Graph с указанным бэкендом (список смежности или матрица)
// EdgeList:
// 1) строки вида "u v", "u -- v", "u -> v" (символы "--" и "->" игнорируются)
// 2) комментарии начинаются с '#'
// 3) пустые строки пропускаются
// AdjacencyMatrix:
// 1. первая строка: n (число вершин)
// 2. затем n строк по n целых чисел (0 или 1)
// 3. ненулевое значение в позиции (i,j) означает ребро между i и j
// 4. петли (i == j) игнорируются
// DIMACS:
//   - строки 'c' – комментарии
//   - строка 'p edge n m' задаёт число вершин n и ожидаемое число рёбер m
//   - строки 'e u v' – рёбра
//   - может использоваться формат 'col' для задач раскраски, но рёбра всё равно задаются через 'e'
// SNAP (Stanford Network Analysis Platform):
//   - строки с "#" – комментарии
//   - отрицательные идентификаторы вершин пропускаются
//   - граф неориентированный, ребро добавляется в обе стороны (обработка в add_edge)

#include "graphdro4/parsers.hpp"

#include <cctype>
#include <sstream>

namespace gd4 {

namespace {

// Удаляет пробельные символы в начале и конце строки
std::string trim(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    return s;
}

}  // namespace

// EdgeListParser 
// Читает граф из текстового списка рёбер.
// Поддерживаемые форматы: "u v", "u -- v", "u -> v".
// Вершины могут быть любыми целыми числами (положительными и нулевыми)
// При обнаружении неверной строки выбрасывается ParseError
Graph EdgeListParser::parse(std::istream& in, StorageKind backend) {
    Graph g(backend);
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;   // пропускаем комментарии
        std::istringstream ls(line);
        int u, v;
        std::string mid;
        if (!(ls >> u)) continue;                       // строка не начинается с числа – пропускаем
        if (ls >> mid) {
            // Если после первого числа есть что-то, проверяем, является ли это разделителем
            if (mid == "--" || mid == "->" || mid == "-") {
                // Ожидаем вторую вершину после разделителя
                if (!(ls >> v)) throw ParseError("edge list: ожидалась вторая вершина");
            } else {
                // Возможно, второе число идёт сразу без разделителя 
                try {
                    v = std::stoi(mid);
                } catch (...) {
                    throw ParseError("edge list: неверный формат (ожидалось u v или u -- v)");
                }
            }
        } else
            continue;   // в строке только одно число – не ребро, пропускаем
        g.add_edge(u, v);
    }
    return g;
}

// AdjacencyMatrixParser 
// Читает матрицу смежности из потока.
// Первая строка: n, далее n строк по n чисел (0/1)
Graph AdjacencyMatrixParser::parse(std::istream& in, StorageKind backend) {
    Graph g(backend);
    int n = 0;
    if (!(in >> n) || n < 0) throw ParseError("matrix: неверная размерность");
    g.add_vertices(n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            int x;
            if (!(in >> x)) throw ParseError("matrix: не хватает элементов");
            // Петли (i == j) игнорируем, ненулевое значение означает ребро
            if (i != j && x != 0) g.add_edge(i, j);
        }
    }
    return g;
}

// DimacsParser 
// Читает граф в формате DIMACS 

Graph DimacsParser::parse(std::istream& in, StorageKind backend) {
    Graph g(backend);
    int n = 0, m_expected = -1;
    int m_seen = 0;
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;
        if (line[0] == 'c') continue;          // строка комментария
        std::istringstream ls(line);
        if (line[0] == 'p') {                  // строка описания графа
            std::string p, fmt;
            int nv, me;
            if (!(ls >> p >> fmt >> nv >> me)) throw ParseError("DIMACS: неверная строка p");
            (void)p;
            if (fmt != "edge" && fmt != "col") {
            }
            n = nv;
            m_expected = me;
            g.add_vertices(n);
            continue;
        }
        if (line[0] == 'e') {                  // строка ребра
            char e;
            int u, v;
            if (!(ls >> e >> u >> v)) throw ParseError("DIMACS: неверная строка e");
            g.add_edge(u - 1, v - 1);
            ++m_seen;
            continue;
        }
        // Любые другие строки игнорируем (например, 'n' или пустые).
    }
    (void)m_expected;
    (void)m_seen;
    return g;
}

// SnapParser 
// Читает граф в формате SNAP (Stanford Network Analysis Platform)
// Строки, начинающиеся с '#' -комментарии
// Отрицательные идентификаторы вершин пропускаются (в данных SNAP их быть не должно, но а вдруг)
Graph SnapParser::parse(std::istream& in, StorageKind backend) {
    Graph g(backend);
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;
        if (line[0] == '#') continue;
        std::istringstream ls(line);
        long long u, v;
        // Пытаемся считать два числа, разделённых любым пробелом (включая табуляцию)
        if (line.find('\t') != std::string::npos) {
            if (!(ls >> u >> v)) continue;
        } else {
            if (!(ls >> u >> v)) continue;
        }
        if (u < 0 || v < 0) continue;   // отрицательные номера вершин не поддерживаются
        g.add_edge(static_cast<int>(u), static_cast<int>(v));
    }
    return g;
}

}  // namespace gd4