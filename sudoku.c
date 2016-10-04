/*2013CS50702_2013CS50799*/

#include <stdio.h>
#include <stdlib.h>
#include "sudoku.h"
#include <string.h>
#include <assert.h>
#include <omp.h> 


#define varT long long
#define MAX_THREADS 8
#define MAX_NESTED_THREADS 8

typedef int bool;
#define true 1
#define false 0


//sudoku
struct Sudoku
{
	varT **grid;
	varT **indexing;
	varT **numElems;
	int row;
	int col;
	varT val;
	struct Sudoku * child;
};

//queue
struct queue
{
	int* array;
	int size;
};

//global variables
struct queue q;
omp_lock_t gl;
omp_lock_t ret;
omp_lock_t locks[MAX_THREADS];
bool active[MAX_THREADS];
struct Sudoku * inputPointers[MAX_THREADS];
int **output;
bool exiting;
int depth;

void Init(struct queue * q)
{
	q->array = (int*)malloc(MAX_THREADS*sizeof(int));
	q->size = 0;
}

void Push(struct queue * q,int put)
{
	q->array[q->size] = put;
	q->size++;
}

int Pop(struct queue * q)
{
	q->size--;
	return q->array[q->size];
}

bool Empty(struct queue * q)
{
	return (q->size == 0);
}

void InitAll (struct Sudoku * input)
{
	int i , j , k;
	
	input->grid = (varT **)malloc(sizeof(varT *)*SIZE);
	input->indexing = (varT **)malloc(sizeof(varT *)*SIZE);
	input->numElems = (varT **)malloc(sizeof(varT *)*SIZE);
	
	for ( i = 0; i < SIZE; ++i)
	{
		input->grid[i] = (varT *)malloc(sizeof(varT)*SIZE);
		input->indexing[i] = (varT *)malloc(sizeof(varT)*SIZE);
		input->numElems[i] = (varT *)malloc(sizeof(varT)*SIZE);
	}
}

void copy(struct Sudoku * dest,struct Sudoku * src)
{
	dest->val = src->val;
	dest->row = src->row;
	dest->col = src->col;

	int i;
	
	for ( i = 0; i < SIZE; ++i)
	{
		memcpy(dest->grid[i],src->grid[i],sizeof(varT)*SIZE);
		memcpy(dest->numElems[i],src->numElems[i],sizeof(varT)*SIZE);
		memcpy(dest->indexing[i],src->indexing[i],sizeof(varT)*SIZE);
	}
}

void freeAll (struct Sudoku * input)
{
	int i , j , k;
	
	for ( i = 0; i < SIZE; ++i)
	{
		free(input->grid[i]);
		free(input->indexing[i]);
		free(input->numElems[i]);
	}

	free(input->grid);
	free(input->indexing);
	free(input->numElems);
}

void bin(varT i) 
{
	varT a=0;
	int j = SIZE;
	while(j--)
	{
		a=i;
		i = i >> 1;
		printf("%lld ", a&0x01);
	}
}


void gridPrintBin(varT **grid)
{
	printf("starts here something bin \n\n");
	int i , j , k;

	for (i = 0; i < SIZE; ++i)
	{
		for (j = 0; j < SIZE; ++j)
		{
			bin(grid[i][j]);
			printf("\n");
		}
		printf("\n\n\n");
	}
}

void gridPrint(varT **grid)
{
	printf("starts here something \n\n");
	int i , j , k;

	for (i = 0; i < SIZE; ++i)
	{
		for (j = 0; j < SIZE; ++j)
		{
			printf("%lld  ", grid[i][j] );
		}
		printf("\n\n");
	}
}

void PrintAll(struct Sudoku * input)
{
	gridPrintBin(input->grid);
	gridPrint(input->indexing);
	gridPrint(input->numElems);
}


