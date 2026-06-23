#import "@preview/simple-todo:0.1.0": todo, list-todos
#import "@preview/symbolist:0.1.0": *

#set cite(form: "normal")
#show figure: set block(spacing: 2em)
#set text(font: "New Computer Modern")
#set page(numbering: "1")

#let figure_width = 70%

#show title: set align(center)

#v(1fr)
#title[Testing the Avellaneda-Stoikov Quoting Strategy in a Simulated Market]

// #v(4fr)
#v(2fr)
#outline()
#v(2fr)

= Introduction
Market making is a style of trading where the market maker is always willing to both buy and sell, buy for below the "true" price and sell above it.
Calculating these buy and sell prices is the most important part of the market-maker's job.
The goal of the market maker is to minimize risk stemming from the price movements in the asset it is trading.
Having a position in the stock on either side (short or long) gives us exposure to the stock movement.
As such, the minimization of this position is advantageous.
At its most basic, that is what the Avellaneda-Stoikov strategy does.
It penalizes large positions by aggresivelly quoting close to the mid price in the opposite direction.

In this project we investigate and compare the:
- Avellaneda-Stoikov strategy with a finite time horizon,
- Avellaneda-Stoikov strategy with a fixed time horizon (Linear Skew), and the
- Fixed spreads strategy.

We start by briefly describing the fixed spreads strategy and motivating the need for a more sophisticated approach.
Then we introduce and derive the Avellaneda-Stoikov strategy, showing how all three tested strategies share the same mathematical foundation.
#v(1fr)

#pagebreak()
= Strategy Definitions
To ensure a mathematically rigorous and fair comparison, we derive all three strategies evaluated in this project from the same stochastic optimal control framework. We start with the foundational model, adapt it for perpetual trading, and finally define our naive control group.

== The Mathematical Foundation: Avellaneda-Stoikov Strategy
=== Assumptions
The basic Avellaneda-Stoikov strategy assumes that the price process is a Brownian motion (not geometric) and that the orders are filled with a Poisson distribution whose rates $lambda^b$ (for the bid side) and $lambda^a$ (for the ask side) depend on the distance from the mid price.
In particular, it assumes:
$ dif S = sigma dif W, $
where $sigma > 0$ and $W$ is the standard Brownian motion (Wiener process).
Further, we assume that the function is symmetric for the bid and ask prices and that it is of the form
$ lambda(delta) = lambda^b (delta) = lambda^a (delta) = A e^(-k delta), $
where $A > 0$ and $k > 0$ are paremeters.
Lastly, the market maker is assumed to have an exponential utility function.

=== Derivation
Our strategy's (or agent's) objective is given by:
$ u(s,x,q,t) = max_(delta^a, delta^b) EE_t [-exp(-gamma (X_T + q_T S_T))], $
where
- $X: Omega times [0,T] -> [0,oo)$ is a stochastic process representing the market maker's cash,
- $S: Omega times [0,T] -> RR$ is a stochastic process representing the mid price of the asset,
- $q: Omega times [0,T] -> ZZ$ is a stochastic process representing the market maker's inventory.

