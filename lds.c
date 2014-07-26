/*
 * TODO: 2011/10/03 Remove same node from both source and target and output source and target
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>

#define DEBUG_TRACE 1  /* 0 - disable trace, 1 - normal trace, 2 - full trace */ 
#define MAX_PATH_LEN 512
#define MAX_NAME_LEN 64

#ifndef FILESYSTEM_PREFIX_LEN
# define FILESYSTEM_PREFIX_LEN(Filename) 0
#endif

#ifndef ISSLASH
# define ISSLASH(C) ((C) == '/')
#endif

#define CREATE_NODE(fin) {fin = malloc(sizeof(FILE_INFO_NODE));\
                          fin->fpath = malloc(MAX_PATH_LEN+1);\
                          fin->fname = malloc(MAX_NAME_LEN+1);\
                          memset(fin->fpath,0x0,MAX_PATH_LEN+1);\
                          memset(fin->fname,0x0,MAX_NAME_LEN+1);\
                          fin->next  = NULL; }

typedef struct FILE_INFO_NODE {
    char * fpath;
    char * fname;
    struct FILE_INFO_NODE * next;
}FILE_INFO_NODE;

/* Strip directory and suffix from filenames */
char * ft_basename (char const *fpath)
{
  char const *base = fpath += FILESYSTEM_PREFIX_LEN (fpath);
  int all_slashes = 1;
  char const *p;

  for (p = fpath; *p; p++) {
      if (ISSLASH (*p)) {
        base = p + 1;
      } else {
        all_slashes = 0;
      }
  }

  /* If NAME is all slashes, arrange to return `/'.  */
  if (*base == '\0' && ISSLASH (*fpath) && all_slashes) {
    --base;
  }

  return (char *) base;
}

/* gou through the link and output all elementes */
void matrix_dump_link(FILE_INFO_NODE * fin_header)
{
#if DEBUG_TRACE == 2
    FILE_INFO_NODE * fin_temp;
    unsigned int index = 0;

    fprintf(stdout,"DUMP LINKER....\n");
    while (fin_header != NULL) {
        fin_temp   = fin_header;
        fin_header = fin_header->next;
        fprintf(stdout,"No%d=%s",index++,fin_temp->fname);
    }

    fprintf(stdout,"DUMP LINKER....DONE\n");
#endif 
}

/* merge source and target by remove same/common node from target and source after done
 */
