import json
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import PathPatch
import seaborn as sns

from pathlib import Path

def load_results(path: str | Path) -> dict:
    with open(path) as f:
        return json.load(f)


def plot_pnl_violins(data: list[dict]):
    strategies = [d["strategy"] for d in data]
    terminal_pnl = [np.asarray(d["terminal_pnl_values"], dtype=float) for d in data]

    fig, ax = plt.subplots(figsize=(8,4))
    vp = ax.violinplot(
        terminal_pnl,
        showmeans=False,
        showmedians=False,
        showextrema=False,
        widths=0.9,
    )
    palette = sns.color_palette("husl", len(terminal_pnl))
    for body, color in zip(vp["bodies"], palette):
        body.set_facecolor(color)
        body.set_edgecolor("#4a4a4a")
        body.set_linewidth(0.7)
        body.set_alpha(0.9)

    quantiles = np.array([np.percentile(arr, [2.5, 25, 50, 75, 97.5]) for arr in terminal_pnl])
    vline_edges = np.array([np.percentile(arr, [1, 99]) for arr in terminal_pnl])
    xs = np.arange(1, len(terminal_pnl) + 1)
    for x, q, e in zip(xs, quantiles, vline_edges):
        ax.vlines(x, e[0], e[1], color="#3a3a3a", linewidth=0.7, alpha=0.9)
        hlines = ax.hlines([q[0], q[1], q[2], q[3], q[4]], 0, len(terminal_pnl) + 1, color="#3a3a3a", linewidth=0.7, linestyle="dashed")
        patch = PathPatch(ax.collections[x-1].get_paths()[0], transform=ax.transData)
        hlines.set_clip_path(patch) # clip the line by the form of the violin

    ax.set_xticks(xs)
    ax.set_xticklabels(strategies, fontsize=11)
    ax.set_xlabel("Strategy", fontsize=13)
    ax.set_xlim((0.4, len(terminal_pnl) + 0.60))
    ax.set_ylabel("Terminal PnL", fontsize=13)
    ax.set_title("Terminal PnL Distribution by Strategy", fontsize=15)
    ax.set_axisbelow(True)
    ax.grid(axis="y", alpha=0.8, linestyle="dashed")
    plt.tight_layout()

def plot_risk_vs_reward(data: list[dict]):
    strategies = [d["strategy"] for d in data]
    rewards = [d["terminal_pnl"]["mean"] for d in data]
    risks = [d["terminal_pnl"]["stddev"] for d in data] # Potential could change it from var to stddev to make the x and y axis have the same "units"

    fig, ax = plt.subplots(figsize=(8, 4))

    palette = sns.color_palette("husl", len(strategies))
    for i, (x, y, label) in enumerate(zip(risks, rewards, strategies)):
        ax.scatter(x, y, color=palette[i], s=100, label=label)

    ax.set_xlabel("Standard Deviation of PnL", fontsize=13)
    ax.set_ylabel("Mean of PnL", fontsize=13)
    ax.set_title("Risk vs Reward", fontsize=15)

    plt.tight_layout()

def plot_max_abs_inventory_violins(data: list[dict]):
    strategies = [d["strategy"] for d in data]
    terminal_pnl = [np.asarray(d["max_abs_inventory_values"], dtype=float) for d in data]

    fig, ax = plt.subplots(figsize=(8,4))
    vp = ax.violinplot(
        terminal_pnl,
        showmeans=False,
        showmedians=False,
        showextrema=False,
        widths=0.9,
    )
    palette = sns.color_palette("husl", len(terminal_pnl))
    for body, color in zip(vp["bodies"], palette):
        body.set_facecolor(color)
        body.set_edgecolor("#4a4a4a")
        body.set_linewidth(0.7)
        body.set_alpha(0.9)

    quantiles = np.array([np.percentile(arr, [2.5, 25, 50, 75, 97.5]) for arr in terminal_pnl])
    vline_edges = np.array([np.percentile(arr, [1, 99]) for arr in terminal_pnl])
    xs = np.arange(1, len(terminal_pnl) + 1)
    for x, q, e in zip(xs, quantiles, vline_edges):
        ax.vlines(x, e[0], e[1], color="#3a3a3a", linewidth=0.7, alpha=0.9)
        hlines = ax.hlines([q[0], q[1], q[2], q[3], q[4]], 0, len(terminal_pnl) + 1, color="#3a3a3a", linewidth=0.7, linestyle="dashed")
        patch = PathPatch(ax.collections[x-1].get_paths()[0], transform=ax.transData)
        hlines.set_clip_path(patch) # clip the line by the form of the violin

    ax.set_xticks(xs)
    ax.set_yticks(np.arange(0, 31))
    ax.set_yticklabels([(str(i) if i%5 == 0 else "") for i in range(31)])
    ax.set_xticklabels(strategies, fontsize=11)
    ax.set_xlim((0.4, len(terminal_pnl) + 0.60))
    ax.set_xlabel("Strategy", fontsize=13)
    ax.set_ylabel("Max Inventory", fontsize=13)
    ax.set_ylim((0, 30))
    ax.set_title("Max Inventory Distribution by Strategy", fontsize=15)
    ax.set_axisbelow(True)
    ax.grid(axis="y", alpha=0.8, linestyle="dashed")
    plt.tight_layout()

