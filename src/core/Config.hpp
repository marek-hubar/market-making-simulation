#pragma once

#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

// ─────────────────────────────────────────────────────────────────────────────
// Config
//
// Single source of truth for every tunable parameter.  All other classes take
// a const-ref to Config so experiments are controlled exclusively by JSON.
//
// Parameter conventions
// ─────────────────────
//   Time units   : T and dt are in the same abstract unit (e.g. "1 trading day"
//                  normalised to 1.0).  Adjust sigma to match.
//   Price units  : S_0 and sigma are in the same units (e.g. dollars).
//   A-S model    : follows Avellaneda & Stoikov (2008), QF 8(3) 217-224.
//
// The Avellaneda-Stoikov formulas assume arithmetic Brownian motion:
//   dS = sigma * dW
// NOT geometric (log-normal).  sigma here is volatility in price units per
// sqrt(time unit), NOT fractional/percentage volatility.
// ─────────────────────────────────────────────────────────────────────────────

struct Config {

    // ── Price process ─────────────────────────────────────────────────────────
    double sigma         = 0.01;   // price volatility  [price / sqrt(time)]
    double mu            = 0.0;    // drift / trend     [price / time]
    double initial_price = 100.0;  // S_0               [price]

    // ── Time ──────────────────────────────────────────────────────────────────
    double T  = 1.0;    // trading horizon (1 = one full session)
    double dt = 0.005;  // Euler time step (200 steps per session by default)

    // ── Avellaneda-Stoikov parameters ─────────────────────────────────────────
    double gamma = 0.1;  // CARA risk-aversion coefficient (gamma > 0)
                         //   small gamma  → aggressive, wide inventory tolerance
                         //   large gamma  → conservative, tight inventory control

    double k = 1.5;      // order-arrival intensity decay  (k > 0)
                         //   lambda(delta) = A * exp(-k * delta)
                         //   large k → fill rate drops sharply with spread

    double A = 1.0;      // baseline order arrival rate  [fills / time unit]
                         //   scales the absolute fill frequency

    // ── Inventory constraint (InvConstrained strategy only) ───────────────────
    int max_inventory = 50;  // hard position limit in [+/- max_inventory]

    double initial_cash = 1000.0;
    int initial_inventory = 0.0;


    // ── Monte Carlo ───────────────────────────────────────────────────────────
    int           num_paths = 1000;
    unsigned long seed      = 42;    // base RNG seed; path i uses seed + i

    // ── Output ────────────────────────────────────────────────────────────────
    std::string output_dir = "data/output/default";

    // ─────────────────────────────────────────────────────────────────────────
    // Derived quantities
    // ─────────────────────────────────────────────────────────────────────────

    // Number of Euler steps in one simulation path.
    // std::round avoids floating-point rounding turning 1.0/0.005 into 199.
    [[nodiscard]] int steps() const {
        return static_cast<int>(std::round(T / dt));
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Validation
    // ─────────────────────────────────────────────────────────────────────────
    void validate() const {
        auto require = [](bool cond, const char* msg) {
            if (!cond) throw std::invalid_argument(msg);
        };
        require(sigma             > 0.0, "sigma must be positive");
        require(gamma             > 0.0, "gamma must be positive");
        require(k                 > 0.0, "k must be positive");
        require(A                 > 0.0, "A must be positive");
        require(T                 > 0.0, "T must be positive");
        require(dt                > 0.0, "dt must be positive");
        require(dt                < T,   "dt must be smaller than T");
        require(num_paths         > 0,   "num_paths must be positive");
        require(max_inventory     > 0,"max_inventory must be positive");
        require(initial_price     > 0.0, "initial_price must be positive");
        require(initial_cash      >= 0.0, "initial_cash must be non-negative");
        require(initial_inventory >= 0, "initial_inventory must be non-negative");

        // Sanity check: fill probability per step should be < 1 at a spread
        // of zero.  P = A * dt must be < 1, otherwise the Bernoulli model
        // loses meaning (you'd always get filled at any spread).
        if (A * dt >= 1.0) {
            throw std::invalid_argument(
                "A * dt >= 1 produces fill probabilities >= 1. "
                "Reduce A or dt so that A * dt < 1."
            );
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Factory: build from a parsed nlohmann::json object
    // ─────────────────────────────────────────────────────────────────────────
    [[nodiscard]] static Config from_json(const nlohmann::json& j) {
        Config c;  // start from defaults

        if (j.contains("simulation")) {
            const auto& s = j["simulation"];
            c.sigma             = s.value("sigma",         c.sigma);
            c.mu                = s.value("mu",            c.mu);
            c.initial_price     = s.value("initial_price", c.initial_price);
            c.T                 = s.value("T",             c.T);
            c.dt                = s.value("dt",            c.dt);
            c.num_paths         = s.value("num_paths",     c.num_paths);
            c.seed              = s.value("seed",          c.seed);
            c.initial_cash      = s.value("initial_cash",      c.initial_cash);
            c.initial_inventory = s.value("initial_inventory", c.initial_inventory);
        }

        if (j.contains("strategy")) {
            const auto& s = j["strategy"];
            c.gamma = s.value("gamma", c.gamma);
            c.k     = s.value("k",     c.k);
            c.A     = s.value("A",     c.A);
        }

        if (j.contains("inventory_constraint")) {
            c.max_inventory = j["inventory_constraint"]
                                .value("max_inventory", c.max_inventory);
        }

        if (j.contains("output")) {
            c.output_dir = j["output"].value("dir", c.output_dir);
        }

        c.validate();
        return c;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Factory: load from a JSON file on disk
    // ─────────────────────────────────────────────────────────────────────────
    [[nodiscard]] static Config from_file(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open())
            throw std::runtime_error("Cannot open config file: " + path);

        nlohmann::json j;
        try {
            f >> j;
        } catch (const nlohmann::json::parse_error& e) {
            throw std::runtime_error(
                "JSON parse error in " + path + ": " + e.what()
            );
        }

        return from_json(j);
    }
};
