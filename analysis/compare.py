import polars as pl
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns

def load_and_aggregate(path):
    df = pl.read_csv(path)
    n = df["path_id"].n_unique()

    agg = df.group_by("time").agg([
        pl.col("mid_price").mean().alias("mid_mean"),
        pl.col("mid_price").std().alias("mid_std"),
        pl.col("inventory").mean().alias("inv_mean"),
        pl.col("inventory").std().alias("inv_std"),
        pl.col("pnl").mean().alias("pnl_mean"),
        pl.col("pnl").std().alias("pnl_std"),
    ]).sort("time")

    terminal = df.filter(pl.col("time") == pl.col("time").max())
    terminal_pnl = terminal["pnl"].to_numpy()

    sharpe = terminal_pnl.mean() / terminal_pnl.std(ddof=1)
    sharpe_se = np.sqrt((1 + 0.5 * sharpe**2) / n)
    term_mean = terminal_pnl.mean()
    term_std  = terminal_pnl.std(ddof=1)

    return {
        "agg": agg,
        "n": n,
        "terminal_pnl": terminal_pnl,
        "sharpe": sharpe,
        "sharpe_se": sharpe_se,
        "term_mean": term_mean,
        "term_std": term_std,
    }

if __name__ == "__main__":
    sns.set_theme(style="whitegrid", palette="muted", font_scale=1.1)

    data_as = load_and_aggregate("data/results_AS.csv")
    data_fs = load_and_aggregate("data/results_LS.csv")

    z95 = 1.96

    for label, d in [("Avellaneda-Stoikov", data_as), ("Fixed Spread", data_fs)]:
        ci_lo = d["sharpe"] - z95 * d["sharpe_se"]
        ci_hi = d["sharpe"] + z95 * d["sharpe_se"]
        term_ci = z95 * d["term_std"] / np.sqrt(d["n"])
        print(f"{label}:")
        print(f"  Sharpe  : {d['sharpe']:.4f}  95% CI [{ci_lo:.4f}, {ci_hi:.4f}]")
        print(f"  PnL mean: {d['term_mean']:.4f} ± {term_ci:.4f}  95% CI [{d['term_mean']-term_ci:.4f}, {d['term_mean']+term_ci:.4f}]")
        print(f"  PnL std : {d['term_std']:.4f}")
        print()

    # ── Plot ──────────────────────────────────────────────────────────────
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))

    t = data_as["agg"]["time"].to_numpy()
    se = lambda std, n: z95 * std / np.sqrt(n)

    c_as = sns.color_palette()[0]  # blue
    c_fs = sns.color_palette()[1]  # orange

    # ── Mid Price ─────────────────────────────────────────────────────
    ax = axes[0, 0]
    agg = data_as["agg"]
    mid_mean = agg["mid_mean"].to_numpy()
    mid_ci   = se(agg["mid_std"].to_numpy(), data_as["n"])
    ax.fill_between(t, mid_mean - mid_ci, mid_mean + mid_ci,
                    alpha=0.25, color=c_as)
    sns.lineplot(x=t, y=mid_mean, linewidth=1.5, color=c_as, ax=ax)
    ax.set_ylabel("Mid Price")
    ax.set_title("Mid Price (95% CI of mean)")

    # ── Inventory ─────────────────────────────────────────────────────
    ax = axes[0, 1]
    for d, c, label in [(data_as, c_as, "A-S"), (data_fs, c_fs, "FS")]:
        agg = d["agg"]
        inv_mean = agg["inv_mean"].to_numpy()
        inv_ci   = se(agg["inv_std"].to_numpy(), d["n"])
        ax.fill_between(t, inv_mean - inv_ci, inv_mean + inv_ci,
                        alpha=0.25, color=c)
        sns.lineplot(x=t, y=inv_mean, linewidth=1.5, color=c, label=label, ax=ax)
    ax.axhline(y=0, color="gray", linestyle="--", linewidth=0.8, alpha=0.5)
    ax.set_ylabel("Inventory")
    ax.set_title("Inventory Mean (95% CI of mean)")
    ax.legend(fontsize=9)

    # ── PnL ───────────────────────────────────────────────────────────
    ax = axes[1, 0]
    for d, c, label in [(data_as, c_as, "A-S"), (data_fs, c_fs, "FS")]:
        agg = d["agg"]
        pnl_mean = agg["pnl_mean"].to_numpy()
        pnl_std  = agg["pnl_std"].to_numpy()
        ax.fill_between(t, pnl_mean - se(pnl_std, d["n"]),
                        pnl_mean + se(pnl_std, d["n"]),
                        alpha=0.25, color=c)
        sns.lineplot(x=t, y=pnl_mean, linewidth=1.5, color=c, label=label, ax=ax)
    ax.set_ylabel("PnL")
    ax.set_xlabel("Time")
    ax.set_title("PnL Mean (95% CI of mean)")
    ax.legend(fontsize=9)

    # ── Terminal PnL histogram ─────────────────────────────────────────
    ax = axes[1, 1]
    bins = 60
    for d, c, label in [(data_as, c_as, "A-S"), (data_fs, c_fs, "FS")]:
        sns.histplot(d["terminal_pnl"], bins=bins, stat="density",
                     alpha=0.35, color=c, edgecolor=None, label=f"{label} (n={d['n']})", ax=ax)
        sns.kdeplot(d["terminal_pnl"], linewidth=2, color=c, ax=ax)

    ax.axvline(x=0, color="gray", linestyle="--", linewidth=1.0, alpha=0.5)

    ylim = ax.get_ylim()
    for d, c, label in [(data_as, c_as, "A-S"), (data_fs, c_fs, "FS")]:
        term_ci = z95 * d["term_std"] / np.sqrt(d["n"])
        ax.axvline(x=d["term_mean"], color=c, linewidth=2,
                   linestyle="-", label=f"{label} mean = {d['term_mean']:.3f}")
        ax.axvline(x=d["term_mean"] - term_ci, color=c, linewidth=1.2,
                   linestyle="--")
        ax.axvline(x=d["term_mean"] + term_ci, color=c, linewidth=1.2,
                   linestyle="--")

    ax.set_xlabel("Terminal PnL")
    ax.set_ylabel("Density")
    ax.set_title(f"Terminal PnL Distribution\n"
                 f"A-S Sharpe = {data_as['sharpe']:.3f}  |  "
                 f"FS Sharpe = {data_fs['sharpe']:.3f}")
    ax.legend(fontsize=8, loc="upper right")

    for ax in axes.flat:
        ax.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.show()
