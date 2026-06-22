#pragma once
#include "Config.hpp"

class Portfolio {
private:
    const Config& config_;
    int q_;
    double cumulative_pnl_;
public:
    Portfolio(const Config& config)
        : config_(config), q_(config.initial_inventory), cumulative_pnl_(0.0) {}

    [[nodiscard]] int get_inventory() const { return q_; }
    [[nodiscard]] double get_pnl() const { return cumulative_pnl_; }

    void add_fill(int quantity, double quote_spread) {
        cumulative_pnl_ += static_cast<double>(std::abs(quantity)) * quote_spread;
        q_ += quantity;
    }

    void apply_price_change(double new_mid, double old_mid) {
        cumulative_pnl_ += static_cast<double>(q_) * (new_mid - old_mid);
    }

    void reset() { q_ = config_.initial_inventory; cumulative_pnl_ = 0.0; }

    int get_inventory() { return q_; }
};
