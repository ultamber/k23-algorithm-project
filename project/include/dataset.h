#ifndef DATASET_HPP
#define DATASET_HPP

#include <string>
#include <vector>
#include <iostream>

struct Vector {
    std::vector<float> values;
};

struct Dataset {
    std::vector<Vector> vectors;
    int dimension = 0;
};

// Function declarations
Dataset loadDataset(const std::string& filename, const std::string& type);
Dataset loadMNIST(const std::string& filename);
Dataset loadSIFT(const std::string& filename);

#endif
