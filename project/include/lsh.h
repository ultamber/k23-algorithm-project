#ifndef LSH_HPP
#define LSH_HPP

#include "search_method.hpp"

// LSH (E2LSH) for Euclidean (L2) distance
class LSH : public SearchMethod {
public:
    explicit LSH(const Arguments& args);
    ~LSH() override = default;

    void buildIndex(const Dataset& data) override;
    void search(const Dataset& queries, std::ofstream& output) override;

private:
    // internal dataset copy
    Dataset data_;

    // L hash tables: each maps combined g -> list of indices (ids in data_)
    std::vector<std::unordered_map<std::uint64_t, std::vector<int>>> hashTables_;

    // random projection vectors a and offsets b:
    // projections_[table][i] is the i-th 'a' vector (length = dimension)
    std::vector<std::vector<std::vector<float>>> projections_;
    std::vector<std::vector<double>> offsets_; // offsets_[table][i] = b

    // utilities
    std::uint64_t combineHashes(const std::vector<int>& hashes) const;
    int computeHi(const std::vector<float>& a, double b, const Vector& v, double w) const;
    double euclideanDistance(const Vector& a, const Vector& b) const;

    bool built_ = false;
};

#endif
