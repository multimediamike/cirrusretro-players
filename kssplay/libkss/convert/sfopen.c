#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * Open a file from the given path.
 *
 */
FILE *pfopen(const char *file, const char *mode, const char *path)
{
  char *buf;
  FILE *fp;

  buf = malloc(strlen(file)+strlen(path)+1);
  strcpy(buf,path);
  strcat(buf,file);
  fp = fopen(buf,mode);
  free(buf);
  return fp;
}

/*
 * Search a file from the given paths.
 * path is a list of folders which are separated with ';'.
 * Ex. "D:\WINDOWS\;Q:\DRIVER"
 *
 */
FILE *sfopen(const char *file, const char *mode, const char *path)
{
  FILE *fp = NULL;
  char *buf, *tok;

  buf = malloc(strlen(path)+1);
  strcpy(buf,path);
  tok = strtok(buf,";");

  while(tok)
  {
    if(fp=pfopen(file,mode,tok)) break;    
    strtok(buf,";");
  }

  free(buf);
  return fp;
}
