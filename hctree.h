#include <inttypes.h>


// user can define KEY_TYPE before including hctree.h
#ifndef KEY_TYPE
#define KEY_TYPE uint64_t
#endif // end of KEY_TYPE


#define		BTREE 		0x01
#define		HCTREE		0x02


#define		HOT_DATA	0x01
#define		COLD_DATA	0x02
#define		WARM_DATA	0x04

#define 	RETURN_SUCCESS 	0x00
#define 	RETURN_NOMEM 	0x01
#define		RETURN_FAIL		0x02
#define		RETURN_SYSTEM	0x04


#define BLOCK_SIZE		(4096)  //block size in bytes


typedef struct __hctree hctree;

struct __hctree{
	bool isHctree;		// true for hctree, false for b+tree
	int fd;				// file descriptor for this tree
	void*	root;		// root page
	uint64_t iTouched; 	// total touchcount for this tree
};

/*	*
	* Get data from hctree
	* If you already know hotness, give a hint to tree
	* If data are hot then touch count will increase by 2
	* If data are warm then touch count will increase by 1
	* If data are cold then touch count will not increase
	*/
int get(hctree *tree, KEY_TYPE key, bool hot) ;

/* 	*
	* Insert data into hctree
	* If you already know hotness, give a touch count hint
	*/
int put(hctree *tree, KEY_TYPE key, uint64_t hint) ;

/*	*
	* Remove data from hctree
	*/
int remove(hctree *tree, KEY_TYPE key) ;

/* 	*
	* Bulk load the data from files 
	*/
int load(hctree *tree, char *filename);

/*	*
	* Set tree to be b+tree or hctree
	*/
int set_hctree(hctree *tree, bool flag) ;
