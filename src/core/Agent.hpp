#pragma once
#include "Config.hpp"

struct Spreads {
    double bid;
    double ask;
};

class Agent {
public:
    virtual ~Agent() = default;
    
    // Pure virtual function declaration
    [[nodiscard]] virtual Spreads compute_spreads(
        int current_inventory, 
        double current_cash, 
        double time_to_horizon, 
        double mid_price
    ) = 0;
};

class AvellanedaStoikovAgent : public Agent {
private:
    double gamma_;
    double sigma_sq_;
    double k_;
    double kappa_term_;

public:
    // Pass config as const reference to prevent accidental modification
    explicit AvellanedaStoikovAgent(const Config& config);

    // EXACT match: name, parameters, and const/noexcept attributes if any
    [[nodiscard]] Spreads compute_spreads(
        int current_inventory, 
        double current_cash, 
        double time_to_horizon, 
        double mid_price
    ) override;
};

class FixedSpreadAgent : public Agent {
private:
    double spread_;
public:
    explicit FixedSpreadAgent(double spread) : spread_(spread) {};
    [[nodiscard]] Spreads compute_spreads(
        int /*current_inventory*/, 
        double /*current_cash*/, 
        double /*time_to_horizon*/, 
        double /*mid_price*/
    ) override {
        return Spreads{spread_, spread_};
    }
};

class LinearSkewAgent : public Agent {
private:
    double spread_;
    double skew_;
public:
    explicit LinearSkewAgent(double spread, double skew) : spread_(spread), skew_(skew) {};
    [[nodiscard]] Spreads compute_spreads(
        int current_inventory, 
        double /*current_cash*/, 
        double /*time_to_horizon*/, 
        double /*mid_price*/
    ) override {
        double comp = skew_ * current_inventory;
        double delta_a = std::max(spread_ - comp, 0.01);
        double delta_b = std::max(spread_ + comp, 0.01);
        return Spreads{delta_b, delta_a};
    }
};
