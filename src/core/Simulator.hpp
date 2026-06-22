#pragma once
#include "Config.hpp"
#include "Portfolio.hpp"
#include "PriceProcess.hpp"
#include "OrderArrivalModel.hpp"
#include "Agent.hpp"
#include "Statistics.hpp"

class Simulator {
private:
    const Config& config_;
    Portfolio portfolio_;
    PriceProcess price_process_;
    OrderArrivalModel arrival_model_;
    StatisticsCollector stats_;

    PathSummary run_path(unsigned long seed, Agent& agent);

public:
    Simulator(const Config& config);
    PathSummary run(Agent& agent);
    std::vector<PathSummary> run_all(Agent& agent);
};
