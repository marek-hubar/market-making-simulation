#pragma once
#include <random>

class PriceProcess {
private:
    double current_mid_;
    double dt_;
    double sqrt_dt_times_sigma_;
    double trend_;
    double sigma_;
    std::normal_distribution<double> normal_dist_;
public:
    PriceProcess(double current_mid, double dt, double trend, double sigma)
        : current_mid_(current_mid),
          dt_(dt),
          sqrt_dt_times_sigma_(std::sqrt(dt) * sigma),
          trend_(trend),
          sigma_(sigma),
          normal_dist_(0.0, 1.0) {}

    [[nodiscard]] double get_mid() const { return current_mid_; }

    void reset(double initial_mid) { current_mid_ = initial_mid; }

    template <typename Rng>
    void step(Rng& rng) {
        current_mid_ += trend_ * dt_ + sqrt_dt_times_sigma_ * normal_dist_(rng);
    }
};
