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
   if(access(pathname,F_OK) ==-1) //파일이 존재하지 않을경우
   {
      fd = open(pathname, O_RDWR | O_CREAT, 0666); //파일 생성
      printf("make new data file\n");
      if(fd==-1)  //open 에러 검출
      {
         printf("file open error when make new file\n");
         return -1;
      }
      else
      {
        //첫 db파일 생성시 설정
         int64_t freeoffset = 4096;
         int64_t rootoffset = 0;
         int64_t number_of_pages = preepage+1; //freepage 64개 + headerpage
         pwrite(fd,&freeoffset,8,0); 
         pwrite(fd,&rootoffset,8,8);
         pwrite(fd,&number_of_pages,8,16);
         for(int i=1;i<=preepage;i++) 
         {
             off_t now_ = i*4096;
             int64_t next = (i+1)*4096;
             pwrite(fd,&next,8,now_);
         }
         return 1;
      }
   }
   else //파일이 이미 생성되어 있는 경우
   {
        fd = open(pathname, O_RDWR , 0666); //기존존재파일 열기
        if(fd==-1) 
        {
            printf("file open error when open old files\n");
            return -1;
        }
   }
}


void usage(void) //사용자 인터페이스 메세지
{
     printf("Enter any of the following commands after the prompt > :\n"
    "\ti <k>  -- Insert <k> (an integer) as both key and value).\n"
    "\tf <k>  -- Find the value under key <k>.\n"
    "\td <k>  -- Delete key <k> and its associated value.\n"
    "\tl <k>  -- layout of fage.\n"
    "\tw <k>  -- find parent.\n"
    "\tp <k>  -- leaf page.\n"
    "\tq -- Quit. (Or use Ctl-D.)\n");
}


void plus_number_of_pages() //headerpage에 총page갯수 증가
{
    int64_t tmp;
    int64_t tmp2;
    pread(fd,&tmp,8,16);
    tmp2=(tmp+1)*4096;
    int isleaf=0;
    printf("tmp : %ld tmp2 : %ld\n", tmp,tmp2);
    pwrite(fd,&tmp2,8,tmp*4096);
    pwrite(fd,&isleaf,4,tmp*4096+8);
    int number = 0;
    pwrite(fd,&number,4,tmp*4096+12);
    tmp++;
    pwrite(fd,&tmp,8,16);
}


int isleaf(int offset) //leafpage인지 확인하는 함수
{
    int is_leaf;
    pread(fd,&is_leaf,4,offset+8);
    if(is_leaf==1) return 1;
    else return 0;
}

int number_of_keys(int64_t page_start_offset) //해당 페이지의 key의 갯수를 리턴
{
     int number_of_key_at_page;
     pread(fd,&number_of_key_at_page,4,page_start_offset+12);
     return number_of_key_at_page;
}

void set_number_of_keys(int64_t page_start_offset,int number) //해당 페이지에 key의 갯수를 정해줌 - split시에 사용
{
    pwrite(fd,&number,4,page_start_offset+12);
}

int64_t find_next_offset_of_internal(int offset_,int64_t key) //interpage_offset만이 인자로 들어옴
{
    int offset=offset_;
    int64_t offset_return;
    int number_of_keys_at_page;
    int64_t comparekey;
    number_of_keys_at_page=number_of_keys(offset); //기존 key갯수
     for(int i=1;i<=number_of_keys_at_page;i++) 
     {
          pread(fd,&comparekey,8,offset+112+16*i);
          if(key<comparekey) //key가 해당되는 범위내의 offset 위치는 i번째이다.
          { 
               pread(fd,&offset_return,8,offset+112+16*i-8); 
               break;
          }
          if(i==number_of_keys_at_page)
          {
            pread(fd,&offset_return,8,offset+112+16*i+8); //가장 오른쪽에 위치한 offset 
          }
     }

    return offset_return;
}


int64_t find_leafpage_offset(int start_offset,int64_t key) //internal page
{
    if(isleaf(start_offset)==1) return start_offset;
    int64_t first_offset=start_offset;
    int64_t second_offset;
    while(isleaf(first_offset)!=1) //root에서 부터 internal page들을 거쳐 leaf에 도달하는 반복문
        {
                 second_offset=find_next_offset_of_internal(first_offset,key);
                 first_offset= second_offset;
        }
    return first_offset;
}

