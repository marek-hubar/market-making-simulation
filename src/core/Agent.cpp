#include "Agent.hpp"
#include <cmath>
#include <algorithm>
#include <limits>

AvellanedaStoikovAgent::AvellanedaStoikovAgent(const Config& config)
    : gamma_(config.gamma),
      sigma_sq_(config.sigma * config.sigma),
      k_(config.k) {
    
    // Precompute the invariant portion of the optimal spread formula.
    // kappa_term = (1 / gamma) * ln(1 + gamma / k)
    // By calculating this once in the constructor, we save the CPU from 
    // computing an expensive natural logarithm (std::log) on every single time step.
    kappa_term_ = (1.0 / gamma_) * std::log(1.0 + (gamma_ / k_));
}

Spreads AvellanedaStoikovAgent::compute_spreads(
    int current_inventory, 
    double time_to_horizon, 
    double mid_price
) {
    // 1. Calculate the Inventory Penalty and Reservation Price
    // r(s,q,t) = s - q * gamma * sigma^2 * (T - t)
    double inventory_penalty = current_inventory * gamma_ * sigma_sq_ * time_to_horizon;
    double reservation_price = mid_price - inventory_penalty;

    // 2. Calculate the Optimal Spread
    // total_spread = (1/gamma)*ln(1 + gamma/k) + gamma * sigma^2 * (T - t)
    // Note: kappa_term_ holds the precomputed (1/gamma)*ln(1 + gamma/k)
    double variance_term = 0.5 * gamma_ * sigma_sq_ * time_to_horizon;
    double half_total_spread = 0.5 * kappa_term_ + variance_term;

    // 3. Map to Bid/Ask Deltas relative to the Mid-Price
    // bid = r - half_spread 
    // ask = r + half_spread
    double delta_b = mid_price - (reservation_price - half_total_spread);
    double delta_a = (reservation_price + half_total_spread) - mid_price;

    // 4. Microstructure Guardrails: Minimum Tick Size
    // Prevents precision errors from crossing the spread internally
    delta_b = std::max(0.01, delta_b);
    delta_a = std::max(0.01, delta_a);

    // 5. Capital & Risk Constraints (The "Infinite Spread" Pattern)
    
    // // Out of Buying Power? Pull the bid
    // if (current_cash < mid_price) {
    //     delta_b = std::numeric_limits<double>::infinity();
    // }

    // Hard Risk Limit? Pull the ask
    // (Prevents the agent from accumulating an infinitely deep short position)
    int max_short_inventory = -50; // You can also move this to Config later
    if (current_inventory <= max_short_inventory) {
        delta_a = std::numeric_limits<double>::infinity();
    }

    // Optional: Hard Risk Limit for Longs
    int max_long_inventory = 50;
    if (current_inventory >= max_long_inventory) {
        delta_b = std::numeric_limits<double>::infinity();
    }

    return {delta_b, delta_a};
}