void matrix_difference_ex(FILE_INFO_NODE **mx_source,  /* in,out */
                          FILE_INFO_NODE **mx_target)  /* in,out */
{
    char letter = 'a';
    unsigned int index;

    FILE_INFO_NODE * fin_mx_source_cursor,* fin_mx_target_cursor,* fin_mx_target_tmp;
    FILE_INFO_NODE * fin_temp;

    char * p1, * p2;
    unsigned int is_same;

    assert(mx_source != NULL);
    assert(mx_target != NULL);

    /* go through mx_source and:
     * - if source is empty and target is what we want
     * - if source is NOT empty but target is empty,then move source to target
     * - if source and target are NOT empty, then difference them - remove same/common node from target
     * */
    while (letter <= 'z') {
       index = letter++ - 'a';

#if DEBUG_TRACE == 1 || DEBUG_TRACE == 2
       fprintf(stdout, "mx_source[%d]=0x%p,mx_target[%d]=0x%p\n",index,mx_source[index],index,mx_target[index]); 
#endif
       if ((mx_source[index] == NULL && mx_target[index] == NULL) ||
           (mx_source[index] == NULL && mx_target[index] != NULL) ||
           (mx_source[index] != NULL && mx_target[index] == NULL)) 
       {
           continue;
       }

       fin_mx_source_cursor = mx_source[index];

       while (fin_mx_source_cursor != NULL) {
           matrix_dump_link(fin_mx_source_cursor); 
           
           fin_mx_target_cursor = mx_target[index];
           fin_mx_target_tmp    = mx_target[index]; /* point to the previous node for removing node purpose */
           while (fin_mx_target_cursor != NULL) {
               matrix_dump_link(fin_mx_target_cursor);

               p1 = fin_mx_source_cursor->fname;
               p2 = fin_mx_target_cursor->fname;

               is_same = 1;

               /* check if the file name including path is same */
               while (p1 != NULL && *p1 != '\0' && *p1 != '.') {
                   if (*p1 - *p2 != 0) {
                       is_same = 0;
                       break;
                   }

                   p1++;
                   p2++;
               }

               /* if we find same node in source and target... */
               if( is_same ) {

                   fin_temp  = fin_mx_target_cursor;

                   /* dislink the node from link */
                   if (fin_mx_target_tmp == fin_mx_target_cursor) {
                       /* remove header */
                       fin_mx_target_cursor = fin_mx_target_cursor->next;
                       fin_mx_target_tmp    = fin_mx_target_cursor;
                       mx_target[index]     = fin_mx_target_cursor;
                   } else if (fin_mx_target_cursor->next == NULL) {
                       /* remove tailer */
                       fin_mx_target_tmp->next = NULL;
                   } else {
                       /* remove middle one */
                       fin_mx_target_tmp->next = fin_mx_target_cursor->next;
                       fin_mx_target_cursor    = fin_mx_target_cursor->next;
                   }

#if DEBUG_TRACE == 1 || DEBUG_TRACE == 2
                   fprintf(stdout, "REMOVE %s\n",fin_temp->fname); 
#endif
                   free(fin_temp->fpath);
                   free(fin_temp->fname);
                   free(fin_temp);
               } else {

                   /* remember current position and move cursor to the next */
                   fin_mx_target_tmp    = fin_mx_target_cursor;
                   fin_mx_target_cursor = fin_mx_target_cursor->next;
               }
          }

          fin_mx_source_cursor = fin_mx_source_cursor->next;
       }

    }
}

/* merge source and target by remove same/common node from target
   after done, the target will keep the difference ones
 */
void matrix_difference(FILE_INFO_NODE **mx_source,  /* in     */
                       FILE_INFO_NODE **mx_target)  /* in,out */
{
    char letter = 'a';
    unsigned int index;

    FILE_INFO_NODE * fin_mx_source_cursor,* fin_mx_target_cursor,* fin_mx_target_tmp;
    FILE_INFO_NODE * fin_temp;

    char * p1, * p2;
    unsigned int is_same;

    assert(mx_source != NULL);
    assert(mx_target != NULL);

    /* go through mx_source and:
     * - if source is empty and target is what we want
     * - if source is NOT empty but target is empty,then move source to target
     * - if source and target are NOT empty, then difference them - remove same/common node from target
     * */
    while (letter <= 'z') {
       index = letter++ - 'a';

#if DEBUG_TRACE == 1 || DEBUG_TRACE == 2
       fprintf(stdout, "mx_source[%d]=0x%p,mx_target[%d]=0x%p\n",index,mx_source[index],index,mx_target[index]); 
#endif
       if ((mx_source[index] == NULL && mx_target[index] == NULL) ||
           (mx_source[index] == NULL && mx_target[index] != NULL) ||
           (mx_source[index] != NULL && mx_target[index] == NULL)) 
       {
           continue;
       }

       fin_mx_source_cursor = mx_source[index];

       while (fin_mx_source_cursor != NULL) {
           matrix_dump_link(fin_mx_source_cursor); 
           
           fin_mx_target_cursor = mx_target[index];
           fin_mx_target_tmp    = mx_target[index]; /* point to the previous node for removing node purpose */
           while (fin_mx_target_cursor != NULL) {
               matrix_dump_link(fin_mx_target_cursor);

               p1 = fin_mx_source_cursor->fname;
               p2 = fin_mx_target_cursor->fname;

               is_same = 1;

               /* check if the file name including path is same */
               while (p1 != NULL && *p1 != '\0' && *p1 != '.') {
                   if (*p1 - *p2 != 0) {
                       is_same = 0;
                       break;
                   }

                   p1++;
                   p2++;
               }

               /* if we find same node in source and target... */
               if( is_same ) {

                   fin_temp  = fin_mx_target_cursor;

                   /* dislink the node from link */
                   if (fin_mx_target_tmp == fin_mx_target_cursor) {
                       /* remove header */
                       fin_mx_target_cursor = fin_mx_target_cursor->next;
                       fin_mx_target_tmp    = fin_mx_target_cursor;
                       mx_target[index]     = fin_mx_target_cursor;
                   } else if (fin_mx_target_cursor->next == NULL) {
                       /* remove tailer */
                       fin_mx_target_tmp->next = NULL;
                   } else {
                       /* remove middle one */
                       fin_mx_target_tmp->next = fin_mx_target_cursor->next;
                       fin_mx_target_cursor    = fin_mx_target_cursor->next;
                   }

#if DEBUG_TRACE == 1 || DEBUG_TRACE == 2
                   fprintf(stdout, "REMOVE %s\n",fin_temp->fname); 
#endif
                   free(fin_temp->fpath);
                   free(fin_temp->fname);
                   free(fin_temp);
               } else {

                   /* remember current position and move cursor to the next */
                   fin_mx_target_tmp    = fin_mx_target_cursor;
                   fin_mx_target_cursor = fin_mx_target_cursor->next;
               }
          }

          fin_mx_source_cursor = fin_mx_source_cursor->next;
       }

    }
}

