#pragma once
#include "Config.hpp"
#include "Portfolio.hpp"
#include "PriceProcess.hpp"
#include "OrderArrivalModel.hpp"
#include "Agent.hpp"
#include "Statistics.hpp"

struct SimulationResult {
    std::vector<PathSummary> path_summaries;
    std::vector<double> time_points;
    std::vector<double> abs_inv_mean;
    std::vector<double> abs_inv_std;
};

class Simulator {
private:
    const Config& config_;
    Portfolio portfolio_;
    PriceProcess price_process_;
    OrderArrivalModel arrival_model_;
    StatisticsCollector stats_;

    PathSummary run_path(unsigned long seed, Agent& agent,
                         std::vector<double>* sum_abs = nullptr,
                         std::vector<double>* sum_sq  = nullptr);

public:
    Simulator(const Config& config);
    PathSummary run(Agent& agent);
    SimulationResult run_all(Agent& agent);
};
