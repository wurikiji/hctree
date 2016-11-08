#include "hctree.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <inttypes.h>
#include <time.h>

using namespace std;

int main(int argc, char *argv[]) 
{
	hctree tree;
	vector<uint64_t> v;
	int num = MAX_RECORDS ;

	if (argc > 1)
		num = atoi(argv[1]);

	printf("Hello\n");
	hc_open(&tree);

	srand(time(NULL));
	hc_set_hctree(&tree, HCTREE);
	printf("Get and store %d, max record %d\n", num, MAX_RECORDS);
	for (int i =1 ;i <= num; i++)
	{
#if 1
		v.push_back(rand()%1048576);
#else
		v.push_back(i);
#endif
		hc_put(&tree, v[i-1], true);
#if 0
	hc_dump(&tree);
	system("read line");
#endif
		//printf("%d ", i);
	}

	printf("End insert\n");
	printf("Accessed pages %" PRIu64 "\n", tree.iAccessed);
	printf("Split %" PRIu64 "\n", tree.iSplit);

	for (int i = 1;i <= num;i++)
	{
		//printf("Get %d\n", i);
		if (RETURN_EXIST != hc_get(&tree, v[i-1],true))
		{
	hc_dump(&tree);
			break;
		}
	}
	printf("End read\n");
	printf("Accessed pages %" PRIu64 "\n", tree.iAccessed);


	return 0;
}
