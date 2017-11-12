#include "bpt.h"

// MAIN

int main( int argc, char ** argv ) {

    char instruction;  
    char *success_of_find = (char*)malloc(120);
    int64_t key;
    int num;
    char* value = (char*)malloc(120);    
    if (argc== 2) {
        if(open_db(argv[1]) == -1)
        {
            printf("can't open_db\n");
            exit(-1);
        }
    }
    else
    {
        printf("you must set argument to filepath\n") ;
        exit(-1);
    }
    usage();
    printf(">");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 'd':
            scanf("%ld", &key);
            delete(key);
            break;
        case 'i':
            scanf("%ld", &key);
            scanf("%s",value);
            if(insert(key, value)!=0)
            {
                printf("insert error\n");
            }
            break;
  
        case 'f':
            scanf("%ld",&key);
            success_of_find=find(key);
            if(success_of_find==NULL)  printf("The KEY does not exist\n");
            else
            {
                printf("success\n");
                printf("%s\n",success_of_find);
            }
            break;
        case 'p':
            print_leaf();
            break;
        case 'l':
            page_layout();
            break;
        case 'k':
            scanf("%d",&num);
            printf_page(num);
            break; 
        case 'w':
            scanf("%d",&num);
            printf("parent : %d\n",parentwho(num));
            break;
        case 'q':
            while (getchar() != (int)'\n');
            return EXIT_SUCCESS;
            break;
        default:
            usage();
            break;
        }
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");
    return 0;
}
