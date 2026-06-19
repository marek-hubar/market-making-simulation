import polars as pl
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns

if __name__ == "__main__":
    sns.set_theme(style="whitegrid", palette="muted", font_scale=1.1)

    df = pl.read_csv("data/results.csv")

    n_paths = df["path_id"].n_unique()
    print(f"Loaded {df.height:,} rows across {n_paths} paths")

    agg = df.group_by("time").agg([
        pl.col("mid_price").mean().alias("mid_mean"),
        pl.col("mid_price").std().alias("mid_std"),
        pl.col("inventory").mean().alias("inv_mean"),
        pl.col("inventory").std().alias("inv_std"),
        pl.col("pnl").mean().alias("pnl_mean"),
        pl.col("pnl").std().alias("pnl_std"),
    ]).sort("time")

    t = agg["time"].to_numpy()
    mid_mean = agg["mid_mean"].to_numpy()
    mid_std  = agg["mid_std"].to_numpy()
    inv_mean = agg["inv_mean"].to_numpy()
    inv_std  = agg["inv_std"].to_numpy()
    pnl_mean = agg["pnl_mean"].to_numpy()
    pnl_std  = agg["pnl_std"].to_numpy()

    terminal = df.filter(pl.col("time") == pl.col("time").max())
    terminal_pnl = terminal["pnl"].to_numpy()

    z95 = 1.96

    # ── Sharpe ratio ──────────────────────────────────────────────────────
    sharpe = terminal_pnl.mean() / terminal_pnl.std(ddof=1)
    sharpe_se = np.sqrt((1 + 0.5 * sharpe**2) / n_paths)
    sharpe_lo = sharpe - z95 * sharpe_se
    sharpe_hi = sharpe + z95 * sharpe_se

    # ── Terminal PnL statistics ───────────────────────────────────────────
    term_mean = terminal_pnl.mean()
    term_std  = terminal_pnl.std(ddof=1)
    term_ci   = z95 * term_std / np.sqrt(n_paths)
    term_lo   = term_mean - term_ci
    term_hi   = term_mean + term_ci

    print(f"Sharpe ratio : {sharpe:.4f}  (95% CI: [{sharpe_lo:.4f}, {sharpe_hi:.4f}])")
    print(f"Terminal PnL  : {term_mean:.4f} ± {term_ci:.4f}  (95% CI: [{term_lo:.4f}, {term_hi:.4f}])")
    print(f"Terminal PnL std : {term_std:.4f}")

    # ── Plot ──────────────────────────────────────────────────────────────
    se = lambda std: z95 * std / np.sqrt(n_paths)

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))

    c_mid = sns.color_palette()[0]
    c_inv = sns.color_palette()[1]
    c_pnl = sns.color_palette()[2]

    # ── Mid Price ─────────────────────────────────────────────────────
    ax = axes[0, 0]
    # ax.fill_between(t, mid_mean - mid_std, mid_mean + mid_std,
    #                 alpha=0.12, color=c_mid)
    sns.lineplot(x=t, y=mid_mean, linewidth=1.5, color=c_mid, ax=ax)
    ax.set_ylabel("Mid Price")
    # ax.set_ylim([99, 101])
    # ax.set_title("Mid Price (mean ± 1σ)")

    # ── Inventory ─────────────────────────────────────────────────────
    ax = axes[0, 1]
    # ax.fill_between(t, inv_mean - inv_std, inv_mean + inv_std,
    #                 alpha=0.12, color=c_inv)
    sns.lineplot(x=t, y=inv_mean, linewidth=1.5, color=c_inv, ax=ax)
    ax.axhline(y=0, color="gray", linestyle="--", linewidth=0.8, alpha=0.6)
    ax.set_ylabel("Inventory")
    ax.set_title("Inventory (mean)")

    # ── PnL trajectory ────────────────────────────────────────────────
    ax = axes[1, 0]
    ax.fill_between(t, pnl_mean - pnl_std, pnl_mean + pnl_std,
                    alpha=0.10, color=c_pnl, label="±1σ range")
    ax.fill_between(t, pnl_mean - se(pnl_std), pnl_mean + se(pnl_std),
                    alpha=0.35, color=c_pnl, label="95% CI of mean")
    sns.lineplot(x=t, y=pnl_mean, linewidth=1.5, color=c_pnl, ax=ax)
    ax.set_ylabel("PnL")
    ax.set_xlabel("Time")
    ax.set_title("PnL Trajectory")
    ax.legend(fontsize=8, loc="upper left")

    # ── Terminal PnL distribution ─────────────────────────────────────
    ax = axes[1, 1]
    sns.histplot(terminal_pnl, bins=59, stat="density", alpha=0.5,
                 color=sns.color_palette()[3], edgecolor="white", ax=ax)
    sns.kdeplot(terminal_pnl, linewidth=2, color=sns.color_palette()[3], ax=ax)
    ax.axvline(x=0, color="gray", linestyle="--", linewidth=1.0, alpha=0.6)
    ax.axvline(x=term_mean, color="black", linewidth=1.8,
               label=f"mean = {term_mean:.3f}")
    ax.axvline(x=term_lo, color="black", linestyle=":", linewidth=1.2,
               label=f"95% CI [{term_lo:.3f}, {term_hi:.3f}]")
    ax.axvline(x=term_hi, color="black", linestyle=":", linewidth=1.2)
    ax.set_xlabel("Terminal PnL")
    ax.set_ylabel("Density")
    ax.set_title(f"Terminal PnL Distribution (n={n_paths:,}, Sharpe={sharpe:.3f})")
    ax.legend(fontsize=8, loc="upper right")

    for ax in axes.flat:
        ax.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.show()
