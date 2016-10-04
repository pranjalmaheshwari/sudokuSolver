#define MINIGRIDSIZE 4
#define SIZE MINIGRIDSIZE*MINIGRIDSIZE

int **readInput(char *);
int isValid(int **, int **);
int **solveSudoku(int **);