def plot_mean_abs_inventory_violins(data: list[dict]):
    strategies = [d["strategy"] for d in data]
    terminal_pnl = [np.asarray(d["mean_abs_inventory_values"], dtype=float) for d in data]

    fig, ax = plt.subplots(figsize=(8,4))
    vp = ax.violinplot(
        terminal_pnl,
        showmeans=False,
        showmedians=False,
        showextrema=False,
        widths=0.9,
    )
    palette = sns.color_palette("husl", len(terminal_pnl))
    for body, color in zip(vp["bodies"], palette):
        body.set_facecolor(color)
        body.set_edgecolor("#4a4a4a")
        body.set_linewidth(0.7)
        body.set_alpha(0.9)

    quantiles = np.array([np.percentile(arr, [2.5, 25, 50, 75, 97.5]) for arr in terminal_pnl])
    vline_edges = np.array([np.percentile(arr, [1, 99]) for arr in terminal_pnl])
    xs = np.arange(1, len(terminal_pnl) + 1)
    for x, q, e in zip(xs, quantiles, vline_edges):
        ax.vlines(x, e[0], e[1], color="#3a3a3a", linewidth=0.7, alpha=0.9)
        hlines = ax.hlines([q[0], q[1], q[2], q[3], q[4]], 0, len(terminal_pnl) + 1, color="#3a3a3a", linewidth=0.7, linestyle="dashed")
        patch = PathPatch(ax.collections[x-1].get_paths()[0], transform=ax.transData)
        hlines.set_clip_path(patch) # clip the line by the form of the violin

    ax.set_xticks(xs)
    ax.set_yticks(np.arange(0, 19))
    ax.set_yticklabels([(str(i) if i%5 == 0 else "") for i in range(19)])
    ax.set_xticklabels(strategies, fontsize=11)
    ax.set_xlim((0.4, len(terminal_pnl) + 0.60))
    ax.set_xlabel("Strategy", fontsize=13)
    ax.set_ylabel("Mean Inventory", fontsize=13)
    ax.set_ylim((0, 18))
    ax.set_title("Mean Inventory Distribution by Strategy", fontsize=15)
    ax.set_axisbelow(True)
    ax.grid(axis="y", alpha=0.8, linestyle="dashed")
    plt.tight_layout()

