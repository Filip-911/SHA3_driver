#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#define _GNU_SOURCE
#include <stdio.h>


int main(void)
{
  typedef enum state_t {MENU, WRITE_DATA, START} state_t;
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
                puts("2 - Start executing");
                scanf("%u", &state);
            break;
            case WRITE_DATA:
                puts("Write the input for hashing(no longer than 100 charachters)");
                str = (char *) malloc (nbytes + 1);
                int bytes_read = scanf("%99s", str);
                printf("read %d bytes\n", bytes_read);

                fw = fopen("/dev/sha3", "w");
                if(fw == NULL){
                    printf("Opening /dev/sha3 error\n");
                    return -1;}

                asprintf(&str,"addr=0x01, data:%s\n", str);
                fprintf(fw,"%s", str);

                if(fclose(fw)){
                    printf("Closing /dev/sha3 error\n");
                    return -1;}
                else 
                    printf("Wrote the value of blockCount: %d\n", blockCount);

                free(str);
                state = MENU;
            break;
            case START:
                fw = fopen("/dev/sha3", "w");
                if(fw == NULL){
                    printf("Opening /dev/sha3 error\n");
                    return -1;}

                fputs("addr=0x00, start=1\n", fw);

                if(fclose(fw)){
                    printf("Closing /dev/sha3 error\n");
                    return -1;}
                else 
                    puts("Sha3 started\n");

                state = MENU;
            break;
            default:
                puts("Wrong input, try 1 or 2 then enter");
                state = MENU;
                break;
        }
  return 0;
}