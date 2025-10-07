#ifndef SEARCH_METHOD_HPP
#define SEARCH_METHOD_HPP

#include <vector>
#include <fstream>
#include <string>
#include "dataset.hpp"
#include "utils.hpp"

// Abstract base class for all search algorithms
class SearchMethod {
public:
    explicit SearchMethod(const Arguments& args) : args(args) {}
    virtual ~SearchMethod() = default;

    // Build index using the dataset
    virtual void buildIndex(const Dataset& data) = 0;

    // Perform search for all queries and write results
    virtual void search(const Dataset& queries, std::ofstream& output) = 0;

protected:
    Arguments args; // all runtime parameters (k, L, R, etc.)
};

#endif