int64_t root_page_offset() //headerpage에 저장된 rootpage의 offset을 리턴하는 함수
{
    int64_t root_offset;
    pread(fd,&root_offset,8,8);
    return root_offset;
}

void set_rootpage_offset(int64_t root___offset) //headerpage에 저장된 rootpage의 offset을 설정하는 함수
{
    int64_t root_offset = root___offset;
    pwrite(fd,&root_offset,8,8);
}


char* find(int64_t key)
{
    char* result;
    result=(char*)malloc(120); //find에 성공했을때 return할 변수
    int64_t leafpage_offset;
    int number_of_keys_at_leaf;
    int64_t comparekey;
    int64_t rootoffset=root_page_offset();
    if(rootoffset==0) //tree 가 존재하지 않음
    {
        printf("there is no root\n");
        return NULL;
    }
        leafpage_offset=find_leafpage_offset(rootoffset,key);     //해당 키가 존재하는 leafpage 찾기
        number_of_keys_at_leaf=number_of_keys(leafpage_offset);
        for(int j=1;j<=number_of_keys_at_leaf;j++) //linear search
        {
            pread(fd,&comparekey,8,leafpage_offset+128*j);
            if(key==comparekey) //키 존재
            {
                pread(fd,result,120,leafpage_offset+128*j+8);
                return result;
            }
        }
        return NULL;
}



int64_t return_preepage_offset() //header page에 저장된 첫번째 freepage의 offset을 리턴하는 함수
{
    int64_t return_fp_offset;
    pread(fd,&return_fp_offset,8,0);
    return return_fp_offset;
}

int64_t pree_to_leaf() //freepage를 leaf페이지로 바꿔주는 함수
{
    printf("leaf\n");
    int64_t first_free_offset;
    int64_t number_of_page;
    pread(fd,&first_free_offset,8,0);
    pread(fd,&number_of_page,8,16);
    printf("first_free : %ld number_of_page : %ld\n", first_free_offset,number_of_page);
    if((number_of_page-1)*4096==first_free_offset) plus_number_of_pages();
    int i=1;
    int64_t preepage_offset= return_preepage_offset(); //1번째 freepage offset
    int64_t next_preepage_offset;
    int64_t init = 0;
    pread(fd,&next_preepage_offset,8,preepage_offset); //2번째 freepage_offset
    pwrite(fd,&next_preepage_offset,8,0); //headerpage에 저장된 첫번째 freepage의 offset값을 바꿔줌
    pwrite(fd,&i,4,preepage_offset+8); //isleaf를 1로 만들어준다.
    pwrite(fd,&init,8,preepage_offset);
    printf("make is leaf : 1 at %ld\n", preepage_offset);
    return preepage_offset;
}

int64_t pree_to_internal() //freepage를 internal page로 바꿔주는 함수
{
    printf("interal\n");
    int64_t first_free_offset;
    int64_t number_of_page;
    pread(fd,&first_free_offset,8,0);
    pread(fd,&number_of_page,8,16);
    printf("first_free : %ld number_of_page : %ld\n", first_free_offset,number_of_page);
    if((number_of_page-1)*4096==first_free_offset) plus_number_of_pages();
    int64_t preepage_offset= return_preepage_offset();
    int64_t next_preepage_offset;
    int i =0;
    int64_t init = 0;
    pread(fd,&next_preepage_offset,8,preepage_offset);
    pwrite(fd,&next_preepage_offset,8,0);
    pwrite(fd,&i,4,preepage_offset+8);
    pwrite(fd,&init,8,preepage_offset);
    
    return preepage_offset;
}

void buildtree(int64_t key, char* value) //tree 생성
{
    int i=1;
    int64_t j=0;
    int64_t new_leaf_page_offset=pree_to_leaf();
    pwrite(fd,&new_leaf_page_offset,8,8); //headpage에 rootpageoffset 바꿔주기
    int64_t next;
    //pread(fd,&next,8,new_leaf_page_offset);
    //pwrite(fd,&next,8,0);
    pwrite(fd,&j,8,new_leaf_page_offset);
    printf("build new tree: %ld\n",new_leaf_page_offset); 
    pwrite(fd,&i,4,new_leaf_page_offset+8); //is leaf=1
    pwrite(fd,&i,4,new_leaf_page_offset+12); //key 갯수 1로 설정
    pwrite(fd,&key,8,new_leaf_page_offset+128); //key 삽입
    pwrite(fd,value,120,new_leaf_page_offset+136); //value 삽입
}