In @HoStoll1981 the following #link("https://en.wikipedia.org/wiki/Hamilton%E2%80%93Jacobi%E2%80%93Bellman_equation")[Hamilton-Jacobi-Bellman equation] for $u$ is derived:
#set math.equation(numbering: "(1)")
$
cases(u_t + 1/2 sigma^2 u_(s s) + max_(delta^b) lambda^b (delta^b) (u(s,x-s+delta^b,q+1,t) - u(s,x,q,t)),
wide + max_(delta^a) lambda^a (delta^a) (u(s,x+s+delta^a,q-1,t)-u(s,x,q,t)) = 0,
#v(2em) (u(s,x,q,T) = -exp(-gamma(x + q s)))).
$<eq:pde>
#set math.equation(numbering: none)

Now we define the so-called reservation bid and ask prices $r^b (s,q,t)$ and $r^a (s,q,t)$ as prices that satisfy the following:
$ u(s,x-r^b (s,q,t),q+1,t) &= u(s,x,q,t)\
  u(s,x+r^a (s,q,t),q-1,t) &= u(s,x,q,t), $
i.e. prices that would make us indifferent to either buying or selling one asset.
We call the average $r (s,q,t) = (r^a (s,q,t) + r^b (s,q,t))/2$ of these two values the _reservation price_.

In @AvellanedaStoikov2008 the solution of @eq:pde is derived by applying a Cole-Hopf transformation and substituting an ansatz enabled by our assumption of an exponential utility function. This reduces the PDE to a system of ordinary differential equations.

Solving these yields the first-order continuous approximation for the reservation price:
$ r(s,q,t) = s - q gamma sigma^2 (T - t). $

The optimal distance from the reservation price to the bid and ask quotes is symmetrically given by:
$ delta^a + delta^b = gamma sigma^2 (T - t) + 2/gamma ln(1 + gamma/k). $

== The Industry Adaptation: Linear Skew Strategy
The classic Avellaneda-Stoikov model optimizes for a strict terminal horizon $T$. As $t -> T$, the time-decay factor $(T-t)$ vanishes, causing the agent to aggressively narrow spreads and cross the book to flatten its inventory—a phenomenon known as "closing bell panic."

To adapt this framework for perpetual, steady-state markets (such as continuous electronic exchanges), the Linear Skew strategy generalizes the equations by locking the time-to-horizon to a constant $tau > 0$ (e.g., $tau = 1$). 
This removes the time-decay completely, leaving a constant skew parameter $rho = gamma sigma^2 tau$.
The reservation price simplifies to a linear shift based on inventory $q$:
$ r(s,q) = s - rho q. $
Using a fixed optimal half-spread $delta = 1/gamma ln(1 + gamma/k) + 1/2 gamma sigma^2 tau$, the bid and ask prices $p^b$ and $p^a$ are actively shifted to dispose of inventory:
$ p^b (s, q) = s - delta - rho q\
  p^a (s, q) = s + delta - rho q. $

== The Control Group: Fixed Spread Strategy
To isolate and evaluate the value of dynamic inventory management, we introduce the Fixed Spread strategy as a naive baseline. 

We construct this control group by taking the Avellaneda-Stoikov framework and assuming the agent operates with zero inventory risk ($q = 0$) and a fixed time horizon ($tau = 1$). This ignores the inventory penalty entirely, collapsing the dynamic equations into a static, symmetric half-spread $delta$, anchored precisely to the public mid-price $S$:
$ p^b (s) = s - delta\
  p^a (s) = s + delta, $
where $delta = 1/gamma ln(1 + gamma/k) + 1/2 gamma sigma^2 tau$.

=== Disadvantages of the Fixed Spread Strategy
When the asset's price randomly trends in one direction, this static strategy fails to adjust its quotes.
It blindly accumulates large (either positive or negative) unhedged inventory entirely by chance, exposing the market maker to significant directional market risk when the asset price continues to wander against their position.
This unhedged inventory risk is precisely why the dynamic skewing of the aforementioned strategies is necessary.


#pagebreak()
= Simulation
The simulation consists of two main components: the (mid) prices process simulation, and the order fill simulation.

== Price process
Since the price process is assumed to satisfy $dif S = sigma dif W$, we simulate the increments as
$ Delta S =  sigma Delta W = sigma sqrt(Delta t) Z, $
where $Z tilde cal(N)(0,1).$

== Order fills
The order fills are assumed to follow a Poisson distribution with the rate dependent on the spread.
In particular, we assume that the rate function is the same for bid fills and ask fills and is of the form $lambda (delta) = A e^(-k delta).$
In each time step of length $Delta t$ order fills are given by the a Poisson distribution with the rate $Delta t lambda (delta)$.
For a r.v. $N$ of Poisson distribution with rate $theta$ it holds:
- $PP(N = 0) = 1 - e^(-theta)$,
- $PP(N = 1) = theta e^(-theta)$,
- $PP(N > 1) = 1 - e^(-theta) (1 + theta)$.
Therefore, for a sufficiently small $theta$, we can approximate the Poisson distribution by a Bernoulli distribution with a probability of success $theta$.
In our setting, we need the the time step $Delta t$ to be small enough to make $Delta t lambda(delta) <= Delta t lambda(0) = A Delta t$ roughly less than $0.1$.
We use this approximation in order to simplify the order fill process.
Had we instead filled the orders according to a Poisson distribution, we would potentially simulate multiple fills.
Because the agent only recomputes quotes at discrete time steps, these fills would occur at stale prices, creating an artificial execution latency.
This would violate the zero-latency assumption of the continuous-time model.

#pagebreak()
= Comparison of the three strategies
10,000 sample paths were simulated for each of the 3 strategies. Each of these 3 Monte-Carlo simulations was started with the same seed to utilize the method of Common Random Numbers, effectively pairing the paths.
We start by comparing the PnLs.
#figure(
  image("figures/pnl_violins.svg", width: figure_width),
  caption: [Comparison of the distributions of PnLs for each strategy. The dashed lines represent from the bottom: 2.5%, 25%, 50%, 75%, and 97.5% quantiles of the given violin.]
)<fig:pnl_violin>
From @fig:pnl_violin, we can see that although the fixed spread strategy has the highest mean, it also has by far the highest variance.
This extreme variance is a direct result of unhedged inventory risk.
Because the fixed spread strategy never skews its quotes in response to accumulated inventory, it blindly accumulates unhedged positions by chance, holding onto them and suffering severe drawdowns when the price drifts unfavorably.
This is further illustrated by @fig:risk_reward.
#figure(
  image("figures/risk_vs_reward.svg", width: figure_width),
  caption: [Risk vs Reward scatter plot of the three strategies.]
)<fig:risk_reward>
From @fig:risk_reward we can see that while the Avellaneda-Stoikov and the Linear Skew strategies have very similar variance, the Linear Skew strategy has a significantly higher mean.

