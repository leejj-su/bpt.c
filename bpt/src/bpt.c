#include "bpt.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int fd = 0;

int open_db(char * pathname)
{
   printf("path : %s\n",pathname);
   if(access(pathname,F_OK) ==-1) //파일이 존재하지 않는경우
   {
      fd = open(pathname, O_RDWR | O_CREAT, 0666); //파일 생성
      printf("new file\n");
      if(fd==-1)  //파일 생성 실패
      {
         printf("file open error when make new file\n");
         return -1;
      }
      else
      {
        //새로운 파일 생성시 초기 설정
         int64_t freeoffset = 4096;
         int64_t rootoffset = 0;
         int64_t number_of_pages = 65; //빈 페이지를 64개 헤더페이지 1개 만들 예정
         pwrite(fd,&freeoffset,8,0);
         pwrite(fd,&rootoffset,8,8);
         pwrite(fd,&number_of_pages,8,16);
         for(int i=1;i<=63;i++) //빈페이지들의 next_free_page_offset 설정
         {
             off_t now_ = i*4096;
             int64_t next = (i+1)*4096;
             pwrite(fd,&next,8,now_);
         }
         return 1;
      }
   }
   else //파일이 이미 존재하는 경우
   {
        fd = open(pathname, O_RDWR , 0666); //파일 열기
        if(fd==-1) 
        {
            printf("file open error when open old files\n");
            return -1;
        }
   }
}


void usage(void)
{
     printf("Enter any of the following commands after the prompt > :\n"
    "\ti <k>  -- Insert <k> (an integer) as both key and value).\n"
    "\tf <k>  -- Find the value under key <k>.\n"
    "\td <k>  -- Delete key <k> and its associated value.\n"
    "\tq -- Quit. (Or use Ctl-D.)\n");
}


void plus_number_of_pages()
{
    int64_t tmp;
    pread(fd,&tmp,8,16);
    tmp++;
    pwrite(fd,&tmp,8,16);
}


int isleaf(int offset)
{
    int is_leaf;
    pread(fd,&is_leaf,4,offset+8);
    if(is_leaf==1) return 1;
    else return 0;
}

int number_of_keys(int64_t page_start_offset)
{
     int number_of_key_at_page;
     pread(fd,&number_of_key_at_page,4,page_start_offset+12); //key 갯수 읽어오기
     return number_of_key_at_page;
}

void set_number_of_keys(int64_t page_start_offset,int number)
{
    pwrite(fd,&number,4,page_start_offset+12);
}

int64_t find_next_offset_of_internal(int offset_,int64_t key)
{
    int offset=offset_;
    int number_of_keys_at_page;
    int64_t comparekey;
    number_of_keys_at_page=number_of_keys(offset); //key 갯수 읽어오기
     for(int i = 1;i<=number_of_keys_at_page;i++)
     {
          printf("internale --> leaf%ld\n",i);
          pread(fd,&comparekey,8,112+16*i);
          if(key<comparekey)
          { 
               pread(fd,&offset,8,112+16*i-8); //오프셋 설정 
               break;
          }
          if(i==number_of_keys_at_page) pread(fd,&offset,8,112+16*i+8); //오프셋 설정, (모든 key보다 큰 경우)
     }
    return offset;
}


int64_t find_leafpage_offset(int start_offset,int64_t key) //internal page
{
    int64_t first_offset=start_offset;
    int64_t second_offset;
    printf("is leaf :%d\n",isleaf(first_offset));
    while(isleaf(first_offset)!=1) //인터널페이지 일때
        {
                 printf("in\n");
                 second_offset=find_next_offset_of_internal(first_offset,key);
                 first_offset= second_offset;
        }
    return first_offset;
}

int64_t root_page_offset()
{
    int64_t root_offset;
    pread(fd,&root_offset,8,8);
    return root_offset;
}

void set_rootpage_offset(int64_t root___offset)
{
    int64_t root_offset = root___offset;
    pwrite(fd,&root_offset,8,8);
}


char* find(int64_t key)
{
    char* result;
    result=(char*)malloc(120); //find 결과 value를 담을 변수
    int64_t leafpage_offset;
    int number_of_keys_at_leaf;
    int64_t comparekey;
    int64_t rootoffset=root_page_offset();
    if(rootoffset==0)
    {
        printf("there is no root");
        return NULL;
    }
        leafpage_offset=find_leafpage_offset(rootoffset,key);     //leaf page에 도달
        printf("leafpage_offset: %ld\n",leafpage_offset);
        number_of_keys_at_leaf=number_of_keys(leafpage_offset);
        printf("number_of_keys_at_leaf : %d\n", number_of_keys_at_leaf);
        for(int j=1;j<=number_of_keys_at_leaf;j++) //linear search
        {
            pread(fd,&comparekey,8,leafpage_offset+128*j);
            printf("comparekey: %d\n",comparekey);
            printf("key: %d\n",key);
            if(key==comparekey) //존재하는 경우
            {
                printf("ok\n");
                pread(fd,result,120,leafpage_offset+128*j+8);
                return result;
            }
        }
        return NULL;
}



