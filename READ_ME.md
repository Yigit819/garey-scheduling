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

├── garey1973.cpp           Main C++ program

├── garey1973.exe           Compiled executable

├── garey_master_log.txt    Master log file recording details of all runs

└── rostami_kucuk/

    ├── os1/ to os6/        Input benchmark folders (from sparse to dense graphs)

    └── gr_os1/ to gr_os6/  Auto-created output folders storing reduced graphs





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

1. Compile the code:

   g++ -std=c++17 -O2 -static garey1973.cpp -o garey1973.exe



2. Run all benchmark folders at once (Batch Mode):

   .\garey1973.exe



   This automatically scans os1 through os6, generates the matching gr_os1 

   through gr_os6 folders, creates all reduced graph files, and writes a complete 

   before-and-after log into garey_master_log.txt.



3. Run on a single file:

   .\garey1973.exe "rostami_kucuk\os1\s_n20_os1_1f.txt"

================================================================================
