/*
*	Author		Markus Palonen
*	Date		4.7.2013
*
*	Affine decipher
*
*	This program deciphers an affine ciphered text file 
*	given as an command line argument into stdout.
*	There are exactly 312 possible solutions.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

void affine(char* data, int a, int b) {

	char* affine = malloc((strlen(data)+1)*sizeof(char));
	strcpy(affine, data);
	int i = 0;
	while(affine[i++]) {
		if(!isalpha(affine[i-1]))
			continue;
		if(isupper(affine[i-1]))
			affine[i-1] = tolower(affine[i-1]);
		
		// The actual magic happens here
		affine[i-1] = (char) (((((int) affine[i-1] - 97) + b) * a) % 26) + 97;
	}
	printf("__________\na:%d b:%d\n%s\n", a, b, affine);
	free(affine);
}

int main (int argc, char* argv[]) {

	if (argc != 2) {
		printf("Usage: %s <encrypted text file>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// Open file and handle errors
	FILE* f = fopen(argv[1], "r");
	if(!f)  {
		printf("Could not open file %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	// Copy the file into program memory
	char* data = NULL;
	size_t s = 1;
	int c;
	while((c = fgetc(f)) != EOF) {
		data = realloc(data, ++s*sizeof(char));
		data[s-2] = c;
	}
	data[s-1] = '\0';
	fclose(f);

	// Execute the affine decryption
	for (int a = 1; a <= 25; a += 2) {
		if (a == 13)
			continue;
		for (int b = 0; b <= 25; b++) {
			affine(data, a, b);
		}
		printf("Press enter to continue");
		while(fgetc(stdin) != '\n');
	}
	
	free(data);
}
