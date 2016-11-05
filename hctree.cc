#include <stdio.h>
#include <inttypes.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "hctree.h"

#define KEY_TYPE uint64_t

/* 	*
	* Open new db files using hctree 
	* This function will allocate a space for hctree **tree
	*/
int open(hctree **tree, char *dbname)
{
	*tree = (hctree*) malloc(sizeof(hctree));
	
	if (tree == NULL) {
		goto mem_fail;
	}
	
	memset(*tree, 0, sizeof(hctree));

	tree->fd = open(dbname, O_RDWR | O_DIRECT);
	if (tree->fd) {
		goto open_fail;
	}

mem_fail:

open_fail:
}

/*	*
	* Close db 
	*/
int close(hctree **tree)
{
}

/*	*
	* Get data from hctree
	* If you already know hotness, give a hint to tree
	* If data are hot then touch count will increase by 2
	* If data are warm then touch count will increase by 1
	* If data are cold then touch count will not increase
	*/
int get(hctree *tree, KEY_TYPE key, bool hot) 
{
	return RETURN_SUCCESS;
}

/* 	*
	* Insert data into hctree
	* If you already know hotness, give a touch count hint
	*/
int put(hctree *tree, KEY_TYPE key, uint64_t hint) 
{
	return RETURN_SUCCESS;
}

/*	*
	* Remove data from hctree
	*/
int remove(hctree *tree, KEY_TYPE key) 
{
	return RETURN_SUCCESS;
}

/* 	*
	* Bulk load the data from files 
	*/
int load(hctree *tree, char *filename)
{
	return RETURN_SUCCESS;
}

/*	*
	* Set tree to be b+tree or hctree
	*/
int set_hctree(hctree *tree, bool flag)
{
	tree->isHctree = flag;
	return RETURN_SUCCESS;
}
