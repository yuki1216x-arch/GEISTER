# GEISTER

C++ tools for constructing, querying, and validating belief-state endgame databases for **Geister**, an imperfect-information board game. The repository also contains a database constructor for the perfect-information **Purple-piece Geister** variant.

The implementation uses a zero-suppressed decision diagram (ZDD) to map board configurations to integer IDs. Retrograde analysis assigns outcome labels to those IDs, and the validation program checks the ZDD mapping and the successor conditions of assigned labels.

## Requirements

- Linux or a compatible Unix-like environment.
- GNU Make and `g++` with C++14 support.
- POSIX thread support (`-pthread`).
- Bash for the batch scripts.

The implementation uses GNU extensions, including variable-length arrays. The supplied Makefile targets `g++`; strict ISO C++14 portability is not assumed.

Run the commands below from the repository root. Database and log paths are relative to that directory.

## Build

```bash
git clone https://github.com/yuki1216x-arch/GEISTER.git
cd GEISTER
make -j2
```

| Command | Assertions | Executable directory |
| --- | --- | --- |
| `make` | Disabled with `-DNDEBUG` | `bin/` |
| `make assert` | Enabled with `-UNDEBUG` | `bin/assert/` |

For assertion-enabled executables:

```bash
make assert -j2
./bin/assert/validation --help
```

Normal and assertion-enabled builds are kept separately. The assertion-enabled build uses the same optimization level as the normal build; it does not automatically enable debug symbols or sanitizers.

The current Makefile does not track header dependencies. After editing a header, rebuild with `make -B` and/or `make assert -B`, or clean both sets of executables first:

```bash
make clean
make -j2
make assert -j2
```

`make clean` removes the contents of `bin/`, including `bin/assert/`. It does not remove databases.

## Quick start with the supplied databases

The reference databases are stored directly in `data/`. The programs read their working databases from `data/db/`.

```bash
mkdir -p data/db data/db_purple data/output data/output/purple data/output/validation
cp data/*_table_*.bin data/db/

# Display 100 randomly sampled configurations and their labels.
./bin/lookup_random 1 1 1 1 1

# Look up a particular board from the self viewpoint.
./bin/lookup 1 1 1 1 1 '6/6/6/6/6/BRuu2'

# Check ZDD mappings and assigned-label conditions for both players to move.
make assert -j2
./bin/assert/validation 1 1 1 1
```

Copying the reference files replaces any working files with the same names. Use a separate checkout or save existing results if you need to retain them.

All five programs accept `--help`.

## Piece counts, viewpoints, and labels

The four piece-count arguments always have this order:

```text
SELF_BLUE SELF_RED OPPONENT_BLUE OPPONENT_RED
```

`self` denotes the player whose piece colors are known in the represented viewpoint. Opponent colors are hidden in the belief-state representation, although the numbers of remaining blue and red pieces are known.

Reversing a viewpoint swaps the self/opponent piece counts and the coordinate orientation. This is distinct from selecting the player to move.

### Belief-state outcome codes

| Code | Identifier | Label |
| --- | --- | --- |
| 0 | `v_unknown` | No FW, CL, or FL label assigned |
| 1 | `v_win` | Forced Win (FW) |
| 2 | `v_loss` | Forced Loss (FL) |
| 3 | `v_can_lose` | Can Lose (CL), without refinement to FL |

In the belief-state decision model, FW guarantees a self win against all compatible opponent color assignments and opposing action plans. CL expresses the existence of an opponent color assignment and action plan that defeats every self action plan. FL requires an appropriate opposing winning action plan for every compatible opponent color assignment.

The database stores one code per entry. FL entries are stored as code 2 rather than retaining a separate CL flag. `v_unknown` is not an assertion that an imperfect-information position is a game-theoretic draw.

Purple-piece analysis uses the win, loss, and unresolved codes. It does not assign CL labels. These results describe the perfect-information variant, not the same decision model as the belief-state labels.

## Construct databases

### Ordinary Geister

```bash
./bin/main ITERATION SELF_BLUE SELF_RED OPPONENT_BLUE OPPONENT_RED forward|reversed
```

Example starting with the smallest nonterminal piece-count class:

```bash
mkdir -p data/db data/output
./bin/main 1 1 1 1 1 forward > data/output/make1-1-1-1.txt 2>&1
```

- `forward` starts with the piece counts as supplied.
- `reversed` starts with the self/opponent counts swapped. When the two count pairs are identical, there is no separate reversed class to process.
- Legacy aliases `s` and `e` mean `forward` and `reversed`, respectively.
- Odd iterations update the self-to-move database; even iterations update the opponent-to-move database.
- `ITERATION` is a propagation-pass index, not a configuration ID or a guaranteed distance to termination.

