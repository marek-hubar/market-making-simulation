#include <iostream>
#include <string>
#include "core/Config.hpp"
#include "core/Simulator.hpp"
#include "core/Statistics.hpp"

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

    auto extract_pnls = [](const std::vector<PathSummary>& paths) {
        std::vector<double> v;
        v.reserve(paths.size());
        for (const auto& p : paths) v.push_back(p.terminal_pnl);
        return v;
    };

    Simulator simulator(cfg);

    // ── Strategy 1: Avellaneda-Stoikov ────────────────────────────────────────
    {
        AvellanedaStoikovAgent agent(cfg);
        auto results = simulator.run_all(agent);
        auto stats   = aggregate_paths(results);
        print_stats_summary(stats, "Avellaneda-Stoikov");
        write_stats_json("data/results_AS.json", stats, "Avellaneda-Stoikov",
                         extract_pnls(results));
    }

    // ── Strategy 2: Fixed Spread ──────────────────────────────────────────────
    {
        double optimal_fixed_half_spread =
            1.0 / cfg.gamma * std::log(1.0 + cfg.gamma / cfg.k)
            + cfg.gamma * cfg.sigma * cfg.sigma * cfg.T * 0.25;
        FixedSpreadAgent agent(optimal_fixed_half_spread);
        auto results = simulator.run_all(agent);
        auto stats   = aggregate_paths(results);
        print_stats_summary(stats, "FixedSpread");
        write_stats_json("data/results_FS.json", stats, "FixedSpread",
                         extract_pnls(results));
    }

    // ── Strategy 3: LinearSkew ────────────────────────────────────────────────
    {
        double tau = 1.0;
        double base_spread = 1.0 / cfg.gamma * std::log(1.0 + cfg.gamma / cfg.k);
        double linear_skew = cfg.gamma * cfg.sigma * cfg.sigma * tau;
        LinearSkewAgent agent(base_spread, linear_skew);
        auto results = simulator.run_all(agent);
        auto stats   = aggregate_paths(results);
        print_stats_summary(stats, "LinearSkew");
        write_stats_json("data/results_LS.json", stats, "LinearSkew",
                         extract_pnls(results));
    }

    return 0;
}
