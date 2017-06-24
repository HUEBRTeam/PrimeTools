/*
 ____ ___ _   _   ____       _
|  _ \_ _| | | | |  _ \ _ __(_)_ __ ___   ___
| |_) | || | | | | |_) | '__| | '_ ` _ \ / _ \
|  __/| || |_| | |  __/| |  | | | | | | |  __/
|_|  |___|\___/  |_|   |_|  |_|_| |_| |_|\___|

Uncorruption Tool for PIU Prime
*/

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* args[]){

	if(argc < 2 || argc > 2){
		printf("Usage -- uncorrupt [infile]\n");
		return 1;
	}

	FILE *fp;
	fp = fopen(args[1], "rb");
	if(!fp) {
		printf("Error - Cannot Open Infile\n");
		return 1;
	}

	//Get size of the file
	fseek(fp, 0, SEEK_END);
	unsigned int fileSize = ftell(fp);
	rewind(fp);

	//Read our file into a buffer for augmentation.
	char* filebuf = (char*)malloc(fileSize+1);
	fread(filebuf, fileSize, 1, fp);
	fclose(fp);

	//Reopen the handle as rw for overwriting original.
	fp = fopen(args[1], "wb");
	int buffPtr, chunkSize;
	for(buffPtr = 0, chunkSize = 0x1000; buffPtr < fileSize; buffPtr += chunkSize) {
		unsigned char seed = 0x6B * filebuf[buffPtr+97];
		int a = (seed >> 4) + 0x7B;
		unsigned char ub = (seed >> 1) & 7;
		int sOff = buffPtr - ub;

		if(buffPtr >= fileSize - 0x1000)
			chunkSize = fileSize - buffPtr;

		while(a < chunkSize) {
			ub = (filebuf[buffPtr+a]) & 0x80;
			filebuf[sOff+a-1] ^= ub;
			a += 217 + (seed & 1);
		}
	}

	//Write out the augmented buffer.
	fwrite(filebuf, fileSize, 1, fp);
	fclose(fp);

	return 0;
}
