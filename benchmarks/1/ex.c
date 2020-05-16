/*
  Example
  7 variables: s,t,u,v,x,y,z 
  7 locs: L0,L1,L2,L2a,L3,L4,L5

  iGen results
  1. (0) true (conj): (1) L3
  2. (2) (u=1 & v=1) (conj): (1) L4
  3. (2) (x=1 & y=1) (conj): (1) L0
  4. (2) (x=0 | y=0) (disj): (2) L2,L2a
  5. (3) (x=1 & y=1 & z=0,3,4) (conj): (1) L1
  6. (4) (s=1 | t=1) & (u=1 & v=1) (mix): (1) L5
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
  
  int max_z = 3;
  
  if (x&&y){
    printf("L0\n"); //x & y
    if (!(0 < z && z < max_z)){
      printf("L1\n"); //x & y & (z=0|3|4)
    }
  }
  else{
    printf("L2\n"); // !x|!y
    printf("L2a\n"); // !x|!y    
  }

  printf("L3\n"); // true
  if(u&&v){
    printf("L4\n"); //u&v
    if(s||t){
	 printf("L5\n");  // (s|t) & (u&v)
    }
  }

  return 0;
}
  