int64_t get_parent_offset(int64_t leaf_offset) //페이지의 0~8에 저장되는 parrent offset을 리턴하는 함수
{
    int64_t parent_offset;
    pread(fd,&parent_offset,8,leaf_offset);
    return parent_offset;
}

void set_parent_offset(int64_t page_offset, int64_t value_of_offset) //페이지의 0~8에 저장되는 parrent offset을 설정하는 함수
{
    pwrite(fd,&value_of_offset,8,page_offset);
}

int64_t split_internal(int64_t internal_offset,int64_t key,int64_t right_leafpage_offset,int64_t left_leafpage_offset)//internal page에서 split
{
    int64_t new_sibling_internalpage_offset = pree_to_internal(); //새로운 internal page 할당
    internal_record * tmp_array = (internal_record*)malloc(sizeof(internal_record)*internal_degree); //248크기의 배열 동적할당
    internal_record tmp;
    for(int i=1;i<=internal_degree-1;i++) 
    {
        pread(fd,&tmp_array[i],16,internal_offset+112+i*16); //기존 존재하던 key offset들 넣어주기
    }
    tmp_array[0].key=key; //새로 들어갈 key
    tmp_array[0].offset=right_leafpage_offset; //새로 들어갈 offset
    internal_record init;
    /*
    if(tmp_array[0].key<tmp_array[1].key) //120바이트 자리가 바꿔어야 되는 경우
    {
       pwrite(fd,&left_leafpage_offset,8,internal_offset+120);
    }
    */
    init.key=0;
    init.offset=0;
    for(int j=0;j<internal_degree-1;j++) //bubble sort
    {
        for (int k=0;k<internal_degree-j-1;k++)
        {
             if(tmp_array[k].key>tmp_array[k+1].key) //key를 기준으로 record정렬
             {
                    tmp=tmp_array[k];
                    tmp_array[k]=tmp_array[k+1];
                    tmp_array[k+1]=tmp;
             }
        }
    }
    for(int p=0;p<internal_degree;p++) printf("split internal :%ld, %ld\n",tmp_array[p].key,tmp_array[p].offset);

    for(int q=1;q<=internal_degree/2;q++)  //부모 지정해주기
    {
        pwrite(fd,&internal_offset,8,tmp_array[q-1].offset);
    }
    for(int r=(internal_degree+1)/2; r<=internal_degree;r++) //부모 지정해주기
    {
        pwrite(fd,&new_sibling_internalpage_offset,8,tmp_array[r-1].offset);
    }
    for(int s=0;s<internal_degree/2;s++) //왼쪽 값 저장 및 안쓰는 부분 0으로 초기화
    {
        pwrite(fd,&tmp_array[s],16,internal_offset+128+s*16);
        pwrite(fd,&init,16,internal_offset+128+(s+internal_degree/2)*16);
    }
    pwrite(fd,&tmp_array[internal_degree/2].offset,8,new_sibling_internalpage_offset+120);//오른쪽 120 
    for(int p=0;p<internal_degree/2;p++)//오른쭉 값 저장
    {
        pwrite(fd,&tmp_array[(internal_degree+1)/2+p],16,new_sibling_internalpage_offset+128+p*16);
    }
    /*
    for(int i=1;i<=internal_degree/2;i++) //리프페이지 0~8에 부모 offset 입력
    {
        pwrite(fd,&internal_offset,8,tmp_array[i-1].offset);
    }
    for(int y= (internal_degree+1)/2;y<=internal_degree;y++) pwrite(fd,&new_sibling_internalpage_offset,8,tmp_array[y].offset);

    for(int l= internal_degree/2+1;l<internal_degree;l++) pwrite(fd,&tmp_array[l],16,new_sibling_internalpage_offset+128+(l-(internal_degree/2))*16); //새로만든 sibling internal page에 넣어주기
    for(int j=internal_degree/2;j<internal_degree;j++)  pwrite(fd,&init,16,internal_offset+128+j*16); //없애주기
    for(int t=0;t<(internal_degree/2)-1;t++) pwrite(fd,&tmp_array[t],16,internal_offset+128+t*16); //넣어주기
    */
    set_number_of_keys(new_sibling_internalpage_offset,internal_degree/2);
    set_number_of_keys(internal_offset,internal_degree/2);
    int64_t result =insert_internal(internal_offset,new_sibling_internalpage_offset,tmp_array[(internal_degree/2)].key);   
   free(tmp_array); 
    return result; 
}

