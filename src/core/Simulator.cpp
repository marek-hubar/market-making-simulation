#include "Simulator.hpp"
#include <pcg_random.hpp>

Simulator::Simulator(const Config& config)
    : config_(config),
      portfolio_(config),
      price_process_(config.initial_price, config.dt, config.mu, config.sigma),
      arrival_model_(config.A, config.k, config.dt, config.mu) {}


PathSummary Simulator::run_path(unsigned long seed, Agent& agent,
                                std::vector<double>* sum_abs,
                                std::vector<double>* sum_sq) {
    portfolio_.reset();
    price_process_.reset(config_.initial_price);
    pcg64 rng(seed);

    stats_.reset();

    double current_time = 0.0;
    int simulation_steps = config_.steps();

    for (int step = 0; step < simulation_steps; step++) {
        double mid = price_process_.get_mid();
        int inv = portfolio_.get_inventory();

        Spreads spreads = agent.compute_spreads(inv,
                                                config_.T - current_time, mid);
        FillResult fill_res = arrival_model_.simulate_fills(spreads.bid, spreads.ask, rng);

        if (fill_res.bid_filled) {
            portfolio_.add_fill(+1, spreads.bid);
        }
        if (fill_res.ask_filled) {
            portfolio_.add_fill(-1, spreads.ask);
        }

        double total_spread = spreads.bid + spreads.ask;
        stats_.record_step(current_time, mid,
                           portfolio_.get_inventory(),
                           portfolio_.get_pnl(),
                           total_spread, fill_res);

        if (sum_abs) {
            double a = static_cast<double>(std::abs(portfolio_.get_inventory()));
            (*sum_abs)[step] += a;
            if (sum_sq) (*sum_sq)[step] += a * a;
        }

        double old_mid = mid;
        price_process_.step(rng);
        current_time += config_.dt;
        portfolio_.apply_price_change(price_process_.get_mid(), old_mid);
    }

    double final_mid = price_process_.get_mid();

    return stats_.finalize(portfolio_.get_inventory(), final_mid,
                           portfolio_.get_pnl());
}

PathSummary Simulator::run(Agent& agent) {
    return run_path(config_.seed, agent);
}

SimulationResult Simulator::run_all(Agent& agent) {
    int steps = config_.steps();
    std::vector<double> sum_abs(steps, 0.0);
    std::vector<double> sum_sq(steps, 0.0);

    SimulationResult res;
    res.path_summaries.reserve(static_cast<std::size_t>(config_.num_paths));

    for (int path = 0; path < config_.num_paths; ++path) {
        res.path_summaries.push_back(run_path(
            config_.seed + static_cast<unsigned long>(path), agent,
            &sum_abs, &sum_sq));
    }

    int n = config_.num_paths;
    res.abs_inv_mean.resize(steps);
    res.abs_inv_std.resize(steps);
    for (int s = 0; s < steps; ++s) {
        double mean = sum_abs[s] / static_cast<double>(n);
        double var  = sum_sq[s] / static_cast<double>(n) - mean * mean;
        double sample_var = n > 1 ? var * n / static_cast<double>(n - 1) : 0.0;
        res.abs_inv_mean[s] = mean;
        res.abs_inv_std[s]  = std::sqrt(std::max(0.0, sample_var));
    }

    res.time_points.reserve(steps);
    for (int s = 0; s < steps; ++s)
        res.time_points.push_back(s * config_.dt);

    return res;
}
