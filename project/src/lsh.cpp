#include "lsh.hpp"
#include <random>
#include <functional>
#include <chrono>
#include <queue>
#include <unordered_set>
#include <limits>
#include <iomanip>

LSH::LSH(const Arguments& args) : SearchMethod(args) {}

int LSH::computeHi(const std::vector<float>& a, double b, const Vector& v, double w) const {
    // compute dot(a, v)
    double dot = 0.0;
    const auto &vals = v.values;
    size_t D = vals.size();
    for (size_t i = 0; i < D; ++i) dot += static_cast<double>(a[i]) * static_cast<double>(vals[i]);
    double val = (dot + b) / w;
    // floor to integer
    int h = static_cast<int>(std::floor(val));
    return h;
}

std::uint64_t LSH::combineHashes(const std::vector<int>& hashes) const {
    // use a 64-bit hash combine (boost-like)
    std::uint64_t seed = 1469598103934665603ULL; // FNV offset basis
    for (int h : hashes) {
        // Mix bits: combine with golden ratio + shift (similar to boost::hash_combine)
        std::uint64_t k = static_cast<std::uint64_t>(static_cast<uint32_t>(h));
        seed ^= k + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    }
    return seed;
}

double LSH::euclideanDistance(const Vector& a, const Vector& b) const {
    const auto &va = a.values;
    const auto &vb = b.values;
    double s = 0.0;
    size_t D = va.size();
    for (size_t i = 0; i < D; ++i) {
        double diff = static_cast<double>(va[i]) - static_cast<double>(vb[i]);
        s += diff * diff;
    }
    return std::sqrt(s);
}

void LSH::buildIndex(const Dataset& data) {
    if (data.vectors.empty()) {
        std::cerr << "[LSH] Warning: empty dataset\n";
        data_ = data;
        built_ = true;
        return;
    }

    data_ = data;
    int L = args.L;
    int k = args.k;
    double w = args.w;
    int dim = data.dimension;
    unsigned seed = static_cast<unsigned>(args.seed);

    hashTables_.clear();
    hashTables_.resize(L);
    projections_.clear();
    offsets_.clear();

    projections_.resize(L);
    offsets_.resize(L);

    std::mt19937_64 rng(seed);
    std::normal_distribution<double> normal(0.0, 1.0);
    std::uniform_real_distribution<double> uniform(0.0, w);

    // Build random vectors (a) and offsets (b) for each table and each of k functions
    for (int t = 0; t < L; ++t) {
        projections_[t].resize(k);
        offsets_[t].resize(k);
        for (int j = 0; j < k; ++j) {
            projections_[t][j].resize(dim);
            for (int d = 0; d < dim; ++d) {
                projections_[t][j][d] = static_cast<float>(normal(rng));
            }
            offsets_[t][j] = uniform(rng);
        }
    }

    // Insert data vectors into hash tables
    for (int id = 0; id < static_cast<int>(data_.vectors.size()); ++id) {
        const Vector& v = data_.vectors[id];
        for (int t = 0; t < L; ++t) {
            std::vector<int> hvals;
            hvals.reserve(k);
            for (int j = 0; j < k; ++j) {
                int h = computeHi(projections_[t][j], offsets_[t][j], v, w);
                hvals.push_back(h);
            }
            std::uint64_t g = combineHashes(hvals);
            hashTables_[t][g].push_back(id);
        }
    }

    built_ = true;
    std::cout << "[LSH] Built index: L=" << L << " k=" << k << " w=" << w
              << " dataset_size=" << data_.vectors.size()
              << " dim=" << dim << std::endl;
}

