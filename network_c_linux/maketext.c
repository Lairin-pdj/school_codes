//
// C Network Programing
// make testset txt
// make by Park Dongjun
//

#include "stdio.h"
#include "stdlib.h"

int main(int argc, char *argv[])
{
	int i;
	int num = atoi(argv[1]);

	FILE *fp = fopen("Text.txt", "w");

	for(i = 1; i < num + 1; i++){
		fprintf(fp, "%08d\n", i);
	}
}
