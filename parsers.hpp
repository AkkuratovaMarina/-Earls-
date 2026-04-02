#pragma once
// Парсеры графов из разных форматов
// Базовый класс GraphParserBase задаёт интерфейс: parse() читает граф из потока
// name() возвращает название формата
// Все наследники реализуют конкретные форматы
// При ошибках разбора кидают исключение ParseError

#include "graph.hpp"

#include <istream>
#include <memory>
#include <stdexcept>
#include <string>

namespace gd4 {

// Исключение, которое бросается при ошибке разбора файла
struct ParseError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Базовый класс для всех парсеров (каждый парсер должен уметь читать граф из потока)
// + возвращение Graph с выбранным бэкендом хранения (список смежности или матрица)
class GraphParserBase {
   public:
    virtual ~GraphParserBase() = default;
    virtual Graph parse(std::istream& in, StorageKind backend = StorageKind::AdjacencyList) = 0;
    virtual std::string name() const = 0;
};

// Парсер текстового списка рёбер
// Каждая строка содержит два целых числа (номера вершин), разделённых пробелом или табуляцией
// Допускаются разделители "--" или "->" между числами (ребро в любом случае неориентированное)
// '#' - начало коммента
class EdgeListParser final : public GraphParserBase {
   public:
    Graph parse(std::istream& in, StorageKind backend = StorageKind::AdjacencyList) override;
    std::string name() const override { return "edge_list"; }
};

// Парсер матрицы смежности
// Первая строка — целое число (n)
// Затем n строк, каждая содержит n целых чисел (0 или 1), разделённых пробелами
// Ненулевое значение в позиции (i, j) означает наличие ребра между вершинами i и j
// Диагональные элементы (i == j) игнорируются (петли не добавляются)
class AdjacencyMatrixParser final : public GraphParserBase {
   public:
    Graph parse(std::istream& in, StorageKind backend = StorageKind::AdjacencyList) override;
    std::string name() const override { return "adjacency_matrix"; }
};

// Парсер формата DIMACS (раскраски чееек)
// Строки могут начинаться с 'c' (коммент) или 'p' (описание задачи)
// Строка 'p' имеет вид: p edge N M, где N — число вершин, M — число рёбер
// Затем идут строки 'e u v', где u и v — номера вершин (нумерация с 1)
class DimacsParser final : public GraphParserBase {
   public:
    Graph parse(std::istream& in, StorageKind backend = StorageKind::AdjacencyList) override;
    std::string name() const override { return "dimacs"; }
};

// Парсер формата SNAP (Stanford Network Analysis Platform)
// Каждая строка содержит два целых числа (FromNodeId и ToNodeId), разделённые пробелом или табуляцией
// '#' - начало коммента
// Граф неориентированный, поэтому ребро добавляется в обоих направлениях
// Отрицательные номера вершин пропускаются.
class SnapParser final : public GraphParserBase {
   public:
    Graph parse(std::istream& in, StorageKind backend = StorageKind::AdjacencyList) override;
    std::string name() const override { return "snap"; }
};

}  // namespace gd4