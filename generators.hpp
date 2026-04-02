#pragma once
// Параметризованные генераторы графов (полный/двудольный/деревья/циклы/колесо и ляля)
// Случайные (зависят от сида в конструкторе) (generators.cpp)

#include "graph.hpp"

#include <cstdint>

namespace gd4 {

// Можно добавлять новые семейства без перекомпиляции остального кода
class IGraphGenerator {
   public:
    virtual ~IGraphGenerator() = default;
    virtual Graph generate(StorageKind backend = StorageKind::AdjacencyList) const = 0;
};

class GenComplete final : public IGraphGenerator {
   public:
    explicit GenComplete(int n) : n_(n) {}
    Graph generate(StorageKind backend = StorageKind::AdjacencyList) const override;

   private:
    int n_;
};

class GenCompleteBipartite final : public IGraphGenerator {
   public:
    GenCompleteBipartite(int n1, int n2) : n1_(n1), n2_(n2) {}
    Graph generate(StorageKind backend = StorageKind::AdjacencyList) const override;

   private:
    int n1_, n2_;
};

class GenTree final : public IGraphGenerator {
   public:
    explicit GenTree(int n) : n_(n) {}
    Graph generate(StorageKind backend = StorageKind::AdjacencyList) const override;

   private:
    int n_;
};

class GenStar final : public IGraphGenerator {
   public:
    // Звезда S_k : k листьев + центр -> n=k+1 вершин
    explicit GenStar(int num_vertices) : n_(num_vertices) {}
    Graph generate(StorageKind backend = StorageKind::AdjacencyList) const override;

   private:
    int n_;
};

class GenCycle final : public IGraphGenerator {
   public:
    explicit GenCycle(int n) : n_(n) {}
    Graph generate(StorageKind backend = StorageKind::AdjacencyList) const override;

   private:
    int n_;
};

class GenPath final : public IGraphGenerator {
   public:
    explicit GenPath(int n) : n_(n) {}
    Graph generate(StorageKind backend = StorageKind::AdjacencyList) const override;

   private:
    int n_;
};

class GenWheel final : public IGraphGenerator {
   public:
    // Wn: n вершин (n >= 4) (один центр и (n-1)-цикл)
    explicit GenWheel(int n) : n_(n) {}
    Graph generate(StorageKind backend = StorageKind::AdjacencyList) const override;

   private:
    int n_;
};

class GenErdosRenyi final : public IGraphGenerator {
   public:
    GenErdosRenyi(int n, double p, std::uint64_t seed) : n_(n), p_(p), seed_(seed) {}
    Graph generate(StorageKind backend = StorageKind::AdjacencyList) const override;

   private:
    int n_;
    double p_;
    std::uint64_t seed_;
};

class GenRandomCubic final : public IGraphGenerator {
   public:
    GenRandomCubic(int n, std::uint64_t seed) : n_(n), seed_(seed) {}
    Graph generate(StorageKind backend = StorageKind::AdjacencyList) const override;

   private:
    int n_;
    std::uint64_t seed_;
};

class GenComponents final : public IGraphGenerator {
   public:
    GenComponents(int n, int components) : n_(n), k_(components) {}
    Graph generate(StorageKind backend = StorageKind::AdjacencyList) const override;

   private:
    int n_, k_;
};

class GenExactBridges final : public IGraphGenerator {
   public:
    GenExactBridges(int n, int bridges) : n_(n), b_(bridges) {}
    Graph generate(StorageKind backend = StorageKind::AdjacencyList) const override;

   private:
    int n_, b_;
};

class GenExactArticulations final : public IGraphGenerator {
   public:
    GenExactArticulations(int n, int articulations) : n_(n), a_(articulations) {}
    Graph generate(StorageKind backend = StorageKind::AdjacencyList) const override;

   private:
    int n_, a_;
};

// 2-мосты: число компонент рёберной двусвязности в связном графе = T
class GenExactEdgeBiconnectedComponents final : public IGraphGenerator {
   public:
    GenExactEdgeBiconnectedComponents(int n, int ebcc_count) : n_(n), t_(ebcc_count) {}
    Graph generate(StorageKind backend = StorageKind::AdjacencyList) const override;

   private:
    int n_, t_;
};

class GenHalin final : public IGraphGenerator {
   public:
    // Если use_sample: одна из заготовок 
    explicit GenHalin(int n, bool use_sample, std::uint64_t seed);
    Graph generate(StorageKind backend = StorageKind::AdjacencyList) const override;

   private:
    int n_;
    bool sample_;
    std::uint64_t seed_;
};

} 