int64_t insert_internal(int64_t leftpage_offset,int64_t rightpage_offset,int64_t key)
{
    int64_t parent_offset;
    pread(fd,&parent_offset,8,leftpage_offset);
    int64_t tmp1;
    int64_t tmp2=0;
    if(leftpage_offset==root_page_offset()) //leafpage_offset이 rootpage인 경우
    {
        
        int64_t new_internal_offset=pree_to_internal(); //root page로 사용될 internal page할당
        int i = 1;
        set_rootpage_offset(new_internal_offset); //root설정
        set_parent_offset(leftpage_offset,new_internal_offset); //해당 internal페이지를 parent로 설정
        set_parent_offset(rightpage_offset,new_internal_offset);
        pwrite(fd,&leftpage_offset,8,new_internal_offset+120);  //왼쪽 leaf 오프셋
        pwrite(fd,&key,8,new_internal_offset+128);  //key
        pwrite(fd,&rightpage_offset,8,new_internal_offset+136); //오른쪽 leaf 오프셋
        pwrite(fd,&i,4,new_internal_offset+12); //number_of_keys == 1
        return parent_offset;
    }
    if(number_of_keys(parent_offset)==(internal_degree-1)) //다찬경우
    {
                int64_t root=split_internal(parent_offset,key,rightpage_offset,leftpage_offset); //split 실행
                return root;
    }

    //자리가 존재하는 경우
    int64_t numbers= number_of_keys(parent_offset);
    int64_t comparekey;
    int i;
    for(i=1;i<=numbers;i++) //들어갈 위치 찾기
    {
        pread(fd,&comparekey,8,(parent_offset+112)+16*i);
        if(comparekey>key) break;
        
    }
    internal_record tmp;
    for (int j = numbers; j>=i ; j--) //밀어주기
    {
        pread(fd,&tmp,16,parent_offset+112+16*j); 
        pwrite(fd,&tmp,16,parent_offset+128+16*j);  
    }
    if(i==1)
    {
        pwrite(fd,&key,8,parent_offset+112+16*i);
        pwrite(fd,&rightpage_offset,8,parent_offset+112+16*i+8);
        pwrite(fd,&leftpage_offset,8,parent_offset+112+16*i-8);
    }
    else
    {
        pwrite(fd,&key,8,parent_offset+112+16*i);
        pwrite(fd,&rightpage_offset,8,parent_offset+112+16*i+8);
    }
    pwrite(fd,&key,8,parent_offset+112+16*i);
    pwrite(fd,&rightpage_offset,8,parent_offset+112+16*i+8);
    set_number_of_keys(parent_offset,++numbers);
   
     //해당 internal페이지를 parent로 설정
    set_parent_offset(rightpage_offset,parent_offset);
    
    return parent_offset;
}

int insert_leaf(int64_t leafpage_offset,int64_t key,char* value)
{
    int number_of_keys_at_page = number_of_keys(leafpage_offset);
    int i;
    int64_t tmp_key;
    int64_t comparekey;
    char* tmp_value= (char*)malloc(120);
     for(i = 1;i<=number_of_keys_at_page;i++) //들어갈 위치 찾기
     {
          pread(fd,&comparekey,8,leafpage_offset+128*i);
          if(key<comparekey) break;
     }
    for(int j=number_of_keys_at_page;j>=i;j--) //밀어주기
    {
        pread(fd,&tmp_key,8,leafpage_offset+128*j);
        pread(fd,tmp_value,120,leafpage_offset+128*j+8);
        pwrite(fd,&tmp_key,8,leafpage_offset+128*(j+1));
        pwrite(fd,tmp_value,120,leafpage_offset+128*(j+1)+8);
    }
    pwrite(fd,&key,8,leafpage_offset+i*128);
    pwrite(fd,value,120,leafpage_offset+i*128+8);  
    number_of_keys_at_page=number_of_keys_at_page+1;
    pwrite(fd,&number_of_keys_at_page,4,leafpage_offset+12);
    free(tmp_value);
    return 1;
}


