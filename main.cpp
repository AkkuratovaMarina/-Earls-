// Консольное приложение GraphoDro4
// Поток данных:
// При парсинге (parse) или экспорте (export) загружается граф из файла
// При генерации (gen) граф создаётся генератором
// Затем (в зависимости от команды) выводятся метрики или сохраняется в файл
// Обработка ошибок: все исключения перехватываются в main, выводят сообщение и возвращают код 1.
// Справка: при запуске без аргументов, с `help`, `-h` или `--help` выводится подробное описание команд.
// Сборка: через Makefile, цель `graphdro4`

#include "graphdro4/connectivity.hpp"
#include "graphdro4/generators.hpp"
#include "graphdro4/graph.hpp"
#include "graphdro4/metrics.hpp"
#include "graphdro4/parsers.hpp"
#include "graphdro4/serializers.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

namespace {

// Выводит подробную справку по использованию приложения
void print_help() {
    std::cout
        << "GraphoDro4 — лабораторная работа №2\n\n"
        << "Использование:\n"
        << "  graphdro4 help\n"
        << "  graphdro4 backend-demo          (демонстрация смены бэкенда списка смежности <-> матрицы)\n"
        << "  graphdro4 parse <format> <file> [backend matrix|list]\n"
        << "      format: edge_list | matrix | dimacs | snap\n"
        << "  graphdro4 metrics <format> <file>\n"
        << "  graphdro4 gen <kind> <args...> [out.dot]\n"
        << "      kind: kn, knm, tree, star, cycle, path, wheel, gnp, cubic,\n"
        << "            components, bridges, art, ebcc, halin [0|1] [seed]\n"
        << "  graphdro4 export dot <format> <file> [out.dot] [flags: cluster_e cluster_v "
           "tree cycle]\n"
        << "  graphdro4 export edges <format> <file> [out.edges]\n"
        << "  graphdro4 export json <format> <file> [out.json]   (бонус-формат)\n";
}

// Парсит строку, задающую тип бэкенда для parse
// Возвращает StorageKind::AdjacencyMatrix для "matrix", иначе список смежности
gd4::StorageKind parse_backend(const std::string& s) {
    if (s == "matrix") return gd4::StorageKind::AdjacencyMatrix;
    return gd4::StorageKind::AdjacencyList;
}

// Фабрика парсеров: по имени формата возвращает соответствующий объект-парсер
std::unique_ptr<gd4::GraphParserBase> make_parser(const std::string& fmt) {
    using namespace gd4;
    if (fmt == "edge_list") return std::make_unique<EdgeListParser>();
    if (fmt == "matrix") return std::make_unique<AdjacencyMatrixParser>();
    if (fmt == "dimacs") return std::make_unique<DimacsParser>();
    if (fmt == "snap") return std::make_unique<SnapParser>();
    return nullptr;
}

// Загружает граф из файла с указанным форматом и бэкендом
// Если файл не открыт или формат неизвестен, бросает исключение
gd4::Graph load_graph(const std::string& fmt, const std::string& path, gd4::StorageKind bk) {
    auto p = make_parser(fmt);
    if (!p) throw std::runtime_error("неизвестный формат парсера");
    std::ifstream in(path);
    if (!in) throw std::runtime_error("не удалось открыть файл");
    return p->parse(in, bk);
}

}  // namespace