/* free all nodes linked to matrix */
void matrix_free(FILE_INFO_NODE ** mx)
{
    char letter = 'a';
    FILE_INFO_NODE * fin_header, * fin_temp;

    assert(mx != NULL);

    while (letter <= 'z') {

        fin_header = (FILE_INFO_NODE *)mx[letter++ - 'a'];
        while (fin_header != NULL) {
            fin_temp   = fin_header;
            fin_header = fin_header->next;
            free(fin_temp->fpath);
            free(fin_temp->fname);
        }
    }

    free(*mx);
}

/* present the result - the nodes not in source but in target only */
void matrix_dump(FILE_INFO_NODE ** mx)
{
    unsigned int index = 1;
    FILE_INFO_NODE * fin_header, * fin_temp;
    char letter = 'a';

    fprintf(stdout,"\r\n");
    fprintf(stdout,"\r\n");
    fprintf(stdout,".................................................DIFFERENCE FILE LIST.............................................................\n");
    
    while (letter <= 'z') {

        fin_header = (FILE_INFO_NODE *)mx[letter++ - 'a'];
        while (fin_header != NULL) {
            fin_temp   = fin_header;
            fin_header = fin_header->next;
            fprintf(stdout,"No%d,path=%s",index++,fin_temp->fpath);
        }
    }

    if(index == 1) {
        fprintf(stdout,"The target list is EMPTY\n");
    }
    
    fprintf(stdout,".................................................DIFFERENCE FILE LIST.............................................................\n");
    fprintf(stdout,"\r\n");
    fprintf(stdout,"\r\n");
}

/*
    The input file is a file that contains file full path for each line,like
      /home/user/src/abc.c
      /home/user/src/adf.c
      /home/user/src/afe.c
      /home/user/src/doom.c
      /home/user/src/elf.c
      /home/user/src/eep.c
      /home/user/src/woman.c
      /home/user/src/what.c
 
    The matrix is a 2-D pointer array that contains file infor(name,path) according to a-z order,like
           
      'a' abc.c--->adf.c--->afe.c--->NULL
           | 
      'b' NULL
           |
      'c' NULL
           |
      'd' doom.c--->NULL
           |
      'e' elf.c--->eep.c--->NULL
           | 
      'f' NULL
           |
          ...
           |
      'w' woman.c--->what.c--->NULL
           |
          ...
      'z' NULL

 */
