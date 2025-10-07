#include "dataset.hpp"
#include <fstream>
#include <stdexcept>
#include <cstdint>
#include <iostream>

// Helper to convert big-endian (MNIST header)
uint32_t readBigEndianUint32(std::ifstream &file) {
    unsigned char bytes[4];
    file.read(reinterpret_cast<char*>(bytes), 4);
    return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

Dataset loadMNIST(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Cannot open MNIST file: " + filename);

    Dataset dataset;

    uint32_t magic = readBigEndianUint32(file);
    uint32_t numImages = readBigEndianUint32(file);
    uint32_t numRows = readBigEndianUint32(file);
    uint32_t numCols = readBigEndianUint32(file);

    if (magic != 2051)
        throw std::runtime_error("Invalid MNIST magic number (expected 2051).");

    dataset.dimension = numRows * numCols;
    dataset.vectors.resize(numImages);

    std::cout << "MNIST dataset: " << numImages
              << " images, " << dataset.dimension << " dimensions each." << std::endl;

    for (uint32_t i = 0; i < numImages; ++i) {
        dataset.vectors[i].values.resize(dataset.dimension);
        for (int j = 0; j < dataset.dimension; ++j) {
            unsigned char pixel;
            file.read(reinterpret_cast<char*>(&pixel), sizeof(pixel));
            dataset.vectors[i].values[j] = static_cast<float>(pixel);
        }
    }

    return dataset;
}

Dataset loadSIFT(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Cannot open SIFT file: " + filename);

    Dataset dataset;
    std::vector<Vector> vecs;

    while (true) {
        int dim;
        file.read(reinterpret_cast<char*>(&dim), sizeof(int));
        if (file.eof()) break;

        if (dataset.dimension == 0) dataset.dimension = dim;
        else if (dim != dataset.dimension)
            throw std::runtime_error("Inconsistent dimension in SIFT file.");

        Vector v;
        v.values.resize(dim);
        file.read(reinterpret_cast<char*>(v.values.data()), sizeof(float) * dim);
        if (!file) break; // stop on EOF
        vecs.push_back(std::move(v));
    }

    dataset.vectors = std::move(vecs);

    std::cout << "SIFT dataset: " << dataset.vectors.size()
              << " vectors, " << dataset.dimension << " dimensions each." << std::endl;

    return dataset;
}

Dataset loadDataset(const std::string& filename, const std::string& type) {
    if (type == "mnist")
        return loadMNIST(filename);
    else if (type == "sift")
        return loadSIFT(filename);
    else
        throw std::invalid_argument("Unknown dataset type: " + type);
}
