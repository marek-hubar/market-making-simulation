#include <random>

struct FillResult {
    bool bid_filled = false;
    bool ask_filled = false;
};

class OrderArrivalModel {
private:
    double A_;
    double k_;
    double dt_;
    double mu_;
    std::uniform_real_distribution<double> uniform_dist_;
    
    [[nodiscard]] double calculate_prob(double delta) { return A_ * dt_ * std::exp(-k_ * delta); }
public:
    explicit OrderArrivalModel(double A, double k, double dt, double mu)
        : A_(A), k_(k), dt_(dt), mu_(mu), uniform_dist_(0.0, 1.0) {}

    template <typename Rng>
    [[nodiscard]] FillResult simulate_fills(double delta_b, double delta_a, Rng& rng) {
        FillResult result;
        
        double delta_b_eff = std::max(0.0, delta_b + mu_ * dt_);
        double delta_a_eff = std::max(0.0, delta_a - mu_ * dt_);

        double prob_b = calculate_prob(delta_b_eff);
        double prob_a = calculate_prob(delta_a_eff);

        result.bid_filled = (uniform_dist_(rng) < prob_b);
        result.ask_filled = (uniform_dist_(rng) < prob_a);
        
        return result;
    }
};
