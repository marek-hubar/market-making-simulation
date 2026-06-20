#pragma once
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct FillResult;

// ─────────────────────────────────────────────────────────────────────────────
// OnlineStats — Welford's single-pass mean & sample variance
// ─────────────────────────────────────────────────────────────────────────────
struct OnlineStats {
    double mean = 0.0;
    double m2   = 0.0;   // sum of squared diffs from current mean
    int    count = 0;

    void update(double x) {
        ++count;
        double delta  = x - mean;
        mean         += delta / static_cast<double>(count);
        double delta2 = x - mean;
        m2           += delta * delta2;
    }

    [[nodiscard]] double variance() const {
        return count > 1 ? m2 / static_cast<double>(count - 1) : 0.0;
    }

    [[nodiscard]] double stddev() const {
        return std::sqrt(variance());
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// PathSummary — per-path statistics returned after one Monte Carlo realisation
// ─────────────────────────────────────────────────────────────────────────────
struct PathSummary {
    double terminal_pnl         = 0.0;
    double terminal_cash        = 0.0;
    int    terminal_inventory   = 0;
    double terminal_midprice    = 0.0;

    double mean_inventory       = 0.0;
    double mean_abs_inventory   = 0.0;
    double inventory_variance   = 0.0;
    int    max_abs_inventory    = 0;

    int    buy_fill_count       = 0;
    int    sell_fill_count      = 0;
    int    fill_count() const   { return buy_fill_count + sell_fill_count; }

    double mean_spread          = 0.0;
    double spread_variance      = 0.0;

    double pnl_variance         = 0.0;
    double pnl_increment_variance = 0.0;

    double max_drawdown         = 0.0;
};

// ─────────────────────────────────────────────────────────────────────────────
// StatisticsCollector — step-by-step accumulator for a single path
// ─────────────────────────────────────────────────────────────────────────────
class StatisticsCollector {
private:
    OnlineStats inv_stats_;
    OnlineStats abs_inv_stats_;
    OnlineStats spread_stats_;
    OnlineStats pnl_level_stats_;
    OnlineStats pnl_increment_stats_;

    int buy_fill_count_  = 0;
    int sell_fill_count_ = 0;

    int    max_abs_inventory_ = 0;
    double running_max_pnl_    = 0.0;
    double max_drawdown_       = 0.0;

    double prev_pnl_       = 0.0;
    bool   has_prev_pnl_   = false;

    bool   first_call_     = true;

public:
    void record_step(double /*time*/, double /*mid*/, int inv, double pnl,
                     double total_spread, const FillResult& fills) {
        inv_stats_.update(static_cast<double>(inv));
        abs_inv_stats_.update(static_cast<double>(std::abs(inv)));
        spread_stats_.update(total_spread);
        pnl_level_stats_.update(pnl);

        if (has_prev_pnl_) {
            pnl_increment_stats_.update(pnl - prev_pnl_);
        }
        prev_pnl_     = pnl;
        has_prev_pnl_ = true;

        if (fills.bid_filled) ++buy_fill_count_;
        if (fills.ask_filled) ++sell_fill_count_;

        int abs_inv = std::abs(inv);
        if (abs_inv > max_abs_inventory_) max_abs_inventory_ = abs_inv;

        if (first_call_) {
            running_max_pnl_ = pnl;
            first_call_      = false;
        }
        if (pnl > running_max_pnl_) running_max_pnl_ = pnl;
        double dd = running_max_pnl_ - pnl;
        if (dd > max_drawdown_) max_drawdown_ = dd;
    }

    [[nodiscard]] PathSummary finalize(int terminal_inv, double terminal_mid,
                                       double terminal_cash,
                                       double terminal_pnl) const {
        PathSummary ps;
        ps.terminal_pnl           = terminal_pnl;
        ps.terminal_cash          = terminal_cash;
        ps.terminal_inventory     = terminal_inv;
        ps.terminal_midprice      = terminal_mid;

        ps.mean_inventory         = inv_stats_.mean;
        ps.mean_abs_inventory     = abs_inv_stats_.mean;
        ps.inventory_variance     = inv_stats_.variance();
        ps.max_abs_inventory      = max_abs_inventory_;

        ps.buy_fill_count         = buy_fill_count_;
        ps.sell_fill_count        = sell_fill_count_;

        ps.mean_spread            = spread_stats_.mean;
        ps.spread_variance        = spread_stats_.variance();

        ps.pnl_variance           = pnl_level_stats_.variance();
        ps.pnl_increment_variance = pnl_increment_stats_.variance();

        ps.max_drawdown           = max_drawdown_;
        return ps;
    }

    void reset() {
        inv_stats_             = {};
        abs_inv_stats_         = {};
        spread_stats_          = {};
        pnl_level_stats_       = {};
        pnl_increment_stats_   = {};
        buy_fill_count_        = 0;
        sell_fill_count_       = 0;
        max_abs_inventory_     = 0;
        running_max_pnl_       = 0.0;
        max_drawdown_          = 0.0;
        has_prev_pnl_          = false;
        first_call_            = true;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// SummaryStats — batch statistics from a vector of values
// ─────────────────────────────────────────────────────────────────────────────
struct SummaryStats {
    double mean      = 0.0;
    double var       = 0.0;
    double stddev    = 0.0;
    double std_error = 0.0;
    int    count     = 0;
    double ci95_lower = 0.0;
    double ci95_upper = 0.0;

    static SummaryStats from(const std::vector<double>& values) {
        SummaryStats s;
        s.count = static_cast<int>(values.size());
        if (s.count == 0) return s;

        double sum = 0.0;
        for (double v : values) sum += v;
        s.mean = sum / static_cast<double>(s.count);

        double m2 = 0.0;
        for (double v : values) {
            double d = v - s.mean;
            m2 += d * d;
        }
        s.var       = s.count > 1 ? m2 / static_cast<double>(s.count - 1) : 0.0;
        s.stddev    = std::sqrt(s.var);
        s.std_error = s.stddev / std::sqrt(static_cast<double>(s.count));

        constexpr double z95 = 1.959963984540054;
        s.ci95_lower = s.mean - z95 * s.std_error;
        s.ci95_upper = s.mean + z95 * s.std_error;
        return s;
    }

    static SummaryStats from_counts(const std::vector<int>& values) {
        std::vector<double> dv;
        dv.reserve(values.size());
        for (int v : values) dv.push_back(static_cast<double>(v));
        return from(dv);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// CrossPathStats — aggregates across all Monte Carlo paths
// ─────────────────────────────────────────────────────────────────────────────
struct CrossPathStats {
    SummaryStats terminal_pnl;
    SummaryStats terminal_cash;
    SummaryStats terminal_inventory;
    SummaryStats terminal_midprice;
    SummaryStats mean_inventory;
    SummaryStats mean_abs_inventory;
    SummaryStats inventory_variance;
    SummaryStats max_abs_inventory;
    SummaryStats fill_count;
    SummaryStats buy_fill_count;
    SummaryStats sell_fill_count;
    SummaryStats mean_spread;
    SummaryStats spread_variance;
    SummaryStats pnl_variance;
    SummaryStats pnl_increment_variance;
    SummaryStats max_drawdown;

    double sharpe_ratio       = 0.0;
    double sharpe_se          = 0.0;   // Lo (2002) standard error
    double sharpe_ci95_lower  = 0.0;
    double sharpe_ci95_upper  = 0.0;
};

// ─────────────────────────────────────────────────────────────────────────────
// aggregate_paths — cross-path aggregation
// ─────────────────────────────────────────────────────────────────────────────
inline CrossPathStats aggregate_paths(const std::vector<PathSummary>& paths) {
    CrossPathStats cps;

    auto extract_d = [&](auto member) {
        std::vector<double> v;
        v.reserve(paths.size());
        for (const auto& p : paths) v.push_back(static_cast<double>(p.*member));
        return SummaryStats::from(v);
    };

    auto extract_i = [&](auto member) {
        std::vector<int> v;
        v.reserve(paths.size());
        for (const auto& p : paths) v.push_back(p.*member);
        return SummaryStats::from_counts(v);
    };

    cps.terminal_pnl            = extract_d(&PathSummary::terminal_pnl);
    cps.terminal_cash           = extract_d(&PathSummary::terminal_cash);
    cps.terminal_inventory      = extract_i(&PathSummary::terminal_inventory);
    cps.terminal_midprice       = extract_d(&PathSummary::terminal_midprice);
    cps.mean_inventory          = extract_d(&PathSummary::mean_inventory);
    cps.mean_abs_inventory      = extract_d(&PathSummary::mean_abs_inventory);
    cps.inventory_variance      = extract_d(&PathSummary::inventory_variance);
    cps.max_abs_inventory       = extract_i(&PathSummary::max_abs_inventory);

    {
        std::vector<int> fills;
        fills.reserve(paths.size());
        for (const auto& p : paths) fills.push_back(p.fill_count());
        cps.fill_count = SummaryStats::from_counts(fills);
    }
    cps.buy_fill_count          = extract_i(&PathSummary::buy_fill_count);
    cps.sell_fill_count         = extract_i(&PathSummary::sell_fill_count);

    cps.mean_spread             = extract_d(&PathSummary::mean_spread);
    cps.spread_variance         = extract_d(&PathSummary::spread_variance);
    cps.pnl_variance            = extract_d(&PathSummary::pnl_variance);
    cps.pnl_increment_variance  = extract_d(&PathSummary::pnl_increment_variance);
    cps.max_drawdown            = extract_d(&PathSummary::max_drawdown);

    // Sharpe ratio from terminal PnL across paths
    const auto& tp = cps.terminal_pnl;
    cps.sharpe_ratio = tp.stddev > 0.0 ? tp.mean / tp.stddev : 0.0;
    double n = static_cast<double>(tp.count);
    cps.sharpe_se = n > 0.0
        ? std::sqrt((1.0 + 0.5 * cps.sharpe_ratio * cps.sharpe_ratio) / n)
        : 0.0;
    constexpr double z95 = 1.959963984540054;
    cps.sharpe_ci95_lower = cps.sharpe_ratio - z95 * cps.sharpe_se;
    cps.sharpe_ci95_upper = cps.sharpe_ratio + z95 * cps.sharpe_se;

    return cps;
}

// ─────────────────────────────────────────────────────────────────────────────
// write_stats_json — compact JSON summary file
// ─────────────────────────────────────────────────────────────────────────────
inline void write_stats_json(const std::string& filepath,
                             const CrossPathStats& s,
                             const std::string& strategy_name,
                             const std::vector<double>& terminal_pnl_values = {}) {
    std::ofstream f(filepath);
    if (!f.is_open()) return;

    auto w = [&](const char* name, const SummaryStats& st, bool last = false) {
        f << "    \"" << name << "\": {\n"
          << "      \"mean\": "      << st.mean      << ",\n"
          << "      \"var\": "       << st.var       << ",\n"
          << "      \"stddev\": "    << st.stddev    << ",\n"
          << "      \"std_error\": " << st.std_error << ",\n"
          << "      \"count\": "     << st.count     << ",\n"
          << "      \"ci95\": ["     << st.ci95_lower << ", " << st.ci95_upper << "]\n"
          << "    }" << (last ? "" : ",") << "\n";
    };

    f << std::fixed << std::setprecision(6);
    f << "{\n"
      << "  \"strategy\": \"" << strategy_name << "\",\n"
      << "  \"n_paths\": " << s.terminal_pnl.count << ",\n\n";

    w("terminal_pnl",            s.terminal_pnl);
    w("terminal_cash",           s.terminal_cash);
    w("terminal_inventory",      s.terminal_inventory);
    w("terminal_midprice",       s.terminal_midprice);
    w("mean_inventory",          s.mean_inventory);
    w("mean_abs_inventory",      s.mean_abs_inventory);
    w("inventory_variance",      s.inventory_variance);
    w("max_abs_inventory",       s.max_abs_inventory);
    w("fill_count",              s.fill_count);
    w("buy_fill_count",          s.buy_fill_count);
    w("sell_fill_count",         s.sell_fill_count);
    w("mean_spread",             s.mean_spread);
    w("spread_variance",         s.spread_variance);
    w("pnl_variance",            s.pnl_variance);
    w("pnl_increment_variance",  s.pnl_increment_variance);
    w("max_drawdown",            s.max_drawdown);

    f << "  \"sharpe_ratio\": "      << s.sharpe_ratio      << ",\n"
      << "  \"sharpe_se\": "          << s.sharpe_se          << ",\n"
      << "  \"sharpe_ci95\": ["      << s.sharpe_ci95_lower
                                    << ", " << s.sharpe_ci95_upper << "]";

    if (!terminal_pnl_values.empty()) {
        f << ",\n";
        f << "  \"terminal_pnl_values\": [";
        for (std::size_t i = 0; i < terminal_pnl_values.size(); ++i) {
            if (i > 0) f << ", ";
            f << terminal_pnl_values[i];
        }
        f << "]";
    }

    f << "\n}\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// print_stats_summary — console table for one strategy
// ─────────────────────────────────────────────────────────────────────────────
inline void print_stats_summary(const CrossPathStats& s,
                                const std::string& strategy_name) {
    std::cout << "────────────────────────────────────────────────────────\n";
    std::cout << "  " << strategy_name << "  (n = " << s.terminal_pnl.count << " paths)\n";
    std::cout << "────────────────────────────────────────────────────────\n";

    std::cout << std::fixed << std::setprecision(4);

    auto row = [](const char* label, const SummaryStats& st) {
        std::cout << "  " << std::left  << std::setw(28) << label
                  << "  mean="  << std::setw(10) << st.mean
                  << "  std="   << std::setw(10) << st.stddev
                  << "  SE="    << std::setw(10) << st.std_error << "\n";
    };

    row("terminal_pnl",            s.terminal_pnl);
    row("terminal_cash",           s.terminal_cash);
    row("terminal_inventory",      s.terminal_inventory);
    row("mean_inventory",          s.mean_inventory);
    row("mean_abs_inventory",      s.mean_abs_inventory);
    row("inventory_variance",      s.inventory_variance);
    row("max_abs_inventory",       s.max_abs_inventory);
    row("fill_count",              s.fill_count);
    row("buy_fill_count",          s.buy_fill_count);
    row("sell_fill_count",         s.sell_fill_count);
    row("mean_spread",             s.mean_spread);
    row("spread_variance",         s.spread_variance);
    row("pnl_variance",            s.pnl_variance);
    row("pnl_increment_variance",  s.pnl_increment_variance);
    row("max_drawdown",            s.max_drawdown);

    std::cout << "\n";
    std::cout << "  Sharpe ratio            : " << s.sharpe_ratio << "\n";
    std::cout << "  Sharpe SE                : " << s.sharpe_se    << "\n";
    std::cout << "  Sharpe 95% CI            : ["
              << s.sharpe_ci95_lower << ", " << s.sharpe_ci95_upper << "]\n";
    std::cout << "\n";
}