int64_t split_leaf(int64_t leafoffset,int64_t insert_key,char* value)
{
    int64_t new_sibling_leafpage_offset = pree_to_leaf(); //새 leaf페이지 할당
    record * tmp_array=(record*)malloc(sizeof(record)*leaf_degree); //해당 레코드를 담을 32 크기의 어레이
    record tmp;
    int64_t tmp_next_sibling;
    record init;
    init.key=-1;
    for (int i =1;i<=leaf_degree-1;i++)  pread(fd,&tmp_array[i],128,leafoffset+128*i); //기존 레코드 삽입
    tmp_array[0].key=insert_key;
    strcpy(tmp_array[0].value,value);
    for(int j=0;j<leaf_degree-1;j++) //bubble sort
    {
        for (int k=0;k<leaf_degree-j-1;k++)
        {
             if(tmp_array[k].key>tmp_array[k+1].key)
             {
                    tmp=tmp_array[k];
                    tmp_array[k]=tmp_array[k+1];
                    tmp_array[k+1]=tmp;
             }
        }
    } 
    //for(int p=0;p<leaf_degree;p++) printf("%d== %ld, %s\n",p+1,tmp_array[p].key,tmp_array[p].value);
    for(int l= leaf_degree/2; l<leaf_degree;l++) pwrite(fd,&tmp_array[l],128,new_sibling_leafpage_offset+128+(l-(leaf_degree/2))*128);
    for(int j=leaf_degree/2-1;j<leaf_degree-1;j++)  pwrite(fd,&init,128,leafoffset+128+j*128);
    for(int t=0;t<leaf_degree/2;t++) pwrite(fd,&tmp_array[t],128,leafoffset+128+t*128);


    pread(fd,&tmp_next_sibling,8,leafoffset+120);//left가 가르키고 있던 sibling offset
    printf("now: %ld  next : %ld return : %d\n",leafoffset ,tmp_next_sibling, pread(fd,&tmp_next_sibling,8,leafoffset+120));
    /*
    if(tmp_next_sibling==leafoffset)
    {
        pwrite(fd,&new_sibling_leafpage_offset,8,leafoffset+120);//left가 right가르침;
        tmp_next_sibling=0;
    }
    */
    if(tmp_next_sibling!=0)
    {
        printf("right\n");
        pwrite(fd,&tmp_next_sibling,8,new_sibling_leafpage_offset+120);//right가 가르키게함
    }
    pwrite(fd,&new_sibling_leafpage_offset,8,leafoffset+120);//left가 right가르침

    set_number_of_keys(new_sibling_leafpage_offset,leaf_degree/2);
    set_number_of_keys(leafoffset,leaf_degree/2);
    insert_internal(leafoffset,new_sibling_leafpage_offset,tmp_array[leaf_degree/2].key);
    return 1;
}



