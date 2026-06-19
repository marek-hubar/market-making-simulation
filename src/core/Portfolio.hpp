#pragma once
#include "Config.hpp"

class Portfolio {
private:
    const Config& config_;
    int q_;
    double cash_;
public:
    // Portfolio() : q_(0), cash_(0.0) {}
    Portfolio(const Config& config) : config_(config), q_(config.initial_inventory), cash_(config.initial_cash) {}

    [[nodiscard]] int get_inventory() const { return q_; }
    [[nodiscard]] double get_cash() const { return cash_; }
    [[nodiscard]] double get_total_wealth(double current_mid) const { return cash_ + q_ * current_mid; }
    [[nodiscard]] double get_mtm_pnl(double current_mid) const { return get_total_wealth(current_mid) - config_.initial_cash - config_.initial_inventory * config_.initial_price; }

    // The quantity is signed, i.e. negative for sell and positive for buy
    void add_fill(int quantity, double price) {
        q_ += quantity;
        cash_ -= quantity * price;
    }

    void reset() { q_ = config_.initial_inventory; cash_ = config_.initial_cash; }

    double get_cash() { return cash_; }
    int get_inventory() { return q_; }
};
