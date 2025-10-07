#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <iostream>
#include <unordered_map>

// Structure to store all parameters
struct Arguments {
    // Required
    std::string inputFile;
    std::string queryFile;
    std::string outputFile;
    std::string type;  // "mnist" or "sift"

    // General
    int N = 1;         // Number of nearest neighbors
    double R = 2000.0; // Search radius
    bool rangeSearch = false;
    int seed = 1;

    // Algorithm flags
    bool useLSH = false;
    bool useHypercube = false;
    bool useIVFFlat = false;
    bool useIVFPQ = false;

    // LSH parameters
    int k = 4;
    int L = 5;
    double w = 4.0;

    // Hypercube parameters
    int kproj = 14;
    int M = 10;
    int probes = 2;

    // IVF parameters (both IVFFlat & IVFPQ)
    int kclusters = 50;
    int nprobe = 5;

    // IVFPQ-specific
    int nbits = 8;
    int Msubvectors = 16;

    // Debug helper
    void print() const {
        std::cout << "Arguments summary:\n";
        std::cout << "Input: " << inputFile << "\nQuery: " << queryFile
                  << "\nOutput: " << outputFile << "\nType: " << type << "\n";
        if (useLSH) std::cout << "Algorithm: LSH\n";
        if (useHypercube) std::cout << "Algorithm: Hypercube\n";
        if (useIVFFlat) std::cout << "Algorithm: IVFFlat\n";
        if (useIVFPQ) std::cout << "Algorithm: IVFPQ\n";
        std::cout << "N=" << N << ", R=" << R << ", seed=" << seed << "\n";
    }
};

// Function declaration
Arguments parseArguments(int argc, char* argv[]);

#endif
