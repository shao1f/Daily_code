#include <iostream>
#include <string>
#include <algorithm>

class Solution {
public:
    void conversion_of_Num(int n,int scale){
        int flag = 1;//看是不是负数
        if(n < 0){
            flag = -1;
        }
        char m = 0;//保存每一位
        std::string s;//保存结果
        n = abs(n);
        while(1){
            m = n % scale;
            if(m >= 10){
                m = 'A' + (m-10);
            }else {
                m += '0';
            }
            s += m;
            n /= scale;
            if(n == 0){
                break;
            }
        }
        if(flag == -1){
            s += '-';
        }
        std::reverse(s.begin(),s.end());
        std::cout<<s<<std::endl;
    }
};


int main()
{
    int n,m;
    while(std::cin>>n>>m){
        Solution s;
        s.conversion_of_Num(n,m);
    }
    return 0;
}

