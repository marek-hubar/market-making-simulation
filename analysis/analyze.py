#!/usr/bin/env python3
"""
analyze.py — Compare Avellaneda-Stoikov simulation strategies from JSON results.

Usage:
    python analysis/analyze.py data/results_AS.json data/results_FS.json data/results_LS.json

Loads one or more result JSON files (with optional terminal_pnl_values arrays),
prints a side-by-side table, generates bar-chart comparisons, and runs bootstrap
Sharpe-ratio difference tests for all strategy pairs.
"""
import json
import sys
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns


def load_results(path: str) -> dict:
    with open(path) as f:
        return json.load(f)


# ── Table ──────────────────────────────────────────────────────────────────────

METRICS = [
    ("terminal_pnl",       "Terminal PnL"),
    ("sharpe_ratio",       "Sharpe ratio"),
    ("fill_count",         "Fill count"),
    ("max_drawdown",       "Max drawdown"),
    ("mean_abs_inventory", "Mean |inv|"),
    ("mean_spread",        "Mean spread"),
]


def print_table(data: list[dict]):
    """Print side-by-side comparison table of key metrics."""
    n = len(data)
    col_w = 32 if n <= 2 else 20

    header = " " * (col_w + 15)
    for d in data:
        header += f"  {d['strategy']:<{col_w}}"
    print(header)
    print("─" * (col_w + n * (col_w + 2)))

    for key, label in METRICS:
        # Initial label offset 
        line = f"  {label:<{col_w - 2}}"
        
        for d in data:
            if key == "sharpe_ratio":
                v = d[key]
                ci_lo, ci_hi = d["sharpe_ci95"]
                
                # 1. Combine into a single string
                cell_text = f"{v:.4f}  [{ci_lo:.3f}, {ci_hi:.3f}]"
                # 2. Right-align the entire string to the column width
                line += f"  {cell_text:>{col_w}}"
                
            else:
                s = d[key]
                m = s["mean"]
                hw = s["std_error"] * 1.959963984540054  # CI half-width
                
                # 1. Combine into a single string
                cell_text = f"{m:.3f}  ±{hw:.3f}"
                # 2. Right-align the entire string to the column width
                line += f"  {cell_text:>{col_w}}"
                
        print(line)
    print()


# ── Bar charts ──────────────────────────────────────────────────────────────────

