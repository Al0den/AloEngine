# Chess game & engine

This is a simple and efficient chess game, completed with a basic engine, coded in pure C, without use of external libraries, and following as best as possible the [first year curriculum](https://cache.media.education.gouv.fr/file/SPE1-MEN-MESRI-4-2-2021/64/6/spe777_annexe_1373646.pdf) of French Classe Preparatoire [MP2I](https://prepas.org/?article=42)

Huge thanks to [BlueFeverSoft](https://www.youtube.com/@BlueFeverSoft) for the underlying basics and code, that I am currently improving and changing to my sauce, the global and widely used chess engine features are implemented thanks to him

## How to use

Whilst not made to be used, and mostly as an individual project to learn fundamentals of game theory, backtracking and pruning, the code should be executable on any machine, running any modern OS, and can be compiled and ran using `gcc src/*.c -O3 -o build/chess.out && .build/chess.out` (Alternatively, a Makefile is provided). 

I will not provide any binaries/executables, not being production/user friendly code and not made for production

If any problem is encountered during compilation, removing `-Ofast` (Compiler optimization) or reducing it should solve the problem

## Current features

- Alpha beta pruning using min-max (negamax) search algorithm
- Move ordering following MVV-LVA (Most Valuable Victim, Less Valuable Attacker)
- Custom position evaluation functions (Pawn structure, board positioning, Queen penalty, Bishop duo)
- Killer moves
- History moves ordering 
- Principal Variation Table
- Basic pawn structure and position evaluation tables
- Hashtable implementation, with biggest depth saving
