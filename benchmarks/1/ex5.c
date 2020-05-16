#include <stdio.h>
#include <stdlib.h>

int main(int argc,char **argv){

  //config vars
  int a = atoi(argv[1]); 
  int b = atoi(argv[2]);
  int c = atoi(argv[3]);
  int d = atoi(argv[4]);
  int e = atoi(argv[5]);

  printf("LS\n");
  if(a==1){
    printf("a1\n");
    if (b==1){
      printf("a1b1\n");
      if (c==1){
	printf("a1b1c1\n");
	if (d==1){
	  printf("a1b1c1d1\n");
	  if (e==1){
	    printf("a1b1c1d1e1\n");
	  }
	  else{
	    printf("a1b1c1d1e0\n");
	  }
	}
	else{    
	  printf("a1b1c1d0\n");
	  if (e==1){
	    printf("a1b1c1d0e1\n");
	  }
	  else{
	    printf("a1b1c1d0e0\n");
	  }
	}	  
      }
      else{
	printf("a1b1c0\n");
	if (d==1){
	  printf("a1b1c0d1\n");
	  if (e==1){
	    printf("a1b1c0d1e1\n");
	  }
	  else{
	    printf("a1b1c0d1e0\n");
	  }
	}
	else{    
	  printf("a1b1c0d0\n");
	  if (e==1){
	    printf("a1b1c0d0e1\n");
	  }
	  else{
	    printf("a1b1c0d0e0\n");
	  }
	}
	
      }
    }
    else{
      printf("a1b0\n");
      if (c==1){
	printf("a1b0c1\n");
	if (d==1){
	  printf("a1b0c1d1\n");
	  if (e==1){
	    printf("a1b0c1d1e1\n");
	  }
	  else{
	    printf("a1b0c1d1e0\n");
	  }
	}
	else{    
	  printf("a1b0c1d0\n");
	  if (e==1){
	    printf("a1b0c1d0e1\n");
	  }
	  else{
	    printf("a1b0c1d0e0\n");
	  }
	}
	
      }
      else{
	printf("a1b0c0\n");
	if (d==1){
	  printf("a1b0c0d1\n");
	  if (e==1){
	    printf("a1b0c0d1e1\n");
	  }
	  else{
	    printf("a1b0c0d1e0\n");
	  }
	}
	else{    
	  printf("a1b0c0d0\n");
	  if (e==1){
	    printf("a1b0c0d0e1\n");
	  }
	  else{
	    printf("a1b0c0d0e0\n");
	  }
	}
	
      }      
    }
  }

  else{
    printf("a0\n");
    if(b==1){
      printf("a0b1\n");
      if (c==1){
	printf("a0b1c1\n");
	if (d==1){
	  printf("a0b1c1d1\n");
	  if (e==1){
	    printf("a0b1c1d1e1\n");
	  }
	  else{
	    printf("a0b1c1d1e0\n");
	  }
	}
	else{    
	  printf("a0b1c1d0\n");
	  if (e==1){
	    printf("a0b1c1d0e1\n");
	  }
	  else{
	    printf("a0b1c1d0e0\n");
	  }
	}
	
      }
      else{
	printf("a0b1c0\n");
	if (d==1){
	  printf("a0b1c0d1\n");
	  if (e==1){
	    printf("a0b1c0d1e1\n");
	  }
	  else{
	    printf("a0b1c0d1e0\n");
	  }
	}
	else{    
	  printf("a0b1c0d0\n");
	  if (e==1){
	    printf("a0b1c0d0e1\n");
	  }
	  else{
	    printf("a0b1c0d0e0\n");
	  }
	}
	
      }
    }
    else{
      printf("a0b0\n");
      if (c==1){
	printf("a0b0c1\n");
	if (d==1){
	  printf("a0b0c1d1\n");
	  if (e==1){
	    printf("a0b0c1d1e1\n");
	  }
	  else{
	    printf("a0b0c1d1e0\n");
	  }
	}
	else{    
	  printf("a0b0c1d0\n");
	  if (e==1){
	    printf("a0b0c1d0e1\n");
	  }
	  else{
	    printf("a0b0c1d0e0\n");
	  }
	}
	
      }
      else{
	printf("a0b0c0\n");
	if (d==1){
	  printf("a0b0c0d1\n");
	  if (e==1){
	    printf("a0b0c0d1e1\n");
	  }
	  else{
	    printf("a0b0c0d1e0\n");
	  }
	}
	else{    
	  printf("a0b0c0d0\n");
	  if (e==1){
	    printf("a0b0c0d0e1\n");
	  }
	  else{
	    printf("a0b0c0d0e0\n");
	  }
	}
	
      }      
    }
  }
  return 0;
}

