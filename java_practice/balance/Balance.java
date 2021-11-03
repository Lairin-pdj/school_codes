//
// JAVA
// class 
// make by Park Dongjun
//

package com.company;

public class Balance {
    static int balance = 0;

    public static int checkBalance(){
        return balance;
    }
    public static int inputBalance(int num){
        if(num < 0)
            return -1;
        balance += num;
        return num;
    }
    public static int outputBalance(int num){
        if(num > balance)
            return -1;
        balance -= num;
        return num;
    }
}
