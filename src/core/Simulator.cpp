#include "Simulator.hpp"
#include <fstream>
#include <pcg_random.hpp>
#include <iomanip>
#include <stdexcept>

Simulator::Simulator(const Config& config)
    : config_(config),
      portfolio_(config),
      price_process_(config.initial_price, config.dt, config.mu, config.sigma),
      arrival_model_(config.A, config.k, config.dt, config.mu) {
    logs_.reserve(config.steps());
}


void Simulator::write_results_to_csv(const std::string& filename) const {
    std::ofstream file(filename);
    
    // Always check if the OS actually granted file access
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for CSV export: " + filename);
    }

    // 1. Write the CSV Header
    file << "path_id,time,mid_price,inventory,cash,pnl,bid_spread,ask_spread\n";

    // 2. Lock float precision
    // Microstructure spreads are tiny (e.g., 0.005). 
    // Without std::fixed and setprecision, C++ might truncate them or use scientific notation.
    file << std::fixed << std::setprecision(6);

    // 3. Write the data rows
    for (const auto& row : logs_) {
        file << row.path_id << ","
             << row.time << ","
             << row.mid_price << ","
             << row.inventory << ","
             << row.cash << ","
             << row.pnl << ","
             << row.bid_spread << ","
             << row.ask_spread << "\n"; 
    }

    file.close();
}


void Simulator::run(Agent& agent) {
    pcg64 rng(config_.seed);
    double current_time = 0.0;

    int simulation_steps = config_.steps();
    for (int step=0; step<simulation_steps; step++) {
        double mid = price_process_.get_mid();
        double cash = portfolio_.get_cash();
        int inv = portfolio_.get_inventory();

        Spreads spreads = agent.compute_spreads(inv, cash, config_.T - current_time, mid);

        FillResult fill_res = arrival_model_.simulate_fills(spreads.bid, spreads.ask, rng);

        if (fill_res.bid_filled) {
            portfolio_.add_fill(+1, mid - spreads.bid);
        }
        if (fill_res.ask_filled) {
            portfolio_.add_fill(-1, mid + spreads.ask);
        }

        logs_.push_back({current_time,
                         mid,
                         portfolio_.get_inventory(),
                         portfolio_.get_cash(),
                         portfolio_.get_mtm_pnl(mid),
                         spreads.bid,
                         spreads.ask});

        price_process_.step(rng);
        current_time += config_.dt;
    }
    double final_mid = price_process_.get_mid();
    int final_inv = portfolio_.get_inventory();

    double penalty_spread = 0.0;
    if (final_inv != 0) {
        penalty_spread = 0.5;
        if (final_inv > 0) portfolio_.add_fill(-final_inv, final_mid - penalty_spread);
        else portfolio_.add_fill(-final_inv, final_mid + penalty_spread);
    }

    logs_.push_back({config_.T,
                     final_mid,
                     0,
                     portfolio_.get_cash(),
                     portfolio_.get_mtm_pnl(final_mid),
                     penalty_spread,
                     penalty_spread});
}

void Simulator::run_all(Agent& agent) {
    logs_.clear();
    logs_.reserve(static_cast<size_t>(config_.num_paths) * (config_.steps() + 1));

    for (int path = 0; path < config_.num_paths; ++path) {
        portfolio_.reset();
        price_process_.reset(config_.initial_price);
        pcg64 rng(config_.seed + static_cast<unsigned long>(path));

        double current_time = 0.0;
        int simulation_steps = config_.steps();

        for (int step = 0; step < simulation_steps; step++) {
            double mid = price_process_.get_mid();
            double cash = portfolio_.get_cash();
            int inv = portfolio_.get_inventory();

            Spreads spreads = agent.compute_spreads(inv, cash, config_.T - current_time, mid);
            FillResult fill_res = arrival_model_.simulate_fills(spreads.bid, spreads.ask, rng);

            if (fill_res.bid_filled) {
                portfolio_.add_fill(+1, mid - spreads.bid);
            }
            if (fill_res.ask_filled) {
                portfolio_.add_fill(-1, mid + spreads.ask);
            }

            logs_.push_back({current_time,
                             mid,
                             portfolio_.get_inventory(),
                             portfolio_.get_cash(),
                             portfolio_.get_mtm_pnl(mid),
                             spreads.bid,
                             spreads.ask,
                             path});

            price_process_.step(rng);
            current_time += config_.dt;
        }

        double final_mid = price_process_.get_mid();
        int final_inv = portfolio_.get_inventory();
        double slippage = 0.0;

        if (final_inv != 0) {
            // Baseline cost to cross the spread (e.g., 1 tick)
            double baseline_half_spread = 1 / config_.gamma * std::log(1 + config_.gamma / config_.k);
            
            // Market impact coefficient (eta). 
            // e.g., Every 10 units of inventory pushes the execution price 1 cent further away.
            double eta = 0.0001; 

            // Total penalty scales with the absolute size of the inventory block
            slippage = baseline_half_spread + (eta * std::abs(final_inv));
            
            if (final_inv > 0) {
                // We are long, dump to the bid side (lower price)
                double execution_price = final_mid - slippage;
                portfolio_.add_fill(-final_inv, execution_price);
            } else {
                // We are short, buy from the ask side (higher price)
                double execution_price = final_mid + slippage;
                portfolio_.add_fill(-final_inv, execution_price);
            }
        }

        logs_.push_back({config_.T,
                         final_mid,
                         0,
                         portfolio_.get_cash(),
                         portfolio_.get_mtm_pnl(final_mid),
                         slippage,
                         slippage,
                         path});
    }
}