void matrix_get(char * file,              /* in,input file path */
                FILE_INFO_NODE ** matrix) /* out,matrix */
{
    FILE * fd;
    FILE_INFO_NODE * fin_temp;
    FILE_INFO_NODE * fin_cursors[26];
    unsigned int index;
    int c;

    if ((fd = fopen(file,"r")) == 0) {
        fprintf(stderr,"Read file(%s) failed:%s\n",file,strerror(errno));
        exit(0);
    }

    /* create header node */
    CREATE_NODE(fin_temp); 

    /* go through file line by line and build a matrix to save sort node link */
    while (fgets((char *)fin_temp->fpath, MAX_PATH_LEN, fd) != 0) {
        strncpy(fin_temp->fname,ft_basename(fin_temp->fpath),MAX_NAME_LEN);

#if DEBUG_TRACE == 2
        fprintf(stdout, "fname is %s",fin_temp->fname);
#endif  
        c = (int)fin_temp->fname[0];
        index = tolower(c) - 'a';
        if (index > 'z') { /* a source file beginning with a ? */
           fprintf(stderr, "Unsupport file name:%s\n",fin_temp->fname);
           continue;
        }

        if (matrix[index] == NULL) {

#if DEBUG_TRACE == 2
            fprintf(stdout, "add header node at matrix[%d]=0x%p\n",index,matrix[index]);
#endif            
            matrix[index]      = (FILE_INFO_NODE *)fin_temp;
            fin_cursors[index] = (FILE_INFO_NODE *)fin_temp;
        } else {
#if DEBUG_TRACE == 2
            fprintf(stdout, "append node at matrix[%d]=0x%p,cursors[%d]=0x%p\n\n",index,matrix[index],index,fin_cursors[index]);
#endif            
            fin_cursors[index]->next = (FILE_INFO_NODE *)fin_temp;
            fin_cursors[index]       = (FILE_INFO_NODE *)fin_temp;
        }

        /* create new node... */
        CREATE_NODE(fin_temp);
    }

    fclose(fd);

    /*
#if DEBUG_TRACE == 1
    matrix_dump(matrix);
#endif
    */
    
}

