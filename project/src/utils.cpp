#include "utils.hpp"
#include <stdexcept>

Arguments parseArguments(int argc, char* argv[]) {
    Arguments args;

    std::unordered_map<std::string, std::string> options;

    for (int i = 1; i < argc; ++i) {
        std::string flag = argv[i];

        // Boolean flags
        if (flag == "-lsh") { args.useLSH = true; continue; }
        if (flag == "-hypercube") { args.useHypercube = true; continue; }
        if (flag == "-ivfflat") { args.useIVFFlat = true; continue; }
        if (flag == "-ivfpq") { args.useIVFPQ = true; continue; }

        // Range flag (true/false)
        if (flag == "-range" && i + 1 < argc) {
            std::string val = argv[++i];
            args.rangeSearch = (val == "true" || val == "1");
            continue;
        }

        // Other options (expect a value)
        if (i + 1 < argc) {
            std::string value = argv[++i];
            options[flag] = value;
        }
    }

    // Helper lambda for safe extraction
    auto get = [&](const std::string& key, std::string& dest) {
        if (options.count(key)) dest = options[key];
    };
    auto geti = [&](const std::string& key, int& dest) {
        if (options.count(key)) dest = std::stoi(options[key]);
    };
    auto getd = [&](const std::string& key, double& dest) {
        if (options.count(key)) dest = std::stod(options[key]);
    };

    // Read basic arguments
    get("-d", args.inputFile);
    get("-q", args.queryFile);
    get("-o", args.outputFile);
    get("-type", args.type);

    geti("-N", args.N);
    getd("-R", args.R);
    geti("-seed", args.seed);

    // LSH
    geti("-k", args.k);
    geti("-L", args.L);
    getd("-w", args.w);

    // Hypercube
    geti("-kproj", args.kproj);
    geti("-M", args.M);
    geti("-probes", args.probes);

    // IVFFlat / IVFPQ
    geti("-kclusters", args.kclusters);
    geti("-nprobe", args.nprobe);

    // IVFPQ-specific
    geti("-nbits", args.nbits);
    geti("-Msub", args.Msubvectors);

    // Validate required args
    if (args.inputFile.empty() || args.queryFile.empty() || args.outputFile.empty() || args.type.empty()) {
        throw std::invalid_argument("Missing required arguments (-d, -q, -o, -type).");
    }

    // Ensure one algorithm is selected
    int algoCount = (args.useLSH + args.useHypercube + args.useIVFFlat + args.useIVFPQ);
    if (algoCount != 1) {
        throw std::invalid_argument("You must specify exactly one algorithm: -lsh, -hypercube, -ivfflat, or -ivfpq.");
    }

    return args;
}