int64_t return_preepage_offset()
{
    int64_t return_fp_offset;
    pread(fd,&return_fp_offset,8,0);
    return return_fp_offset;
}

int64_t find_place_to_make_preepage()
{
    int64_t firstoffset = return_preepage_offset();
    int64_t next_freepage_offset;
    int64_t pagesize = 4096;
    int64_t place;
    int64_t i=1;
    while(1)
    {
        pread(fd,&next_freepage_offset,8,firstoffset+i*pagesize);
        if(next_freepage_offset==0)
        { 
            place=firstoffset+i*pagesize;
            break;
        }
        i++;
    }
    return place;
}


void decision_make_preepage()
{
    int64_t firstoffset = return_preepage_offset();
    int64_t next_offset;
    int64_t new_next_offset;
    pread(fd,&next_offset,8,firstoffset);
    if(next_offset==0)
    {
        new_next_offset=find_place_to_make_preepage();
        pwrite(fd,&new_next_offset,8,next_offset);
        plus_number_of_pages();
    }
}


int64_t pree_to_leaf()
{
    int64_t preepage_offset= return_preepage_offset();
    int64_t next_preepage_offset;
    pread(fd,&next_preepage_offset,8,preepage_offset);
    decision_make_preepage();
    pwrite(fd,&next_preepage_offset,8,0);
    return preepage_offset;
}

int64_t pree_to_internal()
{
    int64_t preepage_offset= return_preepage_offset();
    int64_t next_preepage_offset;
    pread(fd,&next_preepage_offset,8,preepage_offset);
    decision_make_preepage();
    pwrite(fd,&next_preepage_offset,8,0);
    return preepage_offset;
}

/*
int insert_into_leaf(int64_t offset,int64_t key,char* value)
{
    int64_t comparekey;
    int number_of_keys=return_number_of_key(offset);
    int i;
    int64_t tmp_offset;
    char tmp_string[120];
    for(i=1;i<=number_of_keys;i++) //들어갈 위치 찾기
    {
        pread(fd,&comparekey,8,i*128);
        if(comparekey>key)
        {
            break;
        }
    }
    for(int j=number_of_keys;j>=i;j--) //밀어주기
    {
        pread(fd,&tmp_offset,8,j*128);
        pread(fd,&tmp_string,120,j*128+8);
        pwrite(fd,&tmp_offset,8,(j+1)*128);
        pwrite(fd,&tmp_string,120,(j+1)*128+8);
   }

}
*/
void buildtree(int64_t key, char* value)
{
    int i=1;
    int64_t new_leaf_page_offset=pree_to_leaf();
    pwrite(fd,&new_leaf_page_offset,8,8); //root설정
    printf("%d\n",new_leaf_page_offset); 
    pwrite(fd,&i,4,new_leaf_page_offset+8); //is leaf 설정
    pwrite(fd,&i,4,new_leaf_page_offset+12);
    pwrite(fd,&key,8,new_leaf_page_offset+128);
    pwrite(fd,value,120,new_leaf_page_offset+136);
}

int64_t get_parent_offset(int64_t leaf_offset)
{
    int64_t parent_offset;
    pread(fd,&parent_offset,0,8);
    return parent_offset;
}

void set_parent_offset(int64_t page_offset, int64_t value_of_offset)
{
    pwrite(fd,&value_of_offset,8,page_offset);
}


int insert_internal(int64_t leftpage_offset,int64_t rightpage_offset,int64_t key)
{
    int64_t parent_offset;
    pread(fd,&parent_offset,8,leftpage_offset);
    int64_t tmp1;
    int64_t tmp2=0;
    if(parent_offset==0)
    {
        int64_t new_internal_offset=pree_to_internal();
        int i = 1;
        set_rootpage_offset(new_internal_offset);
        set_parent_offset(leftpage_offset,new_internal_offset);
        set_parent_offset(rightpage_offset,new_internal_offset);
        pwrite(fd,&leftpage_offset,8,new_internal_offset+120);
        pwrite(fd,&key,8,new_internal_offset+128);
        pwrite(fd,&rightpage_offset,8,new_internal_offset+136);
        pwrite(fd,&i,4,new_internal_offset+12);
        return 1;
    }

    /*
    if(number_of_key==248)
    {
         if(childpage_offset==root_page_offset()) //
         {
             int64_t new_internal_offset = pree_to_internal();
             int64_t new_root_offset = pree_to_internal();
             for(int i=125;i<=248;i++)  //새로만든 sibling에 넣어주기
             {
                tmp1= pread(fd,&tmp1,8,childpage_offset+112+16*i);
                insert_internal(new_internale_offset,tmp1); 
             }
             for(int j=125;j<=248;j++) pwrite(fd,&tmp2,8,childpage_offset+112+16*i);
             tmp = 
             insert_internaln(new_root_offset,)
            
         }

    }
    */
    return 0;

}

