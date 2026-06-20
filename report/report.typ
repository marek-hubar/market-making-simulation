#import "@preview/simple-todo:0.1.0": todo, list-todos
#import "@preview/symbolist:0.1.0": *

#set cite(form: "normal")

#show title: set align(center)
#title[Testing the Avellaneda-Stoikov Quoting\ Strategy in a Simulated Market]


= Introduction
Market making is a style of trading where the market maker is always willing to both buy and sell, buy for below the "true" price and sell above it.
Calculating these buy and sell prices is the most important part of the market-maker's job.
The goal of the market maker is to minimize risk stemming from the price movements in the asset it is trading.
Having a position in the stock on either side (short or long) gives us exposure to the stock movement.
As such, the minimization of this position is advantageous.
At its most basic, that is what the Avellaneda-Stoikov strategy does.
It penalizes large positions by aggresivelly quoting close to the mid price in the opposite direction.

In this project we look investigate and compare the 
+ Avellaneda-Stoikov strategy with finite time horizon,
+ Avellaneda-Stoikov strategy with an infinite time horizon, and the
+ fixed-spreads strategy.

We start by briefly describing the fixed spreads strategy and motivating the need for a more sophisticated approach.
Then we introduce and derive the Avellaneda-Stoikov strategy in both its variants.

= Fixed spreads strategy
The "true" price is defined as the arithemetic average of the lowest ask (the price someone is asking for the asset) and the highest bid (the price someone is willing to buy the asset at).
From here on we refer to this price as the _mid price_ and denote it by $S$.

The simplest possible strategy is to choose a fixed number $delta > 0$ and always quote our bid at $S - delta$ and our ask at $S + delta$.
The choice of an optimal $delta$ can be made by running our strategy with different values of $delta$ on historical or simulated data and picking the $delta$ that maximizes our profit.
In our case, in order for the comparison between the strategies to be fair, we are going to choose $delta$ to be an explicitly calculated value related to Avellaneda-Stoikov strategy.

== Disadvantages of the fixed preads strategy
When the asset's price is not just hovering around the same value but is trending in one direction, this strategy will accumulate large (either positive or negative) inventory.
This exposes the market maker to risk, as he no longer just makes money on the spread of each transaction, but also makes or loses money based on the movement of the asset's price.
This is precisely the reason market makers try to actively minimize their position on either side #todo[of the trade (probably needs a different phrase)].


= Avellaneda-Stoikov strategy
The Avellaneda-Stoikov strategy solves this issue by quoting with a smaller spread on the side where the market maker has a large inventory and with a larger spread on the opposite side.
That is, if the market maker is long 5 units of a stock, his ask price is going to be closer to the mid price, while his bid price is going to be further.
This way, he actively gets rid of his inventory.


== Assumptions
The basic Avellaneda-Stoikov strategy assumes that the price process is a Brownian motion (not geometric) and that the orders are filled with a Poisson distribution whose rates $lambda^b$ (for the bid side) and $lambda^a$ (for the ask side) depend on the distance from the mid price.
In particular, it assumes:
$ dif S = sigma dif W, $
where $sigma > 0$ and $W$ is the standard Brownian motion (Wiener process).
Further, we assume that the function is symmetric for the bid and ask prices and that it is of the form
$ lambda(delta) = lambda^b (delta) = lambda^a (delta) = A e^(-k delta), $
where $A > 0$ and $k > 0$ are paremeters.
Lastly, the market maker is assumed to have an exponential utility function.

== Derivation
Our strategy's (or agent's) objective is given by:
$ u(s,x,q,t) = max_(delta^a, delta^b) EE_t [-exp(-gamma (X_T + q_T S_T))], $
where
- $X: Omega times [0,T] -> [0,oo)$ is a stochastic process representing the market maker's cash,
- $S: Omega times [0,T] -> RR$ is a stochastic process representing the mid price of the asset,
- $q: Omega times [0,T] -> ZZ$ is a stochastic process representing the market maker's inventory.

In @HoStoll1981 the following #link("https://en.wikipedia.org/wiki/Hamilton%E2%80%93Jacobi%E2%80%93Bellman_equation")[Hamilton-Jacobi-Bellman equation] for $u$ is derived:
$
  cases(u_t + 1/2 sigma^2 u_(s s) + max_(delta^b) lambda^b (delta^b) (u(s,x-s+delta^b,q+1,t) - u(s,x,q,t)),
        wide + max_(delta^a) lambda^a (delta^a) (u(s,x+s+delta^a,q-1,t)-u(s,x,q,t)) = 0,
        #v(2em) (u(s,x,q,T) = -exp(-gamma(x + q s)))).
$
In @AvellanedaStoikov2008 the solution of this equation is derived by first solving this PDE with the use of an ansatz enabled by our assumption of an exponential utility function.
#todo[Finish deriving the reservation price and the spreads $delta^a$ and $delta^b$.]

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

= Comparison with a fixed-spreads strategy



#pagebreak()
#def-symbol($S$, "mid price", unit: $dollar$)
#def-symbol($W$, "Brownian motion (Wiener process)", unit: "dimensionless")
#def-symbol($q$, "Current inventory", unit: "pcs")
#print-symbols()

#pagebreak()
#bibliography("bibliography.bib", style: "ieee")