void getPossibleVals(struct Sudoku * input , int row , int col)
{
	int i, j, k;
	
	assert(input->numElems[row][col] > 1);

	for (i = 0; i < SIZE; ++i)
	{
		//row
		if(i!=col && input->numElems[row][i] == 1)
		{
			input->grid[row][col] &= ~(1LL<<input->indexing[row][i]);
		}
		if(i!=row && input->numElems[i][col] == 1)
		{
			input->grid[row][col] &= ~(1LL<<input->indexing[i][col]);
		}
	}
	
	int rowMax = MINIGRIDSIZE + (row/MINIGRIDSIZE)*MINIGRIDSIZE;
	int colMax = MINIGRIDSIZE + (col/MINIGRIDSIZE)*MINIGRIDSIZE;
	
	for (i = (row/MINIGRIDSIZE)*MINIGRIDSIZE; i < rowMax ; ++i)
	{
		for (j = (col/MINIGRIDSIZE)*MINIGRIDSIZE; j < colMax; ++j)
		{
			if(i!=row && j!=col && input->numElems[i][j] == 1)
			{
				input->grid[row][col] &= ~(1LL<<input->indexing[i][j]);
			}
		}
	}

	input->numElems[row][col] = 0;
	varT val = input->grid[row][col];
	k = 0;
	while(val > 0)
	{
		varT updt = (val & 1LL);
		input->numElems[row][col] += updt;
		val = (val>>1);
		k++;
	}

}


void update(struct Sudoku * input , int row , int col)
{
	varT val =  (input->numElems[row][col] == 1) ? input->grid[row][col] : 0;
	varT k = -1;
	while(val>0)
	{
		val = (val>>1);
		k++;
	}
	input->indexing[row][col] = k;
}

bool loneRanger(struct Sudoku * input , int row , int col, varT index);

bool exclusion(struct Sudoku * input , int row , int col);

bool elimination(struct Sudoku * input , int row , int col)
{
	int i, j, k;
	
	assert(input->numElems[row][col] == 1);

	for (i = 0; i < SIZE; ++i)
	{
		if(i!=col && (input->grid[row][i]>>input->indexing[row][col])&1LL == 1)
		{
			input->grid[row][i] &= ~(1LL<<input->indexing[row][col]);
			input->numElems[row][i] -= 1;
			if(input->numElems[row][i] == 1)
			{
				update(input,row,i);
				if(!elimination(input,row,i))return false;
			}
			if(input->numElems[row][i] < 1)
			{
				return false;
			}
			if(!loneRanger(input,-1,i,input->indexing[row][col]))return false;
		}
		if(i!=row && (input->grid[i][col]>>input->indexing[row][col])&1LL == 1)
		{
			input->grid[i][col] &= ~(1LL<<input->indexing[row][col]);
			input->numElems[i][col] -= 1;
			if(input->numElems[i][col] == 1)
			{
				update(input,i,col);
				if(!elimination(input,i,col))return false;
			}
			else if(input->numElems[i][col] < 1)
			{
				return false;
			}
			if(!loneRanger(input,i,-1,input->indexing[row][col]))return false;
		}
	}

	int rowMax = MINIGRIDSIZE + (row/MINIGRIDSIZE)*MINIGRIDSIZE;
	int colMax = MINIGRIDSIZE + (col/MINIGRIDSIZE)*MINIGRIDSIZE;
	
	for (i = (row/MINIGRIDSIZE)*MINIGRIDSIZE; i < rowMax ; ++i)
	{
		for (j = (col/MINIGRIDSIZE)*MINIGRIDSIZE; j < colMax; ++j)
		{
			if(i!=row && j!=col && (input->grid[i][j]>>input->indexing[row][col])&1LL == 1)
			{
				input->grid[i][j] &= ~(1LL<<input->indexing[row][col]);
				input->numElems[i][j] -= 1;
				if(input->numElems[i][j] == 1)
				{
					update(input,i,j);
					if(!elimination(input,i,j))return false;
				}
				else if(input->numElems[i][j] < 1)
				{
					return false;
				}
				if(!loneRanger(input,i,j,input->indexing[row][col]))return false;
			}
		}
	}

	return true;
}