def plot_bars(data: list[dict]):
    """2x3 bar chart comparing key metrics across strategies."""
    fig, axes = plt.subplots(2, 3, figsize=(16, 14))
    strategies = [d["strategy"] for d in data]
    n = len(strategies)
    palette = sns.color_palette("muted", n)
    x = np.arange(len(METRICS))

    for i, (key, label) in enumerate(METRICS):
        ax = axes[i // 3, i % 3]
        if key == "sharpe_ratio":
            means = [d[key] for d in data]
            ci_lo = [d["sharpe_ci95"][0] for d in data]
            ci_hi = [d["sharpe_ci95"][1] for d in data]
            errors = [[m - l for m, l in zip(means, ci_lo)],
                      [h - m for m, h in zip(means, ci_hi)]]
        else:
            means = [d[key]["mean"] for d in data]
            std_errors = [d[key]["std_error"] for d in data]
            hw = [1.959963984540054 * se for se in std_errors]
            errors = [hw, hw]  # symmetric

        xs = np.arange(n)
        bars = ax.bar(xs, means, yerr=errors, capsize=8, color=palette,
                      edgecolor="white", linewidth=0.8)
        ax.set_xticks(xs)
        ax.set_xticklabels(strategies, fontsize=9, rotation=15, ha="right")
        ax.set_title(label, fontsize=13, fontweight="bold")
        ax.grid(axis="y", alpha=0.3)

        # Value labels on bars
        for bar, mean in zip(bars, means):
            ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.015 * abs(mean),
                    f"{mean:.3f}", ha="center", va="bottom", fontsize=9)

    # Hide unused subplot if < 6 metrics
    for j in range(len(METRICS), 6):
        axes[j // 3, j % 3].set_visible(False)

    fig.suptitle("Strategy Comparison — Key Metrics (bars = mean, whiskers = 95% CI)",
                 fontsize=15, fontweight="bold")
    plt.tight_layout(rect=[0, 0, 1, 0.96])


# ── Bootstrap Sharpe test ───────────────────────────────────────────────────────

def bootstrap_sharpe_test(data: list[dict], n_boot: int = 10_000):
    """
    Pairwise bootstrap test: is Sharpe(A) > Sharpe(B)?
    For each pair (A, B), resample terminal PnL with replacement 10,000 times,
    compute the Sharpe difference, and estimate the p-value.
    """
    pairs = []
    for i in range(len(data)):
        for j in range(i + 1, len(data)):
            pairs.append((i, j))

    if not pairs:
        return None

    results = {}
    rng = np.random.default_rng(42)

    for ia, ib in pairs:
        pnl_a = np.array(data[ia]["terminal_pnl_values"])
        pnl_b = np.array(data[ib]["terminal_pnl_values"])
        n = len(pnl_a)

        diffs = np.empty(n_boot)
        for k in range(n_boot):
            idx = rng.integers(0, n, size=n)
            sa = np.mean(pnl_a[idx])
            sb = np.mean(pnl_b[idx])
            std_a = np.std(pnl_a[idx], ddof=1)
            std_b = np.std(pnl_b[idx], ddof=1)
            diffs[k] = (sa / std_a if std_a > 0 else 0) - (sb / std_b if std_b > 0 else 0)

        p_a_gt_b = np.mean(diffs > 0)
        p_b_gt_a = np.mean(diffs < 0)

        results[(ia, ib)] = {
            "diffs": diffs,
            "p_A_gt_B": p_a_gt_b,
            "p_B_gt_A": p_b_gt_a,
            "name_A": data[ia]["strategy"],
            "name_B": data[ib]["strategy"],
        }

        name_a = data[ia]["strategy"]
        name_b = data[ib]["strategy"]
        if p_a_gt_b > p_b_gt_a:
            print(f"  {name_a} > {name_b}:  p = {p_b_gt_a:.4f}  (B = {n_boot:,})")
        else:
            print(f"  {name_b} > {name_a}:  p = {p_a_gt_b:.4f}  (B = {n_boot:,})")

    return results


def plot_bootstrap_hists(boot_results: dict):
    """Histograms of bootstrap Sharpe differences for each pair."""
    if not boot_results:
        return

    n = len(boot_results)
    fig, axes = plt.subplots(1, n, figsize=(6 * n, 5))
    if n == 1:
        axes = [axes]

    for ax, (pair, res) in zip(axes, boot_results.items()):
        diffs = res["diffs"]
        p_a_gt = res["p_A_gt_B"]
        p_b_gt = res["p_B_gt_A"]
        dominant = res["name_A"] if p_a_gt > p_b_gt else res["name_B"]
        p_val = min(p_a_gt, p_b_gt)

        ax.hist(diffs, bins=80, density=True, alpha=0.6, color="steelblue",
                edgecolor="white", linewidth=0.5)
        ax.axvline(x=0, color="black", linestyle="--", linewidth=1.2)
        ax.set_xlabel(f"Sharpe({res['name_A']}) - Sharpe({res['name_B']})")
        ax.set_ylabel("Density")
        ax.set_title(f"{res['name_A']} vs {res['name_B']}\n"
                     f"{dominant} higher  (p = {p_val:.4f}, B = {len(diffs):,})",
                     fontsize=11)

        # Annotate quantile of 0
        frac_below = np.mean(diffs < 0)
        ylim = ax.get_ylim()
        ax.text(0.02, 0.92,
                f"P(diff < 0) = {frac_below:.4f}\n"
                f"P(diff > 0) = {1 - frac_below:.4f}",
                transform=ax.transAxes, fontsize=9,
                verticalalignment="top",
                bbox=dict(boxstyle="round", facecolor="wheat", alpha=0.5))

    fig.suptitle("Bootstrap Sharpe Difference Test", fontsize=14, fontweight="bold")
    plt.tight_layout(rect=[0, 0, 1, 0.95])


# ── Main ────────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python analysis/analyze.py <results_1.json> [results_2.json ...]")
        sys.exit(1)

    sns.set_theme(style="whitegrid", palette="muted", font_scale=1.1)

    print("═" * 80)
    print("  Avellaneda-Stoikov Strategy Comparison")
    print("═" * 80)
    print()

    data = [load_results(p) for p in sys.argv[1:]]
    for d in data:
        rf = d["terminal_pnl"]["mean"] / (d["terminal_pnl"]["std_error"] * np.sqrt(d["n_paths"]))
        if rf == 0:
            rf = float("nan")
        print(f"  {d['strategy']:<22s}  n = {d['n_paths']:>5} paths  "
              f"  (loaded from {sys.argv[1:][data.index(d)]})")
    print()

    print_table(data)
    plot_bars(data)

    if len(data) >= 2:
        print("─" * 80)
        print("  Bootstrap Sharpe difference test (B = 10,000)")
        print("─" * 80)
        boot = bootstrap_sharpe_test(data)
        plot_bootstrap_hists(boot)
    else:
        print("(need >= 2 strategies for bootstrap test)")

    plt.show()
