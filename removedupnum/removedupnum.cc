#include <iostream>

int removeDuplicates(int A[], int n) {
    if(n <= 0){
        return 0;
    }
   int pos = 0;
   A[pos] = A[0];
   for(int i = 1;i<n;++i){
       if(A[i] == A[pos]){
           continue;
       }else {
           A[++pos] = A[i];
       }
   }
   return pos+1;
}

int main()
{
    int a[] = {1,1,2};
    int ret = removeDuplicates(a,3);
    std::cout<<ret<<std::endl;
    int i = 0;
    for(;i<(int)(sizeof(a)/sizeof(a[0]));++i){
        std::cout<<a[i];
    }
    std::cout<<std::endl;
}