int insert_leaf(int64_t leafpage_offset,int64_t key,char* value)
{
    int number_of_keys_at_page = number_of_keys(leafpage_offset);
    int i;
    int64_t tmp_key;
    int64_t comparekey;
    char* tmp_value= (char*)malloc(120);
     for(i = 1;i<=number_of_keys_at_page;i++)
     {
          printf("internale --> leaf%d\n",i);
          pread(fd,&comparekey,8,leafpage_offset+128*i);
            printf("comparekey : %d\n", comparekey);
          if(key<comparekey) break;
     }
    for(int j=number_of_keys_at_page;j>=i;j--)
    {
        printf("j : %d\n",j);
        pread(fd,&tmp_key,8,leafpage_offset+128*j);
        pread(fd,tmp_value,120,leafpage_offset+128*j+8);
        pwrite(fd,&tmp_key,8,leafpage_offset+128*(j+1));
        pwrite(fd,tmp_value,120,leafpage_offset+128*(j+1)+8);
        printf("what\n");
    }
    pwrite(fd,&key,8,leafpage_offset+i*128);
    pwrite(fd,value,120,leafpage_offset+i*128+8);  
    number_of_keys_at_page=number_of_keys_at_page+1;
    pwrite(fd,&number_of_keys_at_page,4,leafpage_offset+12);
    printf("insert_leaf\nleaf_key_number: %d\n", number_of_keys(leafpage_offset));
    return 1;
}


int64_t split_leaf(int64_t leafoffset,int64_t insert_key,char* value)
{
    int64_t new_sibling_leafpage_offset = pree_to_leaf(); //새로운 leaf페이지 할당
    record * tmp_array=(record*)malloc(sizeof(record)*32); //최대 31개 밖에 안들어가므로, 32크기의 메모리공간을 만들어준다.
    record tmp;
    int64_t tmp_next_sibling;
    record init;
    init.key=-1;
    //32개 정렬
    for (int i =1;i<=31;i++)  pread(fd,&tmp_array[i],128,leafoffset+128*i);
    tmp_array[0].key=insert_key;
    strcpy(tmp_array[0].value,value);
    for(int j=0;j<31;j++) //bubble sort
    {
        for (int k=0;k<31-j;k++)
        {
             if(tmp_array[k].key>tmp_array[k+1].key) //key를 기준으로 record정렬
             {
                    tmp=tmp_array[k];
                    tmp_array[k]=tmp_array[k+1];
                    tmp_array[k+1]=tmp;
             }
        }
    } 
    for(int p=0;p<32;p++) printf("%ld, %s\n",tmp_array[p].key,tmp_array[p].value);
    for(int l= 16; l<32;l++) pwrite(fd,&tmp_array[l],128,new_sibling_leafpage_offset+128+(l-16)*128);
    for(int j=17;j<31;j++)  pwrite(fd,&init,128,leafoffset+128+j*128);
    for(int t=1;t<17;t++) pwrite(fd,&tmp_array[t],128,leafoffset+128+t*128);
    pread(fd,&tmp_next_sibling,8,leafoffset+120);
    pwrite(fd,&tmp_next_sibling,8,new_sibling_leafpage_offset+120);
    pwrite(fd,&new_sibling_leafpage_offset,8,leafoffset+120);
    printf("now split\n");
    set_number_of_keys(new_sibling_leafpage_offset,16);
    set_number_of_keys(leafoffset,16);
    insert_internal(leafoffset,new_sibling_leafpage_offset,tmp_array[16].key);
}


int insert(int64_t key,char* value)
{
    int64_t rootoffset=root_page_offset();
    int64_t leafpage_offset;
    if(rootoffset==0)  //트리가 만들어지지 않은경우
    {
        buildtree(key,value);
        return 0;
    }   
    if(find(key)!=NULL) //이미 해당 키가 존재하는 경우
    {
        printf("The key already exists. Do not allow duplicates.\n");
        return -1;
    }
    printf("--------find----------\n");
    leafpage_offset=find_leafpage_offset(rootoffset,key);
    if(number_of_keys(leafpage_offset)==31)
    {
        printf("over 31\n");
        if(split_leaf(leafpage_offset,key,value)==-1)
        {
            printf("split error\n");
            return -1;
        }
        return 0;
    }
    insert_leaf(leafpage_offset,key,value);
    return 0;
}

void print_leaf()
{
    int64_t now=4096;
    int64_t next;
    int64_t key;
    int number;
    int64_t parrent;
    pread(fd,&next,8,4096+120);
    while(now!=0)
    {
        pread(fd,&parrent,8,now);
        printf("parrent : %ld\t");
        number=number_of_keys(now);
        for(int i=1;i<=number;i++)
        {
            pread(fd,&key,8,now+i*128);
            if(key!=0) printf("%ld ",key);
        }
        printf("|\n");
        now = next;
        pread(fd,&next,8,now+120);
    }
}   
