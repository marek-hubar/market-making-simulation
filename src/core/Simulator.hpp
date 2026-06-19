#pragma once
#include "Config.hpp"
#include "Portfolio.hpp"
#include "PriceProcess.hpp"
#include "OrderArrivalModel.hpp"
#include "Agent.hpp"


// Simple struct to hold the logged state of each step for Python export
struct SimulationRow {
    double time;
    double mid_price;
    int inventory;
    double cash;
    double pnl;
    double bid_spread;
    double ask_spread;
    int path_id = 0;
};


class Simulator {
private:
    const Config& config_;
    Portfolio portfolio_;
    PriceProcess price_process_;
    OrderArrivalModel arrival_model_;
    std::vector<SimulationRow> logs_;

public:
    void write_results_to_csv(const std::string& filename) const;
    double get_mtm_pnl() { return portfolio_.get_mtm_pnl(price_process_.get_mid()); }
    Simulator(const Config& config);
    void run(Agent& agent);
    void run_all(Agent& agent);
};