int main(int argc, char** argv) {
    using namespace gd4;
    if (argc < 2) {
        print_help();
        return 0;
    }
    std::string cmd = argv[1];
    try {
        // Команда help 
        if (cmd == "help" || cmd == "-h" || cmd == "--help") {
            print_help();
            return 0;
        }

        // Демонстрация смены бэкенда
        if (cmd == "backend-demo") {
            Graph g = GenComplete(4).generate(StorageKind::AdjacencyList);
            std::cout << "list: n=" << g.vertex_count() << " m=" << g.edge_count() << "\n";
            g.set_backend(StorageKind::AdjacencyMatrix);
            std::cout << "matrix: n=" << g.vertex_count() << " m=" << g.edge_count() << "\n";
            return 0;
        }

        // Парсинг графа из файла
        // Формат: parse <format> <path> [backend matrix|list]
        if (cmd == "parse" && argc >= 4) {
            std::string fmt = argv[2];
            std::string path = argv[3];
            StorageKind bk = StorageKind::AdjacencyList;
            if (argc >= 6 && std::string(argv[4]) == "backend") bk = parse_backend(argv[5]);
            Graph g = load_graph(fmt, path, bk);
            std::cout << "OK: вершин=" << g.vertex_count() << ", рёбер=" << g.edge_count() << "\n";
            return 0;
        }

        // Вычисление метрик графа из файла
        // Формат: metrics <format> <path>  (всегда использует список смежности)
        if (cmd == "metrics" && argc >= 4) {
            Graph g = load_graph(argv[2], argv[3], StorageKind::AdjacencyList);
            GraphMetrics m(g);
            std::cout << "density=" << m.density() << "\n";
            std::cout << "diameter=" << m.diameter() << "\n";
            std::cout << "transitivity=" << m.transitivity() << "\n";
            std::cout << "components=" << m.components() << "\n";
            std::cout << "articulations=" << m.articulation_count() << "\n";
            std::cout << "bridges=" << m.bridge_count() << "\n";
            std::cout << "bipartite=" << (m.is_bipartite() ? "yes" : "no") << "\n";
            std::cout << "greedy_chi_upper=" << m.greedy_chromatic_upper_bound() << "\n";
            return 0;
        }

        // Генерация графа и экспорт в DOT (если сильно хочется)
        // Формат: gen <kind> <args...> [out.dot]
        // Если указан выходной файл (с расширением .dot), сохраняется граф с подсветкой случайного остова и случайного цикла
        if (cmd == "gen") {
            if (argc < 3) throw std::runtime_error("мало аргументов для gen");
            std::string kind = argv[2];
            Graph g(StorageKind::AdjacencyList);
            int argi = 3;   // индекс первого аргумента после kind

            // Детерминированные генераторы
            if (kind == "kn") {
                g = GenComplete(std::stoi(argv[argi++])).generate();
            } else if (kind == "knm") {
                int a = std::stoi(argv[argi++]);
                int b = std::stoi(argv[argi++]);
                g = GenCompleteBipartite(a, b).generate();
            } else if (kind == "tree") {
                g = GenTree(std::stoi(argv[argi++])).generate();
            } else if (kind == "star") {
                g = GenStar(std::stoi(argv[argi++])).generate();
            } else if (kind == "cycle") {
                g = GenCycle(std::stoi(argv[argi++])).generate();
            } else if (kind == "path") {
                g = GenPath(std::stoi(argv[argi++])).generate();
            } else if (kind == "wheel") {
                g = GenWheel(std::stoi(argv[argi++])).generate();

            // Случайные генераторы
            } else if (kind == "gnp") {
                int gn = std::stoi(argv[argi++]);
                double gp = std::stod(argv[argi++]);
                std::uint64_t gs = std::stoull(argv[argi++]);
                g = GenErdosRenyi(gn, gp, gs).generate();
            } else if (kind == "cubic") {
                int cn = std::stoi(argv[argi++]);
                std::uint64_t cs = std::stoull(argv[argi++]);
                g = GenRandomCubic(cn, cs).generate();

            // Генераторы с точным контролем структурных параметров
            } else if (kind == "components") {
                int c1 = std::stoi(argv[argi++]);  // число вершин
                int c2 = std::stoi(argv[argi++]);  // число компонент
                g = GenComponents(c1, c2).generate();
            } else if (kind == "bridges") {
                int bn = std::stoi(argv[argi++]);  // число вершин
                int bm = std::stoi(argv[argi++]);  // число мостов
                g = GenExactBridges(bn, bm).generate();
            } else if (kind == "art") {
                int an = std::stoi(argv[argi++]);  // число вершин
                int aa = std::stoi(argv[argi++]);  // число точек сочленения
                g = GenExactArticulations(an, aa).generate();
            } else if (kind == "ebcc") {
                int en = std::stoi(argv[argi++]);  // число вершин
                int et = std::stoi(argv[argi++]);  // число компонент рёберной двусвязности
                g = GenExactEdgeBiconnectedComponents(en, et).generate();
            } else if (kind == "halin") {
                // halin N [0|1] [seed]: только «0»/«1» распознаются как флаг sample, чтобы не перепутать с именем файла out.dot 
                int hn = std::stoi(argv[argi++]);   // число вершин
                bool sample = true;
                std::uint64_t hseed = 1;
                if (argi < argc) {
                    std::string a = argv[argi];
                    if (a.find('.') == std::string::npos && (a == "0" || a == "1")) {
                        sample = (a == "1");
                        ++argi;
                    }
                }
                if (argi < argc) {
                    std::string a = argv[argi];
                    if (a.find('.') == std::string::npos) {
                        hseed = std::stoull(a);
                        ++argi;
                    }
                }
                g = GenHalin(hn, sample, hseed).generate();
            } else
                throw std::runtime_error("неизвестный генератор");

            // Если после разбора остался аргумент, и он похож на имя файла (содержит точку), сохраняем граф в DOT с подсветкой остова и цикла
            if (argi < argc) {
                std::string out = argv[argi];
                if (out.find('.') != std::string::npos) {
                    DotSerializeOptions o;
                    o.show_random_spanning_tree = true;
                    o.show_random_simple_cycle = true;
                    DotSerializer ds(o);
                    std::ofstream fo(out);
                    ds.write(g, fo);
                }
            }
            std::cout << "gen OK n=" << g.vertex_count() << " m=" << g.edge_count() << "\n";
            return 0;
        }

        // Экспорт графа в различные форматы
        // Формат: export dot|edges|json <format> <file> [outfile] [flags...]
        if (cmd == "export" && argc >= 5) {
            std::string sub = argv[2];           // dot, edges или json
            std::string fmt = argv[3];           // формат исходного файла (edge_list, matrix итп)
            std::string path = argv[4];           // путь к исходному файлу
            Graph g = load_graph(fmt, path, StorageKind::AdjacencyList);
            std::string outp = (argc >= 6) ? argv[5] : std::string("out.dot");

            if (sub == "dot") {
                DotSerializeOptions o;
                // Обрабатываем флаги из командной строки (начиная с 6-го аргумента)
                for (int i = 6; i < argc; ++i) {
                    std::string f = argv[i];
                    if (f == "cluster_e") o.cluster_edge_biconnected = true;
                    if (f == "cluster_v") o.cluster_vertex_biconnected = true;
                    if (f == "tree") o.show_random_spanning_tree = true;
                    if (f == "cycle") o.show_random_simple_cycle = true;
                }
                DotSerializer ds(o);
                std::ofstream fo(outp);
                ds.write(g, fo);
            } else if (sub == "edges") {
                std::ofstream fo(outp);
                Program4YouEdgesSerializer ser;
                ser.write(g, fo);
            } else if (sub == "json") {
                std::ofstream fo(outp);
                JsonEdgeListSerializer ser;
                ser.write(g, fo);
            } else
                throw std::runtime_error("export: неизвестная подкоманда");
            std::cout << "written " << outp << "\n";
            return 0;
        }

        // Если команда не распознана, выводим справку
        print_help();
    } catch (const std::exception& ex) {
        std::cerr << "Ошибка: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}