bool exclusion(struct Sudoku * input , int row , int col)
{
	if(input->numElems[row][col] > 3)return true;

	//get same gd's
	int i, j, k;
	bool whereR[SIZE];
	bool whereC[SIZE];
	varT countR = 0;
	varT countC = 0;

	varT possibleVals[SIZE];
	varT val = input->grid[row][col];
	assert(val > 0);
	k = 0;
	j = 0;
	while(j < input->numElems[row][col])
	{
		assert(val > 0);
		if((val&1LL) == 1)
		{
			possibleVals[j] = (varT)k;
			j++;
		}
		val = val>>1;
		k++;
	}

	for (i = 0; i < SIZE; ++i)
	{
		if(input->grid[row][col] == input->grid[row][i])
		{
			whereC[i] = 1;
			countC++;
		}
		else
		{
			whereC[i] = 0;
		}
	}

	if(countC == input->numElems[row][col])
	{
		for (i = 0; i < SIZE; ++i)
		{
			if(whereC[i] == 0)
			{
				for (j = 0; j < input->numElems[row][col]; ++j)
				{
					if( (input->grid[row][i] & (1LL<<possibleVals[j] )) > 0)
					{
						input->grid[row][i] &= ~(1LL<<possibleVals[j]);
						input->numElems[row][i] -= 1;
						if(input->numElems[row][i] == 1)
						{
							update(input,row,i);
						}
					}
				}
			}
		}
	}
	else if(countC > input->numElems[row][col])
	{
		return false;
	}


	for (i = 0; i < SIZE; ++i)
	{
		if(input->grid[row][col] == input->grid[i][col])
		{
			whereR[i] = 1;
			countR++;
		}
		else
		{
			whereR[i] = 0;
		}
	}
	
	if(countR == input->numElems[row][col])
	{
		for (i = 0; i < SIZE; ++i)
		{
			if(whereR[i] == 0)
			{
				for (j = 0; j < input->numElems[row][col]; ++j)
				{
					if( (input->grid[i][col]&(1LL<<possibleVals[j])) > 0)
					{
						input->grid[i][col] &= ~(1LL<<possibleVals[j]);
						input->numElems[i][col] -= 1;
						if(input->numElems[i][col] == 1)
						{
							update(input,i,col);
						}
					}
				}
			}
		}
	}
	else if(countR > input->numElems[row][col])
	{
		return false;
	}


	varT count = 0;
	bool where[MINIGRIDSIZE][MINIGRIDSIZE];
	
	int rowMax = MINIGRIDSIZE + (row/MINIGRIDSIZE)*MINIGRIDSIZE;
	int colMax = MINIGRIDSIZE + (col/MINIGRIDSIZE)*MINIGRIDSIZE;
	
	for (i = (row/MINIGRIDSIZE)*MINIGRIDSIZE; i < rowMax ; ++i)
	{
		for (j = (col/MINIGRIDSIZE)*MINIGRIDSIZE; j < colMax; ++j)
		{
			if(input->grid[row][col] == input->grid[i][j])
			{
				where[i-rowMax+MINIGRIDSIZE][j-colMax+MINIGRIDSIZE] = 1;
				count++;
			}
			else
			{
				where[i-rowMax+MINIGRIDSIZE][j-colMax+MINIGRIDSIZE] = 0;
			}
		}
	}
	

	if(count == input->numElems[row][col])
	{
		for (i = (row/MINIGRIDSIZE)*MINIGRIDSIZE; i < rowMax ; ++i)
		{
			for (j = (col/MINIGRIDSIZE)*MINIGRIDSIZE; j < colMax; ++j)
			{
				if(where[i-rowMax+MINIGRIDSIZE][j-colMax+MINIGRIDSIZE] == 0)
				{
					for (k = 0; k < input->numElems[row][col]; ++k)
					{
						if((input->grid[i][j]&(1LL<<possibleVals[k])) > 0)
						{
							input->grid[i][j] &= ~(1LL<<possibleVals[k]);
							input->numElems[i][j] -= 1;
							if(input->numElems[i][j] == 1)
							{
								update(input,i,j);
							}
						}
					}
				}
			}
		}
	}
	else if(count > input->numElems[row][col])
	{
		return false;
	}

	return true;

}


