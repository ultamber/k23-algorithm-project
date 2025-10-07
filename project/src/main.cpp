// #include <iostream>
// #include <string>
// #include <chrono>
// #include "dataset.hpp"
// #include "lsh.hpp"
// #include "hypercube.hpp"
// #include "ivfflat.hpp"
// #include "ivfpq.hpp"
// #include "utils.hpp"

// int main(int argc, char* argv[]) {
//     // Parse command line arguments
//     Arguments args = parseArguments(argc, argv);

//     // Load dataset and queries
//     std::cout << "Loading dataset from " << args.inputFile << "..." << std::endl;
//     Dataset data = loadDataset(args.inputFile, args.type);

//     std::cout << "Loading queries from " << args.queryFile << "..." << std::endl;
//     Dataset queries = loadDataset(args.queryFile, args.type);

//     // Prepare output
//     std::ofstream output(args.outputFile);
//     if (!output.is_open()) {
//         std::cerr << "Error: Could not open output file " << args.outputFile << std::endl;
//         return 1;
//     }

//     // Start algorithm
//     auto startTime = std::chrono::high_resolution_clock::now();

//     if (args.useLSH) {
//         output << "LSH\n";
//         LSH lsh(args);
//         lsh.buildIndex(data);
//         lsh.search(queries, output);
//     }
//     else if (args.useHypercube) {
//         output << "Hypercube\n";
//         Hypercube cube(args);
//         cube.buildIndex(data);
//         cube.search(queries, output);
//     }
//     else if (args.useIVFFlat) {
//         output << "IVFFlat\n";
//         IVFFlat ivf(args);
//         ivf.buildIndex(data);
//         ivf.search(queries, output);
//     }
//     else if (args.useIVFPQ) {
//         output << "IVFPQ\n";
//         IVFPQ pq(args);
//         pq.buildIndex(data);
//         pq.search(queries, output);
//     }
//     else {
//         std::cerr << "Error: No algorithm specified (-lsh, -hypercube, -ivfflat, -ivfpq)\n";
//         return 1;
//     }

//     auto endTime = std::chrono::high_resolution_clock::now();
//     double totalTime = std::chrono::duration<double>(endTime - startTime).count();

//     std::cout << "Search completed in " << totalTime << " seconds." << std::endl;
//     output.close();
//     return 0;
// }
#include <iostream>
#include <fstream>
#include <chrono>
#include "dataset.hpp"
#include "utils.hpp"
#include "lsh.hpp"

int main(int argc, char* argv[]) {
    try {
        // 1️⃣ Parse arguments
        Arguments args = parseArguments(argc, argv);
        args.print();

        // 2️⃣ Load datasets
        std::cout << "\nLoading dataset from " << args.inputFile << "..." << std::endl;
        Dataset data = loadDataset(args.inputFile, args.type);

        std::cout << "Loading queries from " << args.queryFile << "..." << std::endl;
        Dataset queries = loadDataset(args.queryFile, args.type);

        // 3️⃣ Prepare output file
        std::ofstream output(args.outputFile);
        if (!output.is_open()) {
            std::cerr << "Error: could not open output file: " << args.outputFile << std::endl;
            return 1;
        }

        // 4️⃣ Select algorithm
        auto startTotal = std::chrono::high_resolution_clock::now();

        if (args.useLSH) {
            LSH lsh(args);
            lsh.buildIndex(data);
            lsh.search(queries, output);
        } else {
            std::cerr << "Error: Only LSH implemented so far (-lsh flag required)\n";
            return 1;
        }

        auto endTotal = std::chrono::high_resolution_clock::now();
        double totalTime = std::chrono::duration<double>(endTotal - startTotal).count();

        std::cout << "\n✅ Search completed successfully in "
                  << totalTime << " seconds.\n";
        output.close();

    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
