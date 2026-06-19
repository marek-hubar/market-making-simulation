#include <iostream>
#include <string>
#include "core/Config.hpp"
#include "core/Simulator.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// Phase 1 main
//
// Currently only validates the config pipeline end-to-end.
// Each subsequent phase will add real simulation logic here.
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {

    const std::string config_path = (argc > 1) ? argv[1] : "config/default.json";

    Config cfg;
    try {
        cfg = Config::from_file(config_path);
    } catch (const std::exception& e) {
        std::cerr << "[error] " << e.what() << "\n";
        return 1;
    }

    // ── Print a summary of the loaded parameters ──────────────────────────────
    std::cout << "─────────────────────────────────────────\n";
    std::cout << "  Avellaneda-Stoikov Simulator  v0.1\n";
    std::cout << "─────────────────────────────────────────\n";
    std::cout << "  Config file   : " << config_path           << "\n\n";

    std::cout << "  [simulation]\n";
    std::cout << "    sigma             = " << cfg.sigma             << "\n";
    std::cout << "    mu                = " << cfg.mu                << "\n";
    std::cout << "    initial_price     = " << cfg.initial_price     << "\n";
    std::cout << "    T                 = " << cfg.T                 << "\n";
    std::cout << "    dt                = " << cfg.dt                << "\n";
    std::cout << "    steps             = " << cfg.steps()           << "\n";
    std::cout << "    num_paths         = " << cfg.num_paths         << "\n";
    std::cout << "    seed              = " << cfg.seed              << "\n";
    std::cout << "    initial_cash      = " << cfg.initial_cash      << "\n";
    std::cout << "    initial_inventory = " << cfg.initial_inventory << "\n\n";

    std::cout << "  [strategy]\n";
    std::cout << "    gamma         = " << cfg.gamma           << "\n";
    std::cout << "    k             = " << cfg.k               << "\n";
    std::cout << "    A             = " << cfg.A               << "\n\n";

    std::cout << "  [inventory_constraint]\n";
    std::cout << "    max_inventory = " << cfg.max_inventory   << "\n\n";

    std::cout << "  [output]\n";
    std::cout << "    output_dir    = " << cfg.output_dir      << "\n";
    std::cout << "─────────────────────────────────────────\n";

    Simulator simulator(cfg);

    AvellanedaStoikovAgent agent_AS(cfg);
    simulator.run_all(agent_AS);
    simulator.write_results_to_csv("data/results_AS.csv");

    double optimal_fixed_half_spread = 1 / cfg.gamma * std::log(1 + cfg.gamma / cfg.k) + cfg.gamma * cfg.sigma * cfg.sigma * cfg.T * 0.25;
    FixedSpreadAgent agent_FS(optimal_fixed_half_spread);
    simulator.run_all(agent_FS);
    simulator.write_results_to_csv("data/results_FS.csv");

    double tau = 1.0;
    optimal_fixed_half_spread = 1 / cfg.gamma * std::log(1 + cfg.gamma / cfg.k);
    double linear_skew = cfg.gamma * cfg.sigma * cfg.sigma * tau;
    LinearSkewAgent agent_LS(optimal_fixed_half_spread, linear_skew);
    simulator.run_all(agent_LS);
    simulator.write_results_to_csv("data/results_LS.csv");

    std::cout << "─────────────────────────────────────────\n";
    std::cout << "           Simulation Results \n";
    std::cout << "─────────────────────────────────────────\n";
    std::cout << "  paths simulated : " << cfg.num_paths << "\n\n";



    return 0;
}
