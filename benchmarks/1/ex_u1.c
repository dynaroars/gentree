/*
  Example
  7 variables: s,t,u,v,x,y,z 
  7 locs: L0,L1,L2,L2a,L3,L4,L5

  Consists of unsupported interactions
*/

#include <stdlib.h>
#include <stdio.h>
int main(int argc, char **argv){

  int s = atoi(argv[1]);
  int t = atoi(argv[2]);
  int u = atoi(argv[3]);
  int v = atoi(argv[4]);  

  int x = atoi(argv[5]);
  int y = atoi(argv[6]);
  int z = atoi(argv[7]);
  
  if ((u||v)){
    if (x||y){
      printf("L3\n");
    }
  }

  return 0;
}
  


