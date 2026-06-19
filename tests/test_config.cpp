#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "core/Config.hpp"

using Catch::Matchers::WithinAbs;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

// Build a minimal valid JSON object so tests don't have to repeat themselves.
static nlohmann::json make_json(
    double sigma = 0.01, double gamma = 0.1, double k = 1.5,
    double A = 1.0,      double T = 1.0,     double dt = 0.005)
{
    return {
        {"simulation", {{"sigma", sigma}, {"T", T}, {"dt", dt}}},
        {"strategy",   {{"gamma", gamma}, {"k", k}, {"A", A}}}
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// Default construction
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("Default Config has positive parameters", "[config][defaults]") {
    Config c;
    REQUIRE(c.sigma         > 0.0);
    REQUIRE(c.gamma         > 0.0);
    REQUIRE(c.k             > 0.0);
    REQUIRE(c.A             > 0.0);
    REQUIRE(c.T             > 0.0);
    REQUIRE(c.dt            > 0.0);
    REQUIRE(c.initial_price > 0.0);
    REQUIRE(c.num_paths     > 0);
    REQUIRE(c.max_inventory > 0);
}

TEST_CASE("Default Config passes validation", "[config][defaults]") {
    REQUIRE_NOTHROW(Config{}.validate());
}

TEST_CASE("Default steps() is consistent with T and dt", "[config][defaults]") {
    Config c;
    // steps = round(T / dt) = round(1.0 / 0.005) = 200
    REQUIRE(c.steps() == 200);
}

// ─────────────────────────────────────────────────────────────────────────────
// JSON loading
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("Config loads sigma from JSON", "[config][json]") {
    auto cfg = Config::from_json(make_json(/*sigma=*/0.02));
    REQUIRE_THAT(cfg.sigma, WithinAbs(0.02, 1e-12));
}

TEST_CASE("Config loads all strategy params from JSON", "[config][json]") {
    auto j = make_json(0.01, /*gamma=*/0.5, /*k=*/2.0, /*A=*/0.8);
    auto cfg = Config::from_json(j);

    REQUIRE_THAT(cfg.gamma, WithinAbs(0.5, 1e-12));
    REQUIRE_THAT(cfg.k,     WithinAbs(2.0, 1e-12));
    REQUIRE_THAT(cfg.A,     WithinAbs(0.8, 1e-12));
}

TEST_CASE("Config loads simulation timing from JSON", "[config][json]") {
    auto j = nlohmann::json{
        {"simulation", {{"T", 2.0}, {"dt", 0.01}, {"sigma", 0.01}}},
        {"strategy",   {{"gamma", 0.1}, {"k", 1.5}, {"A", 1.0}}}
    };
    auto cfg = Config::from_json(j);

    REQUIRE_THAT(cfg.T,  WithinAbs(2.0,  1e-12));
    REQUIRE_THAT(cfg.dt, WithinAbs(0.01, 1e-12));
    REQUIRE(cfg.steps() == 200);   // round(2.0 / 0.01) = 200
}

TEST_CASE("Config preserves defaults for fields absent from JSON", "[config][json]") {
    // Only override sigma; everything else should stay at defaults.
    nlohmann::json j = {
        {"simulation", {{"sigma", 0.05}}}
    };

    // This will fail validation because A*dt might be < 1, but sigma=0.05
    // and defaults for A=1.0, dt=0.005 => A*dt=0.005 which is fine.
    // gamma=0.0 is NOT the default issue here, gamma default is 0.1.
    // Actually since strategy is missing, we get the Config defaults.
    auto cfg = Config::from_json(j);

    REQUIRE_THAT(cfg.sigma, WithinAbs(0.05, 1e-12));
    REQUIRE_THAT(cfg.gamma, WithinAbs(0.1,  1e-12));  // default
    REQUIRE_THAT(cfg.k,     WithinAbs(1.5,  1e-12));  // default
    REQUIRE_THAT(cfg.A,     WithinAbs(1.0,  1e-12));  // default
}

TEST_CASE("Config loads output dir from JSON", "[config][json]") {
    nlohmann::json j = make_json();
    j["output"] = {{"dir", "data/output/experiment_01"}};

    auto cfg = Config::from_json(j);
    REQUIRE(cfg.output_dir == "data/output/experiment_01");
}

TEST_CASE("Config loads inventory constraint from JSON", "[config][json]") {
    nlohmann::json j = make_json();
    j["inventory_constraint"] = {{"max_inventory", 25}};

    auto cfg = Config::from_json(j);
    REQUIRE(cfg.max_inventory == 25);
}

TEST_CASE("Config loads Monte Carlo params from JSON", "[config][json]") {
    nlohmann::json j = {
        {"simulation", {
            {"sigma", 0.01}, {"T", 1.0}, {"dt", 0.005},
            {"num_paths", 500}, {"seed", 999}
        }},
        {"strategy", {{"gamma", 0.1}, {"k", 1.5}, {"A", 1.0}}}
    };
    auto cfg = Config::from_json(j);

    REQUIRE(cfg.num_paths == 500);
    REQUIRE(cfg.seed      == 999UL);
}

// ─────────────────────────────────────────────────────────────────────────────
// Derived quantities
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("steps() rounds correctly to avoid floating-point drift", "[config][derived]") {
    // T/dt should yield exact integer counts even with fp arithmetic.
    struct Case { double T; double dt; int expected; };
    const Case cases[] = {
        {1.0,    0.005,  200},
        {1.0,    0.01,   100},
        {0.5,    0.005,  100},
        {2.0,    0.002, 1000},
        // dt=0.5 keeps A*dt=0.5 < 1, satisfying the Bernoulli constraint.
        {300.0,  0.5,    600},
    };
    for (const auto& c : cases) {
        nlohmann::json j = {
            {"simulation", {{"T", c.T}, {"dt", c.dt}, {"sigma", 0.01}}},
            {"strategy",   {{"gamma", 0.1}, {"k", 1.5}, {"A", 1.0}}}
        };
        auto cfg = Config::from_json(j);
        REQUIRE(cfg.steps() == c.expected);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Validation: expected failures
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("validate() rejects non-positive sigma", "[config][validation]") {
    Config c;
    c.sigma = 0.0;
    REQUIRE_THROWS_AS(c.validate(), std::invalid_argument);
}

TEST_CASE("validate() rejects non-positive gamma", "[config][validation]") {
    Config c;
    c.gamma = -0.1;
    REQUIRE_THROWS_AS(c.validate(), std::invalid_argument);
}

TEST_CASE("validate() rejects dt >= T", "[config][validation]") {
    Config c;
    c.dt = c.T;
    REQUIRE_THROWS_AS(c.validate(), std::invalid_argument);
}

TEST_CASE("validate() rejects A * dt >= 1 (fill prob overflow)", "[config][validation]") {
    // A=200, dt=0.01 => A*dt = 2.0 >= 1 → invalid Bernoulli model
    Config c;
    c.A  = 200.0;
    c.dt = 0.01;
    REQUIRE_THROWS_AS(c.validate(), std::invalid_argument);
}

TEST_CASE("from_json() throws on malformed JSON key types", "[config][validation]") {
    // sigma given as a string instead of a number
    nlohmann::json j = {
        {"simulation", {{"sigma", "bad"}, {"T", 1.0}, {"dt", 0.005}}},
        {"strategy",   {{"gamma", 0.1},  {"k", 1.5}, {"A", 1.0}}}
    };
    REQUIRE_THROWS(Config::from_json(j));
}

TEST_CASE("from_file() throws on missing file", "[config][io]") {
    REQUIRE_THROWS_AS(
        Config::from_file("nonexistent_file_xyz.json"),
        std::runtime_error
    );
}