bool loneRanger(struct Sudoku * input , int row , int col, varT index)
{
	int i, j, k;
	varT sum = 0;
	int whereR,whereC;
	varT dummy;
	if(col == -1)
	{
		whereR = row;
		for (i = 0; i < SIZE; ++i)
		{
			dummy = (input->grid[row][i]>>index)&1LL;
			sum += dummy;
			whereC = (dummy == 1) ? i : whereC;
			if(sum > 1)return true;
		}
	}
	else if(row == -1)
	{
		whereC = col;
		for (i = 0; i < SIZE; ++i)
		{
			dummy = (input->grid[i][col]>>index)&1LL;
			sum += dummy;
			whereR = (dummy == 1) ? i : whereR;
			if(sum > 1)return true;
		}
	}
	else
	{
		int rowMax = MINIGRIDSIZE + (row/MINIGRIDSIZE)*MINIGRIDSIZE;
		int colMax = MINIGRIDSIZE + (col/MINIGRIDSIZE)*MINIGRIDSIZE;
		
		for (i = (row/MINIGRIDSIZE)*MINIGRIDSIZE; i < rowMax ; ++i)
		{
			for (j = (col/MINIGRIDSIZE)*MINIGRIDSIZE; j < colMax; ++j)
			{
				dummy = (input->grid[i][j]>>index)&1LL;
				sum += dummy;
				whereR = (dummy == 1) ? i : whereR;
				whereC = (dummy == 1) ? j : whereC;
				if(sum > 1)return true;
			}
		}
	}
	if(sum == 1)
	{
		input->grid[whereR][whereC] = 1LL<<index;
		input->numElems[whereR][whereC] = 1;
		input->indexing[whereR][whereC] = index;
		if(!elimination(input,whereR,whereC))return false;
		return true;
	}
	else if(sum < 1)
	{
		return false;
	}

	return true;
}


void rec(int thread_id,struct Sudoku * input)
{
	if(exiting)
	{
		return;
	}
	if(input->row != SIZE && input->col != SIZE)
	{
		input->grid[input->row][input->col] = 1LL<<input->val;
		input->numElems[input->row][input->col] = 1;
		input->indexing[input->row][input->col] = input->val;
		if(!elimination(input,input->row,input->col))return;
	}
	int i, j, k;
	int row = SIZE;
	int col = SIZE;
	int minElems = SIZE+1;
	for (i = 0; i < SIZE; ++i)
	{
		for (j = 0; j < SIZE; ++j)
		{
			if(minElems > input->numElems[i][j] && input->numElems[i][j] > 1)
			{
				minElems = input->numElems[i][j];
				row = i;
				col = j;
			}
			else if(input->numElems[i][j] < 1)
			{
				return;
			}
		}
	}
	if(row == SIZE && col == SIZE)
	{
		omp_set_lock(&ret);
		for (i = 0; i < SIZE; ++i)
		{
			for (j = 0; j < SIZE; ++j)
			{
				output[i][j] = (int)input->indexing[i][j] + 1;
			}
		}
		exiting = 1;
		omp_unset_lock(&ret);
		return;
	}

	varT possibleVals[SIZE];
	varT val = input->grid[row][col];
	assert(val > 0);
	k = 0;
	j = 0;
	while(j < minElems)
	{
		assert(val > 0);
		if(val&1 == 1)
		{
			possibleVals[j] = (varT)k;
			j++;
		}
		val = val>>1;
		k++;
	}

	int st = 0;

	if(!Empty(&q))
	{
		omp_set_lock(&gl);
		if(!Empty(&q))
		{
			int j = Pop(&q);
			omp_unset_lock(&gl);
			input->row = row;
			input->col = col;
			input->val = possibleVals[0];
			copy(inputPointers[j],input);
			active[j] = 1;
			//give some work
			omp_unset_lock(&locks[j]);
			st++;
		}
		else
		{
			omp_unset_lock(&gl);
		}
	}

	input->row = row;
	input->col = col;

	for(k = st; k < minElems; ++k)
	{
		input->val = possibleVals[k];
		copy(input->child,input);
		depth += 1;
		rec(thread_id,input->child);
		depth -= 1;
	}

}


