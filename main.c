#include<stdio.h>
#include"functions.c"

int main()
{
    char ip[1024];
    startsys();
    
    while(1)
    {
        printf("[");
        printf("%s", currentdir);
        printf("]: ");
        gets(ip);

        char* order = strtok(ip, " ");
        char* parameter = strtok(NULL, " ");

        if(!strcmp("my_mkdir", order))
        {
            my_mkdir(parameter);
        }
        else if(!strcmp("my_format", order))
        {
            my_format();
        }
        else if(!strcmp("my_ls", order))
        {
            my_ls();
        }
        else if(!strcmp("my_cd", order))
        {
            my_cd(parameter);
        }
        else if(!strcmp("my_rmdir", order))
        {
            my_rmdir(parameter);
        }
        else if(!strcmp("my_open", order))
        {
            int result = my_open(parameter);
            printf("fd: %d\n", result);
        }
        else if(!strcmp("my_close", order))
        {
            char num = parameter[0];
            int num_final = num - 48;
            my_close(num_final);
        }
        else if(!strcmp("my_create", order))
        {
            my_create(parameter);
        }
        else if(!strcmp("my_rm", order))
        {
            my_rm(parameter);
        }
        else if(!strcmp("my_read", order))
        {
            printf("Please input read size: \n");
            int len;
            scanf("%d", &len);
            my_read(parameter[0] - 48, len);
            getchar();
        }
        else if(!strcmp("my_write", order))
        {
            my_write(parameter[0] - 48);
        }
        else if(!strcmp("my_exitsys", order))
        {
            my_exitsys();
            break;
        }
    }
    return 0;
}