int insert(int64_t key,char* value)
{
    int64_t rootoffset=root_page_offset();
    int64_t leafpage_offset;
    if(rootoffset==0)  //tree가 빌드되지 않은경우
    {
        buildtree(key,value);
        return 0;
    }   
    if(find(key)!=NULL) //이미 해당 키가 존재하는 경우
    {
        printf("The key already exists. Do not allow duplicates.\n");
        return -1;
    }
    leafpage_offset=find_leafpage_offset(rootoffset,key);
    if(number_of_keys(leafpage_offset)==leaf_degree-1) //다 차서  split이 필요한 경우
    {
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

int64_t get_sibling_offset(int64_t offset)
{
    int64_t parentoffset;
    pread(fd,&parentoffset,8,offset);
    int number_of_parent=number_of_keys(parentoffset);
    if(root_page_offset()==offset)
    {
        return 0;
    }
    int64_t* offset_arr=(int64_t*)malloc(sizeof(int64_t)*(number_of_parent+1));
    for(int i=0;i<=number_of_parent;i++) pread(fd,&offset_arr[i],8,parentoffset+120+16*i);
    if(offset_arr[0]==offset)
    {
        return 1;
        //가장 왼쪽인 경우
    }
    for(int j=0;j<=number_of_parent;j++)
    {
        if(offset_arr[j]==offset) return offset_arr[j-1];
        //일반적인 경우
    }
    return 0;
}


void make_preepage(int64_t offset)
{
   int64_t first=return_preepage_offset();
   pwrite(fd,&offset,8,0);
   pwrite(fd,&first,8,offset);
   int init=0;
   pwrite(fd,&init,4,offset+8);
   pwrite(fd,&init,4,offset+12);
}

int64_t delete_or_change_internal(int64_t offset,int64_t key,int mode)
{

    if(mode==1) //change
    {
        printf("offset :%ld change : %ld\n",offset,key);
        pwrite(fd,&key,8,offset);
        return 1;
    }
    printf("delete internal : offset %ld : key : %ld\n",offset,key);
    int64_t now=(offset/4096)*4096;
    int number=(int)((offset%4096)-128)/16+1;  //몇번째인지 확인
    int64_t left_child;
    pread(fd,&left_child,8,offset-8);
    int number_of_k=number_of_keys(now);
    printf("%d  number_of_intenal : %d\n",number, number_of_k);
    if(number_of_k==1)
    {
        if(root_page_offset()==now)
        {
            set_rootpage_offset(left_child);
        }
        make_preepage(now);
    }
    internal_record init;
    for(int i=number+1;i<=number_of_k;i++)
    {
        pread(fd,&init,16,now+112+16*i);
        pwrite(fd,&init,16,now+112+16*(i-1));
    }
    set_number_of_keys(now,number_of_k-1);
    merge_and_Redistribution_internal(now);
    return 1;
}

int64_t merge_and_Redistribution_leaf(int64_t page_offset)
{
    int64_t sibling;
    int64_t left;
    int64_t right;
    sibling=get_sibling_offset(page_offset);
    int number_=number_of_keys(page_offset);
    if(number_>page_offset/2) return 0; //절반 이상 차있을때
    int64_t root;
    pread(fd,&root,8,8);
    if(root==page_offset) return 0; //leaf가 root일때
    int number_right;
    int number_left;
    if(sibling==0) return 0; //sibling이 존재하지 않을때
    
    if(sibling==1) { //sibling이 오른쪽에 있을때
        pread(fd,&right,8,page_offset+120);
        sibling=right;
        left= page_offset;
    }
    else
    {
        left=sibling;
        right=page_offset;
    }
     printf("left : %ld right : %ld\n",left,right);
    int sibling_number=number_of_keys(sibling);
    int64_t parent_offset = get_parent_offset(page_offset);
    number_right=number_of_keys(right);
    printf("number_of_right : %d\n",number_right);
    number_left=number_of_keys(left);
    printf("number_of_left : %d\n",number_left); 
    record * tmp_array=(record *)malloc(sizeof(record)*(number_right+number_left));

    int number=0;
    for(int i=1;i<=number_left;i++)
    {
        pread(fd,&tmp_array[number],128,left+128*i);
        number++;
    }
    //int64_t median_key;
    int64_t median_offset;
    int64_t tmp;
    int64_t number_of_key=number_of_keys(parent_offset);
    int n=number_of_keys(parent_offset);
    for(int i=1;i<=n;i++)
    {
       pread(fd,&tmp,8,parent_offset+104+i*16);
       if(tmp==left)
       {
                //pread(fd,&median_key,8,parent_offset+120+i*16+8);
                median_offset=parent_offset+104+i*16+8;
                break;
       } 
    }  
    for(int j=1;j<=number_right;j++)
    {
        pread(fd,&tmp_array[number],128,right+128*j);
        number++;
    }
    for(int k=0;k<number;k++) printf("%d:key : %ld\n",k+1,tmp_array[k].key);
    //배열 생성
    if(sibling_number<=(leaf_degree)/2) //합병
    {
        printf("merge_leaf\n");
        for(int i=1;i<=number;i++)
        {
            printf("now merging %d \n ",i);
             pwrite(fd,&tmp_array[i-1],128,left+128*i);
        }   
        set_number_of_keys(left,number); //키 갯수 설정
        int64_t right_sibling;
        int64_t init =0;
        pread(fd,&right_sibling,8,right+120); //오른쪽 리프페이지 설정
        if(right_sibling!=0)
        {
            printf("right_sibling = %ld\n",right_sibling);
             pwrite(fd,&right_sibling,8,left+120);
        }
        else
        {
            pwrite(fd,&init,8,left+120);
        }
        make_preepage(right);
        delete_or_change_internal(median_offset,tmp_array[number-1].key,0);
    }
    else if(sibling_number>(leaf_degree)/2) //재분배
    {
        printf("re_leaf\n");
        int num=0;
        for(int i=1;i<=(number+1)/2;i++)
        {
            pwrite(fd,&tmp_array[num],128,left+128*i);
            num++;
        }
        set_number_of_keys(left,(number+1)/2);
        delete_or_change_internal(median_offset,tmp_array[num].key,1); //change leaf key
           for(int j=1;j<=number/2;j++)
            {
                pwrite(fd,&tmp_array[num],128,right+128*j);
                num++;
            }
            set_number_of_keys(right,number/2);
    }
    free(tmp_array);
    return 0;

}

int64_t merge_and_Redistribution_internal(int64_t page_offset)
{
    int64_t sibling;
    int64_t left;
    int64_t right;
    sibling=get_sibling_offset(page_offset);
    printf("--------------------internal\n");
    int number_=number_of_keys(page_offset);
    if(number_>internal_degree/2) return 0; //절반 이상 차있을때
    
    int64_t root;
    pread(fd,&root,8,8);
    int number_right;
    int number_left;
    int64_t parent_offset = get_parent_offset(page_offset);
    int64_t number_of_key=number_of_keys(parent_offset);
    if(sibling==0) return 0; //sibling이 존재하지 않을때
    
    if(sibling==1) { //sibling이 오른쪽에 있을때   
        pread(fd,&right,8,parent_offset+120+16*(number_of_key));
        printf("right: %ld\n",right);
        sibling=right;
        left= page_offset;
        printf("sibling right \n");
    }
    else
    {
        printf("sibling left\n");
        left=sibling;
        right=page_offset;
    }
    printf("left : %ld right : %ld\n",left,right);
    int sibling_number=number_of_keys(sibling);
    printf("printf parent : %ld\n", parent_offset);
    number_right=number_of_keys(right);
    number_left=number_of_keys(left); 
    printf("left : %d  right : %d", number_left,number_right);
    internal_record * tmp_internal_array=(internal_record *)malloc(sizeof(internal_record)*(number_right+number_left+1));
    int number=0;
    for(int i=1;i<=number_left;i++)
    {
        pread(fd,&tmp_internal_array[number],16,left+(16*i)+112);
        number++;
    }
    int64_t median_key;
    int64_t median_offset;
     //인터널인경우 부모의 사이 값을 넣어줍니다.
        int64_t tmp;
        for(int i=0;i<=number_of_key;i++)
        {
            pread(fd,&tmp,8,parent_offset+120+i*16);
            if(tmp==left)
            {
                int64_t l;
                pread(fd,&l,8,right+120);
                pread(fd,&median_key,8,parent_offset+120+i*16+8);
                median_offset=parent_offset+120+i*16+8;
                tmp_internal_array[number].key=median_key;
                tmp_internal_array[number].offset=l;
                number++;
                break;
            }
        }  
    
    for(int j=1;j<=number_right;j++)
    {
        pread(fd,&tmp_internal_array[number],16,right+16*j+112);
        number++;
    }

     for(int k=0;k<number;k++) printf("%d:key : %ld   %ld\n",k+1,tmp_internal_array[k].key,tmp_internal_array[k].offset);
    
    //배열 생성
    if(sibling_number==(leaf_degree)/2) //합병
    {
            printf("merge\n");
            for(int j=1;j<=number;j++)
            {
                pwrite(fd,&tmp_internal_array[j-1],16,left+114+16*j);
            }
            set_number_of_keys(left,number);
            //pree(right);
            delete_or_change_internal(median_offset,tmp_internal_array[number/2].key,0);
    }
    else if(sibling_number>(leaf_degree)/2) //재분배
    {
        printf("re\n");
        int num=0;
            for(int i=1;i<=(number+1)/2;i++)
            {   
                pwrite(fd,&tmp_internal_array[num],16,left+112*i);
                num++;
            }
            delete_or_change_internal(median_offset,tmp_internal_array[num].key,1); //change internal key
            for(int j=1;j<=number/2;j++)
            {   
                pwrite(fd,&tmp_internal_array[num],16,right+112*j);
                num++;
            }
    }
    free(tmp_internal_array);
    return 0;

}


int delete(int64_t key)
{   

    if(root_page_offset()==0)  //tree가 빌드되지 않은경우
    {
        printf("data tree does not exist.\n");
        return -1;
    }
    if(find(key)==NULL)
    {
       printf("The key does not exist.\n");
       return -1;
    }
    int64_t leafpage_offset=find_leafpage_offset(root_page_offset(),key);
    int number_of_key=number_of_keys(leafpage_offset);
    if(number_of_key==1) 
    {
        printf("clean\n");
        if(root_page_offset()==leafpage_offset)
        {
            int64_t init=0;
            pwrite(fd,&init,8,8);
        }
        make_preepage(leafpage_offset);
        return 0;
    }
    record * arr=(record*)malloc(sizeof(record)*number_of_key);
    int target;
    record tmp;
    for(int i=0;i<number_of_key;i++) 
    {
        pread(fd,&arr[i],128,leafpage_offset+(i+1)*128);
        if(arr[i].key==key)
        {
             target=i+1;
             if(i==0)
             {
                 int64_t k;
                 pread(fd,&k,8,leafpage_offset+256);
            
                 int64_t median_key;
                 int64_t median_offset;
                 int64_t tmp;
                 printf("leafpage_offset :%ld root: %ld \n",leafpage_offset,root_page_offset());
                 if(root_page_offset()!=leafpage_offset)
                 {
                 printf("parent ok\n");
                 int64_t parent_offset=get_parent_offset(leafpage_offset);
                 int number_of = number_of_keys(parent_offset);
                 for(int i=0;i<=number_of;i++)
                 {
                      pread(fd,&tmp,8,parent_offset+120+i*16);
                      if(tmp==leafpage_offset)
                      {
                            pread(fd,&median_key,8,parent_offset+120+i*16+8);
                            median_offset=parent_offset+120+i*16+8;
                            if(i==number_of)
                            {
                                printf("right\n");  
                                median_offset=parent_offset+120+i*16-8;
                            }
                            break;
                      }
                 }
                 delete_or_change_internal(median_offset,k,1);
                 }
                 record tmp1;
                 for(int j=target+1;j<=number_of_key;j++)
                 {
                    printf("j=:%d\n",j);
                    pread(fd,&tmp1,128,leafpage_offset+(j*128));
                    printf("tmp : %ld\n",tmp1.key);
                    pwrite(fd,&tmp1,128,leafpage_offset+(j-1)*128);
                 }
                 set_number_of_keys(leafpage_offset,number_of_key-1);
                 merge_and_Redistribution_leaf(leafpage_offset);
                 free(arr);
                 return 0;
             }
        }
    }
    for(int j=target+1;j<=number_of_key;j++)
    {
        pread(fd,&tmp,128,leafpage_offset+(j*128));
        pwrite(fd,&tmp,128,leafpage_offset+(j-1)*128);
    }
    printf("leafpage_offset : %ld\n",leafpage_offset);
    set_number_of_keys(leafpage_offset,number_of_key-1);
    int64_t sibling;
    merge_and_Redistribution_leaf(leafpage_offset);
    free(arr);
    return 0;
}

void print_leaf()
{
    int64_t now=4096;
    int64_t next=0;
    int64_t key;
    int number;
    int64_t parrent;
    int64_t num;
    pread(fd,&num,8,16);
    while(num>0)
    {
        num--;
        pread(fd,&parrent,8,now);
        printf("parrent : %ld\t",(parrent/4096));
        number=number_of_keys(now);
        for(int i=1;i<=number;i++)
        {
            pread(fd,&key,8,now+i*128);
            if(key!=0) printf("%ld ",key);
        }
        printf("|\n");
        pread(fd,&next,8,now+120);
        //printf("in print : now :%ld next : %ld return : %d \n", now,next,pread(fd,&next,8,now+120));
        now=next;
    }
}

void page_layout()
{
    int64_t number_of_pages;
    pread(fd,&number_of_pages,8,16);
    for(int i=1;i<number_of_pages;i++)
    {
        if(number_of_keys(i*4096)==0) printf("[%d] %d : freepage\n",i,i*4096);
        else
        {
            if(isleaf(i*4096)==1)
            {
                printf("[%d] %d : leafpage\n",i,i*4096);    
             }
            else
            {
                printf("[%d] %d : internalpage\n",i,i*4096);
            }
        }
    }

}   

void printf_page(int num)
{
    int64_t offset = num*4096;
    int64_t key;
    if(isleaf(offset)==1)
    {
        int64_t number=number_of_keys(offset);
        for(int i=1;i<=number;i++)
        {
            //printf("is leaf\n");
            pread(fd,&key,8,offset+i*128);
            if(key!=0) printf("%ld ",key);
        }
        printf("\n");

    }
    else
    {
         int64_t number=number_of_keys(offset);
        for(int i=1;i<=number;i++)
        {
            //printf("is internal\n");
            pread(fd,&key,8,offset+112+i*16);
            printf("%ld ",key);
        }
        printf("\n");
    }
}

int parentwho(int num)
{
    int64_t parentoffset;
    pread(fd,&parentoffset,8,num*4096);
    return (int)(parentoffset/4096);
}