Next, we look at the Sharpe ratios of the three strategies.
To compare the strategies we use a paired non-parametric bootstrap test.
That is, we simulate price 10,000 paths for each strategy (with the same starting seed for each strategy), and then for each pair of strategies we use a paired non-parametric bootstrap test for the mean value.

The results of the test are summarized in @tab:bootstrap_tests.
From this table it is clear that the Linear Skew strategy is best in this regard.
This outperformance can be explained by market microstructure mechanics: the classic Avellaneda-Stoikov strategy suffers from "terminal indifference."
As time approaches the terminal horizon $T$, the variance penalty $gamma sigma^2 (T-t)$ decays to zero. Because the agent perceives no future price variance risk, it stops skewing its quotes to manage inventory. In these final moments, it effectively degrades into the Fixed Spread strategy, blindly accumulating unhedged inventory and exposing itself to terminal price variance.
By locking the time horizon $tau$, the Linear Skew strategy avoids this terminal decay and maintains a stable risk profile.
@fig:bootstrap_plots presents the histograms of differences in bootstrapped sharpe ratios.


#figure(
  table(
    columns: (auto, auto, auto, 7em),
    align: (right, center, left, right),
    stroke: (x,y) => (
    top: if y <= 1 {black + 1.0pt},
    bottom:  {black + 0.5pt}
  ),
  table.cell(colspan: 3, [*Alternative Hypothesis*], align: center), [*p-value*],
  [Linear Skew Sharpe ratio], [>], [Avellaneda-Stoikov Sharpe ratio], [0.0001],
  [Linear Skew Sharpe ratio], [>], [Fixed Spread Sharpe ratio], [< 0.0001],
  [Avellaneda-Stoikov Sharpe ratio], [>], [Fixed Spread Sharpe ratio], [< 0.0001],
  ),
  caption: [Results of the paired non-parametric bootstrap tests comparing the mean Sharpe ratios across 10,000 paired sample paths.]
)<tab:bootstrap_tests>

#figure(
  grid(columns: (figure_width/3,) * 3, gutter: 0mm,
    image("figures/sharpe_bootstrap_Avellaneda-Stoikov_vs_Fixed Spread.svg"),
    image("figures/sharpe_bootstrap_Avellaneda-Stoikov_vs_Linear Skew.svg"),
    image("figures/sharpe_bootstrap_Linear Skew_vs_Fixed Spread.svg"),
  ),
  caption: [Bootstrapped histograms of the Sharpe ratio differences.]
)<fig:bootstrap_plots>

Focusing now on the inventory, the mean terminal absolute inventory results of each strategy are summarized in @fig:mean_abs_inv_violins.
From this, it is apparent that the Fixed Spread strategy tends to accumulate very large positions in the asset, exposing itself to severe directional market risk.
In contrast, the Avellaneda-Stoikov and especially the Linear Skew strategy successfully keep their inventory very small, proving that the inventory penalty term effectively mitigates unhedged market risk and safely controls the net open position.

#figure(
  image("figures/mean_abs_inv_violins.svg", width: figure_width),
  caption: [Mean absolute inventory during a single path.]
)<fig:mean_abs_inv_violins>

Further, @fig:mean_abs_inv_ts illustrates the evolution of the mean absolute inventory for each strategy over the course of the simulation.
#figure(
  image("figures/mean_abs_inv_ts.svg", width: figure_width),
  caption: [Mean absolute inventory at each time point.]
)<fig:mean_abs_inv_ts>
From this figure the dynamics of inventory management for each strategy are more aparent.
The Fixed Spread strategy follows a square root curve.
The shape of this curve is connected to the fact that $EE[ |"RW"_n| ] prop sqrt(n)$, where RW is a simple random walk.

On the other hand, we can see that the Avellaneda-Stoikov and the Linear Skew strategies control their inventory more effectively.
In particular, we see that close to the terminal period the Avellaneda-Stoikov strategy becomes increasingly indifferent to holding inventory.
This is the result of the inventory management term in the formula being $gamma sigma^2 (T-t)$, which is small when $t -> T$, and thus does not skew the spread very much.

Conversely, the dynamic skewing of the Avellaneda-Stoikov and Linear Skew strategies transforms their inventory into a mean-reverting process, keeping the expected absolute position strictly bounded.
However, the phenomenon of "terminal indifference" in the classic Avellaneda-Stoikov strategy is distinctly visible.
As time approaches the terminal period ($t -> T$), the inventory penalty term decays toward zero.
Because the agent progressively stops skewing its spread to manage risk, it begins to accumulate inventory at an accelerating rate.

This is not a problem if the assumption of free mid price liquidation holds.
If, however, the the agent's positions need to be liquidated at a loss, the accumulated inventory drags the final PnL down.

= Conclusion

#v(1fr)

#bibliography("bibliography.bib", style: "ieee")