int main(int argc, char * argv[])
{
    char cmdStr[MAX_PATH_LEN] = {0};
    char allready = 1;
    clock_t start_time, end_time;

    FILE_INFO_NODE * matrix_source[26] = { NULL };
    FILE_INFO_NODE * matrix_refer[26]  = { NULL };

    char refer_file_list[MAX_PATH_LEN]  = "refer_file_list";
    char source_file_list[MAX_PATH_LEN] = "source_file_list";

#if DEBUG_TRACE == 2
         {
             unsigned short argNums = argc;
             fprintf(stdout, "argc=%d\n",argc);
             while (argNums != 0) {
                 fprintf(stdout, "argv[%d]=%s\n",argNums,argv[argNums]);
                 argNums--;
             }
         } 
#endif

    ////////////////////////////////////////////////////////////////////////
    switch(argc)
    {
       case 2:   /* e.g. $lsd --help */ 
         if (argv[1][0] == '-' && 
             argv[1][1] == '-' &&
             argv[1][2] == 'h' &&
             argv[1][3] == 'e' &&
             argv[1][4] == 'l' &&
             argv[1][5] == 'p') 
         {
             fprintf(stdout, "Usage: lsd [SOURCE_DIR] [SUFFIX] [REFERENCE_DIR] [SUFFIX]\n");
             fprintf(stdout, "  or:  lsd -c SOURCE_FILE REFERENCE_FILE\n");
             fprintf(stdout, "  or:  lsd -a SOURCE_DIR REFERENCE_DIR\n");
             fprintf(stdout, "Print file path of the file that in SOURCE DIR but not in REFERENCE DIR with same PREFIX and difference SUFFIX\n");
             fprintf(stdout, "\n");
             fprintf(stdout, "    -c  comparinf specify SOURCE and REFERENCE file instead of DIR\n");
             fprintf(stdout, "    -a  comparing all types of file in specify SOURCE and REFERENCE directory\n");
             fprintf(stdout, "\n");
             fprintf(stdout, "For example:\n");
             fprintf(stdout, "   List file's path that in /home/src/cfiles/ but not in /home/src/objfiles/ with same PREFIX but difference SUFFIX.\n");
             fprintf(stdout, "   $lsd /home/src/cfiles/ .c /home/src/objfiles/ .obj\n");
             fprintf(stdout, "\n");
             fprintf(stdout, "   If already has SOURCE or REFERENCE file generated by,for example\n");
             fprintf(stdout, "       '$ls -name '*.c' > source_file_list'\n");
             fprintf(stdout, "   and '$ls -name '*.obj' > refer_file_list'\n");
             fprintf(stdout, "   then using '-c' option directly,as below:\n");
             fprintf(stdout, "   $lsd -c source_file_list refer_list_file\n");
         } else {
             goto MISSING_FILE_OPERAND;
         }
         allready = 0;
         break;
       case 4:   /* e.g.   $lsd -c source_file_list target_file_list */
                 /*     or $lsd -a source_path target_path           */
         if (argv[1][0] == '-' && argv[1][1] =='c') {
             fprintf(stdout,"Running...\n");
             strncpy(source_file_list,argv[2],MAX_PATH_LEN);
             strncpy(refer_file_list,argv[3],MAX_PATH_LEN);
         } else if (argv[1][0] == '-' && argv[1][1] =='a') {
             fprintf(stdout,"Running...\n");
             fprintf(stdout,"Generating source file list...\n");

             /* find all files but ignore hidden file and directory */
             sprintf(cmdStr,"find %s \\( ! -regex '.*/\\..*' \\) -xtype f -name '*' > source_file_list", argv[2]);
             printf("Generating source_file_list:\n");
             printf("   %s",cmdStr);
             if (system(cmdStr) != 0) {
                 fprintf(stderr, "Can not get source file list, please check your shell or path\n");
                 allready = 0;
                 break;
             } else {
                 fprintf(stdout,"Generating c file list...DONE\n");
             }

             fprintf(stdout,"Generating reference file list...\n");
             sprintf(cmdStr,"find %s \\( ! -regex '.*/\\..*' \\) -xtype f -name '*' > refer_file_list", argv[3]);
             printf("Generating refer_file_list:\n");
             printf("   %s",cmdStr);
             if (system(cmdStr) != 0) {
                 fprintf(stderr, "Can not get reference file list, please check your shell or path\n");
                 allready = 0;
                 break;
             } else {
                 fprintf(stdout,"Generating reference file list...DONE\n");
             }

         } else {
            goto MISSING_FILE_OPERAND;
         }
         break;
       case 5:   /* e.g. $lsd source_path .obj target_path .c */
         fprintf(stdout,"Running...\n");
         fprintf(stdout,"Generating source file list...\n");
         sprintf(cmdStr,"find %s \\( ! -regex '.*/\\..*' \\) -xtype f -name '*%s' > source_file_list", argv[1], argv[2]);
         if (system(cmdStr) != 0) {
             fprintf(stderr, "Can not get source file list, please check your shell or path\n");
             allready = 0;
             break;
         } else {
             fprintf(stdout,"Generating c file list...DONE\n");
         }

         fprintf(stdout,"Generating reference file list...\n");
         sprintf(cmdStr,"find %s \\( ! -regex '.*/\\..*' \\) -xtype f -name '*%s' > refer_file_list", argv[3], argv[4]);
         if (system(cmdStr) != 0) {
             fprintf(stderr, "Can not get reference file list, please check your shell or path\n");
             allready = 0;
             break;
         } else {
             fprintf(stdout,"Generating reference file list...DONE\n");
         }
         break;
       default:
MISSING_FILE_OPERAND:
         fprintf(stdout, "lsd: missing file operand\n");
         fprintf(stdout, "Try `lsd --help' for more information.\n");
         allready = 0;
         break;
    }

    ////////////////////////////////////////////////////////////////////////

    if (allready) {

        start_time = clock();

        matrix_get(refer_file_list,matrix_refer);
        matrix_get(source_file_list,matrix_source);

#if DEBUG_TRACE == 1 || DEBUG_TRACE == 2
        matrix_dump(matrix_source);
#endif

        matrix_difference(matrix_refer,matrix_source);
        matrix_dump(matrix_source);

        end_time = clock();
        fprintf(stdout,"Time consumation is %.4f seconds\n",(end_time - start_time)/(double)CLOCKS_PER_SEC);
    }

    return 0;
}
