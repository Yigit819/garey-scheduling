================================================================================
           SEQUENTIAL TESTING PROBLEM - GAREY (1973) REDUCTION SOLVER
================================================================================

1. WHAT THIS PROJECT DOES
-------------------------
This project takes a set of diagnostic tests with dependency rules (precedence 
arrows) and finds the cheapest order to run them.

- You test components one by one.
- If ANY test fails, the whole system is broken and testing stops immediately.
- If a test passes, you move on to the next one.
- You only pay for the tests you actually end up running.
- Goal: Find a valid test sequence that obeys all arrows while minimizing the 
  total expected testing cost.

Priority Metric (R-value / Smith's Ratio):
    R = Cost / (1 - Success Probability)
    (In other words: Cost divided by Failure Chance)

Rule of thumb: Lower R means the test is cheap and/or very likely to fail early, 
so you want to run it as early as possible.


2. HOW THE GAREY (1973) ALGORITHM WORKS
---------------------------------------
The code repeatedly applies 4 reduction rules until no more changes can be made:

A. Deleting Redundant Arrows (Theorems 3.3 and 4.2)
- If test A has a lower R-value than test B, the greedy order already wants 
  to run A before B naturally.
- The arrow forcing A before B is not restricting anything, so the code 
  safely deletes it (or moves it up to A's prerequisites).

B. Merging Blocked Tasks (Theorems 3.2 and 4.1)
- If test B has a lower R-value than test A, B is more desirable than A, 
  but the arrow forces B to wait for A.
- As soon as A finishes, B should run immediately with nothing in between.
- The code glues them together into a single combined task [A, B] with pooled 
  costs and combined success probabilities.

C. Final Scheduling
- Once all possible reductions are complete, any remaining tasks and merged 
  groups are sorted from lowest R-value to highest R-value.
- Merged groups are unpacked back into their original individual test IDs.
- The code prints the final sequence and calculates the total expected cost.


3. PROJECT DIRECTORY STRUCTURE
------------------------------
PURE/
├── garey1973.cpp             Main C++ reduction solver
├── garey1973.exe             Compiled solver executable
├── garey_master_log.txt      Master log file recording details of all runs
├── summary_table.cpp         C++ script that parses the log into summary tables
├── summary_table.exe         Compiled summary table executable
├── garey_summary_table.txt   Formatted text summary table (per graph, OS1-OS6, overall)
├── garey_summary_table.csv   Spreadsheet-friendly CSV version of the summary table
└── rostami_kucuk/
    ├── os1/ to os6/          Input benchmark folders (from sparse to dense graphs)
    └── gr_os1/ to gr_os6/    Auto-created output folders storing reduced graphs


4. INPUT AND OUTPUT FILE FORMAT (.txt)
--------------------------------------
Both input files (s_n*.txt) and reduced output files (gr_s_n*.txt) use the exact
same structure:

- Line 1: Total number of tasks (nodes)
- Line 2: Comma-separated costs for each task
- Line 3: Comma-separated success probabilities for each task
- Line 4 to End: Arrows written as "A-B" (meaning task A must run before task B)

In the generated "gr_*" files, any merged/remaining tasks are neatly re-indexed 
from 1 to N_new.


5. HOW TO COMPILE AND RUN
-------------------------
A. Main Reduction Solver (garey1973.cpp)
1. Compile the code:
   g++ -std=c++17 -O2 -static garey1973.cpp -o garey1973.exe

2. Run all benchmark folders at once (Batch Mode):
   .\garey1973.exe

   This automatically scans os1 through os6, generates the matching gr_os1 
   through gr_os6 folders, creates all reduced graph files, and writes a complete 
   before-and-after log into garey_master_log.txt.

3. Run on a single file:
   .\garey1973.exe "rostami_kucuk\os1\s_n20_os1_1f.txt"

B. Summary Table Generator (summary_table.cpp)
1. Compile the summary script:
   g++ -std=c++17 -O2 -static summary_table.cpp -o summary_table.exe

2. Generate the summary tables from garey_master_log.txt:
   .\summary_table.exe

   This prints the full comparison table to the terminal and saves both 
   garey_summary_table.txt and garey_summary_table.csv in the project folder.


6. WHAT THE SUMMARY SCRIPT DOES (summary_table.cpp)
---------------------------------------------------
Because garey_master_log.txt contains detailed node-by-node logs for all 120 
graphs, summary_table.cpp parses it automatically to give a clean overview:

- Lists Old Nodes, New Nodes, Node Reduction (%), Old Arcs, New Arcs, and 
  Arc Reduction (%) for every single graph.
- Groups the results by Order Strength (OS1 through OS6) and calculates the 
  average counts and reduction percentages for each density level.
- Computes the overall averages and reduction percentages across all 120 graphs.
- Exports the exact same data to garey_summary_table.csv for easy viewing in Excel.


7. KEY FINDINGS: SPARSE VS. DENSE NETWORKS
------------------------------------------
Running the solver across all 120 benchmark instances (OS1 = sparsest, OS6 = densest)
reveals a clear trade-off in how Garey's (1973) rules behave on general DAGs:

  Group     Avg Nodes (Old -> New)   Node Red (%)   Avg Arcs (Old -> New)   Arc Red (%)
  -------------------------------------------------------------------------------------
  OS1       35.00 -> 32.40              7.43%       55.40  ->  23.45          57.67%
  OS2       35.00 -> 34.15              2.43%       92.50  ->  51.35          44.49%
  OS3       35.00 -> 34.40              1.71%       111.55 ->  75.50          32.32%
  OS4       35.00 -> 34.60              1.14%       116.40 ->  99.45          14.56%
  OS5       35.00 -> 34.95              0.14%       111.45 -> 113.90          -2.20%
  OS6       35.00 -> 35.00              0.00%       108.50 -> 124.40         -14.65%
  -------------------------------------------------------------------------------------
  OVERALL   35.00 -> 34.25              2.14%       99.30  ->  81.34          18.08%

Takeaways:
- Strong preprocessing on sparse graphs (OS1 - OS3): Garey's rules eliminate 
  32% to 58% of all precedence arcs (and even solve some sparse instances to 
  0 remaining arcs). This heavily simplifies the graph before passing it to an 
  exact solver like Branch-and-Bound.
- Why node merging stays low (2.14% overall): Theorems 3.2 and 4.1 require a 
  strict tree-like condition (a merged node can only have 1 predecessor or 1 
  successor). In general DAGs, most nodes have multiple connections, so very 
  few pairs qualify to merge.
- Why arc counts actually INCREASE in dense graphs (OS5 - OS6): When Theorem 
  3.3 or 4.2 removes a single arc (A -> B), it preserves indirect dependencies 
  by adding new arcs from ALL predecessors of A to B (or from A to all 
  successors of B). In dense networks where nodes have many neighbors, deleting 
  1 arc can introduce several new (often transitively redundant) arcs, causing 
  the total edge count to grow unless a transitive reduction pass is applied.
================================================================================
