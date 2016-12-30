# sudokuSolver
OpenMP parallel implementation to solve sudoku

To compile/make executable run the following on terminal:<br />
./compile.sh

To run the solver run the following on terminal:<br />
./sudoku number_of_threads input_file

# Input file format:

1) Line represents a row separated by spaces.<br />
2) Spaces represented 0.<br />
3) Numbering 1,2,3,4.....<br />

Example input file format - input.txt<br />

Note: For various sizes of sudoku you need to change the MINIGRIDSIZE macro in "sudoku.h".<br />
In the "sudoku.h" file it is set to solve a 16x16 sudoku.<br />
