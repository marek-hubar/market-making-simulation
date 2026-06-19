# Avellaneda–Stoikov Market-Making Simulator

A C++20 / Python research project implementing the Avellaneda–Stoikov (2008) optimal
market-making model, benchmarked against fixed-spread and inventory-constrained
strategies using Monte Carlo simulation.

## Prerequisites

| Tool | Minimum version |
|------|----------------|
| CMake | 3.22 |
| GCC / Clang / MSVC | C++20 support (GCC ≥ 11, Clang ≥ 14, MSVC 19.28+) |
| Python | 3.10 (analysis scripts, Phase 5+) |
| Git | any recent version |

Dependencies (nlohmann/json, Catch2) are fetched automatically by CMake at
configure time — no manual installation required.

## Build

```bash
# Configure (downloads dependencies on first run, ~30 s)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Compile
cmake --build build -j$(nproc)   # Linux / macOS
cmake --build build              # Windows
```

## Run

```bash
./build/simulator                          # uses config/default.json
./build/simulator config/default.json     # explicit path
```

## Test

```bash
cd build && ctest --output-on-failure
# or run the test binary directly for verbose output:
./build/tests/tests -v
```

## Project structure

```
avellaneda-stoikov/
├── CMakeLists.txt          # root build; fetches nlohmann/json + Catch2
├── config/
│   ├── default.json        # baseline parameters
│   └── experiments/        # per-experiment overrides (Phase 6)
├── src/
│   ├── main.cpp
│   └── core/
│       ├── Config.hpp           # ← Phase 1 (this phase)
│       ├── PriceProcess.hpp/.cpp    # Phase 2
│       ├── OrderArrivalModel.hpp/.cpp
│       ├── Portfolio.hpp
│       └── Simulator.hpp/.cpp
│   ├── strategies/              # Phase 3
│   │   ├── Strategy.hpp         # abstract base
│   │   ├── AvellanedaStoikov.hpp/.cpp
│   │   ├── FixedSpread.hpp/.cpp
│   │   └── InventoryConstrained.hpp/.cpp
│   └── utils/                   # Phase 4
│       ├── Random.hpp
│       ├── CSVWriter.hpp/.cpp
│       └── Statistics.hpp
├── tests/
│   ├── CMakeLists.txt
│   ├── test_config.cpp          # ← Phase 1
│   ├── test_price_process.cpp   # Phase 2
│   ├── test_strategy.cpp        # Phase 3
│   └── test_simulator.cpp       # Phase 4
├── python/                      # Phase 5
│   ├── requirements.txt
│   ├── analysis/
│   └── experiments/
└── data/output/                 # generated, gitignored
```

## Key formulas (Avellaneda & Stoikov 2008)

```
Price process        dS  = σ dW                    (arithmetic BM, NOT log-normal)
Reservation price    r   = S − q·γ·σ²·(T−t)
Optimal half-spread  δ*  = (1/γ)·ln(1 + γ/k) + γ·σ²·(T−t)/2
Bid quote            p_b = r − δ*
Ask quote            p_a = r + δ*
Fill intensity       λ(δ) = A·exp(−k·δ)
Fill probability/dt  P   = λ(δ)·dt                (Bernoulli per tick)
Mark-to-market PnL   PnL = cash + inventory·S
```

## References

Avellaneda, M., & Stoikov, S. (2008). *High-frequency trading in a limit order book.*
Quantitative Finance, 8(3), 217–224.