int **solveSudoku(int **originalgrid)
{
	depth = 0;

	struct Sudoku input[MAX_THREADS];
	
	int i , j , k;

	output = (int **)malloc(sizeof(int *)*SIZE);

	for ( i = 0; i < SIZE; ++i)
	{
		output[i] = (int *)malloc(sizeof(int)*SIZE);
	}

	for (i = 0; i < MAX_THREADS; ++i)
	{
		InitAll(&input[i]);
		struct Sudoku * children = &input[i];
		for (j = 0; j < SIZE*SIZE+1; ++j)
		{
			//form children till depth SIZE*SIZE
			children->child = malloc(sizeof(struct Sudoku));
			InitAll(children->child);
			children = children->child;
		}
	}


	for ( i = 0; i < SIZE; ++i)
	{

		for (j = 0; j < SIZE; ++j)
		{

			if(originalgrid[i][j] == 0)
			{
				input[0].grid[i][j] = (1LL<<SIZE) - 1;
			}
			else
			{
				input[0].grid[i][j] = 1LL<<(originalgrid[i][j]-1);
			}

			input[0].indexing[i][j] = (originalgrid[i][j] == 0) ? -1:originalgrid[i][j]-1;
			input[0].numElems[i][j] = (originalgrid[i][j] == 0) ? SIZE:1;

		}
	}

	for (i = 0; i < SIZE; ++i)
	{
		for (j = 0; j < SIZE; ++j)
		{
			if(input[0].numElems[i][j] == 1)
				if(!elimination(&input[0], i , j))
				{
					exiting = true;
				}
		}
	}
	input[0].row = SIZE;
	input[0].col = SIZE;
	input[0].val = SIZE;
	//mapping done


	fflush(stdout);


	Init(&q);
	omp_init_lock(&gl);
	omp_init_lock(&ret);
	exiting = 0;

	for (i = 0; i < MAX_THREADS; ++i)
	{
		inputPointers[i] = &input[i];
		active[i] = 0;
		omp_init_lock(&locks[i]);
	}
	active[0] = 1;

	#pragma omp parallel num_threads(MAX_THREADS) private(i,j,k)
	{
		#pragma omp master
		{
			for (j = 1; j < MAX_THREADS; ++j)
			{
				Push(&q,j);
				omp_set_lock(&locks[j]);
			}
		}

		#pragma omp barrier

		int thread_id = omp_get_thread_num();
		bool flag = 1;

		while(flag)
		{
			if(active[thread_id] == 1)
			{

				rec(thread_id,&input[thread_id]);
				
				omp_set_lock(&locks[thread_id]);
				active[thread_id] = 0;
				omp_set_lock(&gl);
				Push(&q,thread_id);
				omp_unset_lock(&gl);
			}
			else
			{
				omp_set_lock(&locks[thread_id]);
				omp_unset_lock(&locks[thread_id]);
			}
			flag = 0;
			for (j = 0; j < MAX_THREADS; ++j)
			{
				flag |= active[j];
			}
		}


		#pragma omp single
		{
			for (j = 0; j < MAX_THREADS; ++j)
			{
				omp_unset_lock(&locks[j]);
			}
		}
	}



	//before returning remap

	return output;
}	