#include "hctree.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <inttypes.h>
#include <time.h>

using namespace std;

int main(void) 
{
	hctree *tree;
	vector<uint64_t> v;
	int num = MAX_RECORDS * 1024;
	printf("Hello\n");
	hc_open(&tree);

	srand(time(NULL));
	printf("Get and store %d\n", num);
	for (int i =0 ;i < num; i++)
	{
		v.push_back(rand()%1048576);
		hc_put(tree, v[i], 0);
	}

	for (int i = 0;i < num;i++)
	{
		if (RETURN_EXIST != hc_get(tree, v[i],0))
		{
			printf("Not exist %d, %" PRIu64 "\n", i, v[i]);
			break;
		}
	}

	hc_dump(tree);

	return 0;
}
