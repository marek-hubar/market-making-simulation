#include "Simulator.hpp"
#include <cmath>
#include <pcg_random.hpp>

Simulator::Simulator(const Config& config)
    : config_(config),
      portfolio_(config),
      price_process_(config.initial_price, config.dt, config.mu, config.sigma),
      arrival_model_(config.A, config.k, config.dt, config.mu) {}


PathSummary Simulator::run_path(unsigned long seed, Agent& agent) {
    portfolio_.reset();
    price_process_.reset(config_.initial_price);
    pcg64 rng(seed);

    stats_.reset();

    double current_time = 0.0;
    int simulation_steps = config_.steps();

    for (int step = 0; step < simulation_steps; step++) {
        double mid = price_process_.get_mid();
        int inv = portfolio_.get_inventory();

        Spreads spreads = agent.compute_spreads(inv, portfolio_.get_cash(),
                                                config_.T - current_time, mid);
        FillResult fill_res = arrival_model_.simulate_fills(spreads.bid, spreads.ask, rng);

        if (fill_res.bid_filled) {
            portfolio_.add_fill(+1, mid - spreads.bid);
        }
        if (fill_res.ask_filled) {
            portfolio_.add_fill(-1, mid + spreads.ask);
        }

        double total_spread = spreads.bid + spreads.ask;
        stats_.record_step(current_time, mid,
                           portfolio_.get_inventory(),
                           portfolio_.get_mtm_pnl(mid),
                           total_spread, fill_res);

        price_process_.step(rng);
        current_time += config_.dt;
    }

    int inv_before_liq = portfolio_.get_inventory();
    double final_mid = price_process_.get_mid();
    double slippage = 0.0;

    if (inv_before_liq != 0) {
        double baseline_half_spread = 1.0 / config_.gamma
                                      * std::log(1.0 + config_.gamma / config_.k);
        double eta = 0.0001;
        slippage = baseline_half_spread + (eta * std::abs(inv_before_liq));

        if (inv_before_liq > 0) {
            portfolio_.add_fill(-inv_before_liq, final_mid - slippage);
        } else {
            portfolio_.add_fill(-inv_before_liq, final_mid + slippage);
        }
    }

    return stats_.finalize(inv_before_liq, final_mid,
                           portfolio_.get_cash(),
                           portfolio_.get_mtm_pnl(final_mid));
}

PathSummary Simulator::run(Agent& agent) {
    return run_path(config_.seed, agent);
}

std::vector<PathSummary> Simulator::run_all(Agent& agent) {
    std::vector<PathSummary> results;
    results.reserve(static_cast<std::size_t>(config_.num_paths));

    for (int path = 0; path < config_.num_paths; ++path) {
        results.push_back(run_path(
            config_.seed + static_cast<unsigned long>(path), agent));
    }
    return results;
}
