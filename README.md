# Chess game & engine

This is a simple and efficient chess game, completed with a basic engine, coded in pure C, without use of external libraries, and following as best as possible the [first year curriculum](https://cache.media.education.gouv.fr/file/SPE1-MEN-MESRI-4-2-2021/64/6/spe777_annexe_1373646.pdf) of French Classe Preparatoire [MP2I](https://prepas.org/?article=42)

## How to build and run

The project now uses CMake for builds. A typical out-of-source build (Release by default):

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/AloEngine
```

Alternatively, you can run it via a CMake custom target:

```
cmake --build build --target run
```

If you prefer a one-off compile, a Makefile is still provided, but CMake is the recommended approach.

### Build Types and Presets

- Default: If not specified, builds use `Release` optimizations.
- Force Release: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build`
- Using presets (recommended):

```
cmake --preset release
cmake --build --preset release -j
```

VS Code users: select the `release` CMake preset to avoid Debug builds.

### Perft target

Build the separate perft driver and run at depth 5:

```
cmake --build build --target perft
./build/perft 5
# or
cmake --build build --target perft-run
```

You can pass a FEN after the depth with the keyword `fen`:

```
./build/perft 4 fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
```

### Elo evaluation utility

From match results (wins/draws/losses), estimate Elo difference and optional absolute Elo against a baseline:

```
cmake --build build --target evaluate_elo
./build/evaluate_elo <wins> <draws> <losses> [--baseline <elo>]

# Example: 55 wins, 20 draws, 25 losses vs a 2000 Elo baseline
./build/evaluate_elo 55 20 25 --baseline 2000
```

This uses the logistic Elo model with a normal approximation for a 95% confidence interval.

### UCI match runner

Run head-to-head matches between two UCI engines and print running Elo after each game:

```
cmake --build build --target uci_match
./build/uci_match --engineA ./build/AloEngine --engineB ./build/AloEngine --games 20 --movetime 100
```

Options:

- `--engineA <path>`: path to first engine (A)
- `--engineB <path>`: path to second engine (B)
- `--games <N>`: number of games (default 10)
- `--movetime <ms>`: per-move time in milliseconds (default 100)
- `--startfen <FEN>`: optional custom start position

## Lichess Bot Integration

You can run this engine on Lichess using the community `lichess-bot` bridge.

Steps:

1) Build the engine:
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

2) Get `lichess-bot`:
- Clone: `git clone https://github.com/ShailChoksi/lichess-bot && cd lichess-bot`
- Or install: `pip install lichess-bot` (check repo for latest instructions)

3) Create a bot account and token on Lichess, grant scopes `bot:play`, `challenge:read`, `challenge:write`.

4) Configure the bot:
- Copy `integrations/lichess-bot.config.example.yml` from this repo into your lichess-bot directory as `config.yml`.
- Edit `config.yml`:
  - Set `token` to your API token
  - Set `engine.command` to the full path of `./build/AloEngine`
  - Set `engine.workingDir` to your project directory (or absolute paths)
  - Optionally adjust `Threads`, `Hash`, and time control preferences

5) Run the bot:
```
python3 lichess-bot.py -c config.yml
# or if installed as a package
python3 -m lichess_bot -c config.yml
```

Notes:
- This engine supports standard chess only. Keep `variant: standard`.
- Some UCI options in the example may be ignored if unrecognized; they are safe to leave.
- Start with unrated games until you are confident in stability.

### Auto-seeking (Matchmaking)

To proactively get games, enable matchmaking in the bot config. Our example config includes a ready-to-use block that posts casual seeks for bullet (1+0), blitz (3+2), and rapid (10+0):

```
matchmaking:
  enabled: true
  max_concurrent_seeks: 1
  min_interval_seconds: 10
  seeks:
    - name: "bullet-1+0-casual"
      rated: false
      variant: standard
      time_control: bullet
      initial: 60
      increment: 0
    - name: "blitz-3+2-casual"
      rated: false
      variant: standard
      time_control: blitz
      initial: 180
      increment: 2
```

Adjust time controls, rated/casual, and concurrency to suit your needs. If your lichess-bot version expects slightly different keys, mirror them from its README, keeping the same intent.

I will not provide any binaries/executables, not being production/user friendly code and not made for production

If any problem is encountered during compilation, removing or reducing optimizations may help. With CMake, you can use `-DCMAKE_BUILD_TYPE=Debug` for an unoptimized build.

## Current features

- Alpha beta pruning using min-max (negamax) search algorithm
- Move ordering following MVV-LVA (Most Valuable Victim, Less Valuable Attacker)
- Custom position evaluation functions (Pawn structure, board positioning, Queen penalty, Bishop duo)
- Killer moves
- History moves ordering 
- Principal Variation Table
- Basic pawn structure and position evaluation tables
- Hashtable implementation, with biggest depth saving