Construct smaller piece-count classes before larger ones because captures require their databases. Existing files and their iteration headers affect loading; do not assume that any arbitrary iteration value resumes an interrupted computation correctly.

An unavailable input database is initialized with unknown labels by `Table`. This supports construction from scratch, but does not distinguish a missing prerequisite from an intentionally new table. Ensure that required smaller databases have been constructed before running larger classes.

The constructor writes working results in `data/db/` and replaces files at the selected output paths.

### Purple-piece Geister

```bash
./bin/main_purple ITERATION SELF_BLUE SELF_RED OPPONENT_BLUE OPPONENT_RED
```

Example:

```bash
mkdir -p data/db_purple data/output/purple
./bin/main_purple 1 1 1 1 1 > data/output/purple/make1-1-1-1.txt 2>&1
```

This executable is compiled with `USE_PURPLE` by the Makefile and writes to `data/db_purple/`. It takes no viewpoint-orientation argument. Its loop increments the supplied iteration before processing, so the first processed pass in this example is 2. It retains the current implementation's stopping rule: stop after a pass makes no label updates.

### Batch construction

```bash
bash scripts/run_main.sh
bash scripts/run_main_purple.sh
```

Create the directories shown in the quick start before running these scripts. The scripts launch multiple processes and wait between groups of jobs. They contain selected piece-count classes with four through eight remaining pieces; they are not a single small demonstration or a resource-limit check.

Each analysis process currently uses seven worker threads plus a coordinator. Concurrent jobs can therefore require substantial CPU time, memory, and storage. Review the scripts and available resources before starting a full batch. A script's listed classes do not certify that every listed computation has completed in the supplied dataset.

## Query databases

The lookup tools read ordinary-Geister files from `data/db/`. They do not select `data/db_purple/` through a command-line option.

### Query a board

```bash
./bin/lookup PLAYER_TO_MOVE SELF_BLUE SELF_RED OPPONENT_BLUE OPPONENT_RED BOARD
```

`PLAYER_TO_MOVE` is `1` for self to move and `2` for opponent to move. It is not the constructor's `forward`/`reversed` setting.

`BOARD` is a FEN-style string containing six slash-separated rows, with each row expanding to six squares:

| Character | Meaning |
| --- | --- |
| `B` | Self blue piece |
| `R` | Self red piece |
| `u` | Opponent piece with hidden color |
| `1` through `6` | Consecutive empty squares in that row |

Rows are read from the top of the printed board, left to right, corresponding to square indices 35 down to 0. For example:

```bash
./bin/lookup 1 1 1 1 1 '6/6/6/6/6/BRuu2'
```

Supply a valid board whose piece counts agree with the arguments. This is a research utility, not a complete validator for arbitrary board strings. Assertions perform some checks only in assertion-enabled builds.

The output includes the configuration ID, board, numeric label code, and label name.

### Query random configurations

```bash
./bin/lookup_random PLAYER_TO_MOVE SELF_BLUE SELF_RED OPPONENT_BLUE OPPONENT_RED
```

The program samples 100 configuration IDs with replacement, using a clock-seeded random generator. It displays every sampled configuration and its label, including unknown, FW, FL, and CL entries. Duplicates are possible, and a repeated run need not show the same sample.

Sampling is over the indexed configuration class, not a distribution of positions reached in actual play.

## Validate databases

```bash
./bin/assert/validation SELF_BLUE SELF_RED OPPONENT_BLUE OPPONENT_RED
```

For the selected piece-count class, validation:

1. Enumerates configurations independently through node states and compares them with ZDD decoding.
2. Encodes each enumerated board and checks that its ID matches the enumeration order.
3. Checks the successor conditions of assigned FW, CL, and FL labels for both players to move, including reversed-view checks for FL.

The program requires the current and reversed-view databases, plus nonterminal capture databases needed by those checks. It does not require a capture database when capturing the last piece of that color terminates the game. Detected mismatches are reported and cause abnormal termination; a missing required file causes an error exit.

These executable checks do not, by themselves, constitute a proof of every history-dependent strategy property. The verifier checks the represented belief-state model and the conditions implemented in the source. It does not validate Purple-piece databases through a separate command-line mode.

The batch validation script lists selected classes with four through six remaining pieces:

```bash
mkdir -p data/output/validation
bash scripts/run_validation.sh
```