void LSH::search(const Dataset& queries, std::ofstream& output) {
    if (!built_) {
        throw std::runtime_error("[LSH] Index not built before search()");
    }

    const int L = args.L;
    const int k = args.k;
    const double w = args.w;
    const int N = args.N;
    const double R = args.R;
    const bool doRange = args.rangeSearch;

    size_t Q = queries.vectors.size();
    if (Q == 0) return;

    // Stats accumulators
    double totalApproxTime = 0.0;
    double totalTrueTime = 0.0;
    double sumAF = 0.0; // average approximation factor sum
    int recallCount = 0;

    // Write method header (the main program may also write it; duplication okay)
    output << "LSH\n";

    // For each query
    for (size_t qi = 0; qi < Q; ++qi) {
        const Vector& qv = queries.vectors[qi];

        // --- approximate search using L hash tables ---
        auto t0 = std::chrono::high_resolution_clock::now();

        std::unordered_set<int> candidates;
        candidates.reserve(1024);

        for (int t = 0; t < L; ++t) {
            std::vector<int> hvals; hvals.reserve(k);
            for (int j = 0; j < k; ++j) {
                int h = computeHi(projections_[t][j], offsets_[t][j], qv, w);
                hvals.push_back(h);
            }
            std::uint64_t g = combineHashes(hvals);
            auto it = hashTables_[t].find(g);
            if (it != hashTables_[t].end()) {
                const auto &bucket = it->second;
                for (int id : bucket) candidates.insert(id);
            }
        }

        // keep a max-heap of size N for approximate nearest (pair<dist, id>)
        using PD = std::pair<double,int>;
        auto cmp = [](const PD &a, const PD &b){ return a.first < b.first; }; // max-heap
        std::priority_queue<PD, std::vector<PD>, decltype(cmp)> approxHeap(cmp);

        // range results (from approximate candidate set, though spec is a bit flexible)
        std::vector<int> rangeNeighbors;

        if (candidates.empty()) {
            // fallback: if no candidates found, we can optionally probe full dataset (but keep consistent)
            // Here we do nothing: approximate result empty.
        } else {
            for (int id : candidates) {
                double dist = euclideanDistance(qv, data_.vectors[id]);
                if (doRange && dist <= R) rangeNeighbors.push_back(id);

                if (N > 0) {
                    if ((int)approxHeap.size() < N) approxHeap.emplace(dist, id);
                    else if (dist < approxHeap.top().first) {
                        approxHeap.pop();
                        approxHeap.emplace(dist, id);
                    }
                }
            }
        }

        // Extract approximate neighbors in ascending distance order
        std::vector<PD> approxSorted;
        approxSorted.reserve(approxHeap.size());
        while (!approxHeap.empty()) {
            approxSorted.push_back(approxHeap.top());
            approxHeap.pop();
        }
        std::reverse(approxSorted.begin(), approxSorted.end()); // now ascending

        auto t1 = std::chrono::high_resolution_clock::now();
        double approxTime = std::chrono::duration<double>(t1 - t0).count();
        totalApproxTime += approxTime;

        // --- exact (true) search by linear scan ---
        auto t2 = std::chrono::high_resolution_clock::now();

        // max-heap for true neighbors
        std::priority_queue<PD, std::vector<PD>, decltype(cmp)> trueHeap(cmp);
        for (int id = 0; id < static_cast<int>(data_.vectors.size()); ++id) {
            double dist = euclideanDistance(qv, data_.vectors[id]);
            if ((int)trueHeap.size() < N) trueHeap.emplace(dist, id);
            else if (dist < trueHeap.top().first) {
                trueHeap.pop();
                trueHeap.emplace(dist, id);
            }
        }
        std::vector<PD> trueSorted;
        trueSorted.reserve(trueHeap.size());
        while (!trueHeap.empty()) {
            trueSorted.push_back(trueHeap.top());
            trueHeap.pop();
        }
        std::reverse(trueSorted.begin(), trueSorted.end()); // ascending

        auto t3 = std::chrono::high_resolution_clock::now();
        double trueTime = std::chrono::duration<double>(t3 - t2).count();
        totalTrueTime += trueTime;

        // --- metrics for this query ---
        // distanceApproximate: distance to nearest approximate neighbor (1st in approxSorted)
        // distanceTrue: distance to true nearest neighbor (trueSorted[0])
        double distApprox = std::numeric_limits<double>::infinity();
        int approxNearestId = -1;
        if (!approxSorted.empty()) {
            distApprox = approxSorted[0].first;
            approxNearestId = approxSorted[0].second;
        }

        double distTrue = std::numeric_limits<double>::infinity();
        int trueNearestId = -1;
        if (!trueSorted.empty()) {
            distTrue = trueSorted[0].first;
            trueNearestId = trueSorted[0].second;
        }

        double af = 1.0;
        if (distTrue > 0 && std::isfinite(distApprox)) af = distApprox / distTrue;
        else if (!std::isfinite(distApprox)) af = std::numeric_limits<double>::infinity();
        sumAF += af;

        // recall@N: whether trueNearestId is among approxSorted
        bool foundTrueInApprox = false;
        for (const auto &p : approxSorted) if (p.second == trueNearestId) { foundTrueInApprox = true; break; }
        if (foundTrueInApprox) recallCount++;

        // --- write output for this query ---
        output << "Query: " << qi << "\n";
        // approximate neighbors
        for (int i = 0; i < N; ++i) {
            if (i < static_cast<int>(approxSorted.size())) {
                output << "Nearest neighbor-" << (i+1) << ": " << approxSorted[i].second << "\n";
                output << "distanceApproximate: " << std::fixed << std::setprecision(6) << approxSorted[i].first << "\n";
                // find corresponding true distance (from trueSorted) for the same neighbor id
                double trueDistForApprox = -1.0;
                for (const auto &tp : trueSorted) {
                    if (tp.second == approxSorted[i].second) { trueDistForApprox = tp.first; break; }
                }
                if (trueDistForApprox < 0) {
                    // not in trueSorted (possible if N true < approx size), compute explicitly
                    trueDistForApprox = euclideanDistance(qv, data_.vectors[approxSorted[i].second]);
                }
                output << "distanceTrue: " << std::fixed << std::setprecision(6) << trueDistForApprox << "\n";
            } else {
                // no approximate neighbor
                output << "Nearest neighbor-" << (i+1) << ": " << -1 << "\n";
                output << "distanceApproximate: " << "inf\n";
                output << "distanceTrue: " << "inf\n";
            }
        }

        // Range neighbors (R-near neighbors)
        output << "R-near neighbors:\n";
        if (doRange) {
            for (int id : rangeNeighbors) {
                output << id << "\n";
            }
        }
        output << "\n";

    } // end queries loop

    // finalize aggregated metrics
    double avgAF = sumAF / static_cast<double>(Q);
    double recallAtN = static_cast<double>(recallCount) / static_cast<double>(Q);
    double qps = static_cast<double>(Q) / totalApproxTime;
    double tApproxAverage = totalApproxTime / static_cast<double>(Q);
    double tTrueAverage = totalTrueTime / static_cast<double>(Q);

    output << "Average AF: " << std::fixed << std::setprecision(6) << avgAF << "\n";
    output << "Recall@N: " << std::fixed << std::setprecision(6) << recallAtN << "\n";
    output << "QPS: " << std::fixed << std::setprecision(6) << qps << "\n";
    output << "tApproximateAverage: " << std::fixed << std::setprecision(6) << tApproxAverage << "\n";
    output << "tTrueAverage: " << std::fixed << std::setprecision(6) << tTrueAverage << "\n";

    // flush to file
    output.flush();
}
