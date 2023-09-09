#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#define _GNU_SOURCE


	int main(void)
{
  typedef enum state_t {MENU, WRITE, READ, EXIT} state_t;
  state_t state = MENU;
  FILE *fw, *fr;
  char* str;
  int blockCount;
  size_t nbytes = 100;
            
    while(1)
        switch(state)
        {
            case MENU:
                puts("\nMENU:");
                puts("1 - Write in the data");
                puts("2 - Read the hash output");
		puts("3 - Exit the application");
                scanf("%u", &state);
            break;
            case WRITE:
                puts("Write the input for hashing(no longer than 100 charachters)");
                str = (char *) malloc (nbytes + 1);
                int bytes_read = scanf("%99s", str);
                printf("read %d bytes, input: %s\n", bytes_read, str);

                fw = fopen("/dev/sha3", "w");
                if(fw == NULL){
                    printf("Opening /dev/sha3 error\n");
                    return -1;}

                fprintf(fw,"%s\n", str);

                if(fclose(fw)){
                    printf("Closing /dev/sha3 error\n");
                    return -1;}

		free(str);
                state = MENU;
            break;
            case READ:
                fr = fopen("/dev/sha3", "r");
                if(fr == NULL){
                    printf("Opening /dev/sha3 error\n");
                    return -1;}

		int rbytes = 64;
		char* str1 = (char*) malloc(rbytes + 1);
                bytes_read = getline(&str1,&rbytes, fr);
               if(fclose(fr)){
                    printf("Closing /dev/sha3 error\n");
                    return -1;}
		printf("hash output: %s", str1);
 		free(str1);
                state = MENU;
            break;
	    case EXIT:
		return 0;
	    break;
            default:
                puts("Wrong input, try 1 or 2 then enter");
                state = MENU;
                break;
        }
  return 0;
}