That script invokes `bin/validation`, the normal build. To use assertions for an individual check, invoke `bin/assert/validation` explicitly as above.

## Regression test

```bash
make test
```

This target runs ordinary-Geister construction for `(1,1,1,1)`, `(1,1,1,2)`, and `(1,1,2,1)`, then compares ten generated database files in `data/db/` against the reference files in `data/`. It writes logs to `data/output/` and exits unsuccessfully if a compared file differs.

The test runs database construction and can overwrite working results. It does not run the Purple-piece constructor or the standalone validator. It uses the normal `bin/main` executable.

## Database filenames and binary format

Examples for the piece-count class `(1,1,1,2)`:

```text
self_table_1-1-1-2.bin
enemy_table_1-1-1-2.bin
```

`self_table` means self to move; `enemy_table` means opponent to move. The existing on-disk `enemy_table` prefix is retained for compatibility even though newer variable names use `opponent`.

| Location | Purpose |
| --- | --- |
| `data/` | Supplied reference databases |
| `data/db/` | Working ordinary-Geister databases |
| `data/db_purple/` | Working Purple-piece databases |
| `data/output/` | Construction, validation, and test logs |

Each database begins with an eight-byte header:

| Byte offset | Meaning |
| --- | --- |
| 0 | Reserved; currently zero |
| 1 | Iteration value stored in one byte |
| 2-7 | Configuration count as a six-byte little-endian integer |

The analysis programs write **two bits per entry**, packing four labels per byte from the least significant bits. For configuration ID `id`:

```cpp
byte_offset = 8 + id / 4;
label_code = (packed_byte >> (2 * (id % 4))) & 3U;
```

A final partial byte is padded with zero-valued labels. The header count excludes padding, but `TableWriter::print_statistics()` currently includes padding in its unknown-label count. An entry stores a label only; the board, piece counts, player to move, and strategy are not stored in each entry. Correct interpretation requires the matching filename class and ZDD mapping.

The common `Table` reader also supports the bit widths supplied by its caller; the current analysis, lookup, and validation programs use two-bit databases. Bit width is not recorded in the header.

## Table loading with or without information output

The default C++ API prints the input path, header, and applicable label statistics:

```cpp
Table table(0, database_path.c_str(), 2, configuration_count);
```

To suppress informational messages from loading this table:

```cpp
Table table(0, database_path.c_str(), 2, configuration_count,
            TableReadOutput::silent);
```

The first argument, `0`, disables comparison against a particular iteration value when reading the header. In silent mode, validation and loading still run, and errors remain visible on standard error. This option does not suppress ZDD diagnostics, writer statistics, or messages printed by a calling program. There is currently no CLI switch that silences all program output.

`Table::get(id)` and `Table::set(id, label)` access packed in-memory entries. `TableWriter` writes the header and entries; call `flush()` to finish a partial byte and `print_statistics()` when a written-label summary is wanted.

## Source layout

| Path | Responsibility |
| --- | --- |
| `src/analysis/main.cpp` | Ordinary-Geister database construction |
| `src/analysis/main_purple.cpp` | Purple-piece database construction |
| `src/lookup/lookup.cpp` | Board-string lookup |
| `src/lookup/lookup_random.cpp` | Random-ID lookup |
| `src/validation/validation.cpp` | ZDD and assigned-label validation |
| `src/common/node.hpp`, `node.cpp` | Node constraints, equivalence, and subtree statistics |
| `src/common/zdd_geister.hpp`, `zdd_geister.cpp` | ZDD construction and configuration-ID conversion |
| `src/common/position_geister.hpp`, `position_geister.cpp` | `Position`, actions, viewpoint changes, and compatible color assignments |
| `src/common/table.hpp`, `table.cpp` | Packed label storage, binary I/O, and loading diagnostics |
| `scripts/` | Batch construction and validation commands |
| `Makefile` | Build and regression-test targets |

Recent API names include `ZDD::encode_configuration`, `ZDD::decode_configuration`, `Position::load_configuration`, `Position::get_zdd_id`, and `TableWriter::print_statistics`.

## Operational notes

- Run larger classes only after their prerequisite classes are available. Memory and disk requirements increase rapidly with the number of pieces.
- Keep the reference files in `data/` separate from generated working results in `data/db/`.
- An encoded configuration is not necessarily reachable from a standard initial position. The tools do not apply a separate reachability filter by default.
- Preserve the numeric outcome codes and ID ordering when changing names or implementation details; existing database files depend on both.
- To save both normal output and errors, use `> run.log 2>&1`. To append instead of replacing the log, use `>> run.log 2>&1`.