def sharpe_ratio(samples: np.ndarray) -> float:
    """Compute Sharpe ratio with guards against overflow/invalid variance math."""
    arr = np.asarray(samples, dtype=np.float64).ravel()
    arr = arr[np.isfinite(arr)]

    if arr.size < 2:
        return 0.0

    mean = float(np.mean(arr))
    centered = arr - mean
    scale = float(np.max(np.abs(centered)))

    if scale == 0.0:
        return 0.0

    std = float(np.sqrt(np.mean((centered / scale) ** 2)) * scale)
    if not np.isfinite(std) or std <= 0.0:
        return 0.0

    sharpe = mean / std
    if not np.isfinite(sharpe):
        return 0.0

    return float(sharpe)

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
        pnl_a = np.asarray(data[ia]["terminal_pnl_values"], dtype=np.float64)
        pnl_b = np.asarray(data[ib]["terminal_pnl_values"], dtype=np.float64)
        pnl_a = pnl_a[np.isfinite(pnl_a)]
        pnl_b = pnl_b[np.isfinite(pnl_b)]

        if pnl_a.size == 0 or pnl_b.size == 0:
            continue

        n_a = pnl_a.size
        n_b = pnl_b.size

        diffs = np.empty(n_boot, dtype=np.float64)
        for k in range(n_boot):
            idx_a = rng.integers(0, n_a, size=n_a)
            idx_b = rng.integers(0, n_b, size=n_b)
            sharpe_a = sharpe_ratio(pnl_a[idx_a])
            sharpe_b = sharpe_ratio(pnl_b[idx_b])
            diffs[k] = sharpe_a - sharpe_b

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

def plot_bootstrap_hists(boot_results: dict | None, output_dir: Path):
    """Histograms of bootstrap Sharpe differences — one figure per pair."""
    if not boot_results:
        return

    for (pair, res) in boot_results.items():
        diffs = res["diffs"]
        p_a_gt = res["p_A_gt_B"]
        p_b_gt = res["p_B_gt_A"]
        dominant = res["name_A"] if p_a_gt > p_b_gt else res["name_B"]
        p_val = min(p_a_gt, p_b_gt)

        fig, ax = plt.subplots(figsize=(4, 5))
        ax.hist(diffs, bins=80, density=True, alpha=0.6, color="steelblue",
                edgecolor="white", linewidth=0.5)
        # ax.axvline(x=0, color="black", linestyle="--", linewidth=1.2)
        ax.set_xlabel(f"Sharpe({res['name_A']})\n - Sharpe({res['name_B']})", fontsize=13)
        ax.set_ylabel("Density", fontsize=13)
        ax.set_title(f"{res['name_A']} vs {res['name_B']}",
                     # f"{dominant} higher  (p = {p_val:.4f}, B = {len(diffs):,})",
                     fontsize=13)

        fig.tight_layout()
        fname = output_dir / f"sharpe_bootstrap_{res['name_A']}_vs_{res['name_B']}.svg"
        fig.savefig(fname, bbox_inches="tight")
        plt.close(fig)

def plot_mean_abs_inventory(data: list[dict]):
    strategies = [d["strategy"] for d in data]
    abs_inventories = [np.asarray(d["abs_inventory_over_time_mean"], dtype=float) for d in data]

    fig, ax = plt.subplots(figsize=(8, 4))
    palette = sns.color_palette("husl", len(strategies))

    xs = np.linspace(0, 1, len(abs_inventories[0]))
    for abs_inventory_ts, label, color in zip(abs_inventories, strategies, palette):
        ax.plot(xs, abs_inventory_ts, color=color, label=label, linewidth=2.0)

    ax.set_xlabel("Time", fontsize=13)
    ax.set_ylabel("Mean inventory", fontsize=13)
    ax.set_title("Mean inventory time series", fontsize=15)
    ax.legend()
    fig.tight_layout()

if __name__ == "__main__":
    data_dir = Path("test")
    output_dir = Path("test")

    strategies = [(data_dir / "results_AS.json", "Avellaneda-Stoikov"),
                  (data_dir / "results_LS.json", "Linear Skew"),
                  (data_dir / "results_FS.json", "Fixed Spreads")]

    data = [load_results(results_file) for results_file, _ in strategies]
    plot_pnl_violins(data)
    plt.savefig(output_dir / "pnl_violins.svg")
    plot_risk_vs_reward(data)
    plt.savefig(output_dir / "risk_vs_reward.svg")
    plot_max_abs_inventory_violins(data)
    plt.savefig(output_dir / "max_abs_inv_violins.svg")
    plot_mean_abs_inventory_violins(data)
    plt.savefig(output_dir / "mean_abs_inv_violins.svg")
    bootstrap_results = bootstrap_sharpe_test(data, 100_000)
    plot_bootstrap_hists(bootstrap_results, output_dir)

    plot_mean_abs_inventory(data)
    plt.savefig(output_dir / "mean_abs_inv_ts.svg")
