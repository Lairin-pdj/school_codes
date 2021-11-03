//
// JAVA
// balance using main 
// make by Park Dongjun
//

package com.company;

import java.util.Scanner;

public class Main {
    public static void main(String[] args) {
        Scanner sc = new Scanner(System.in);
        int i, num;
        boolean check = true;
        Balance test = new Balance();
        System.out.println("BALANCE SYSTEM");

        while (check) {
            System.out.println("\n\tMENU");
            System.out.println("1 : CHECK BALANCE");
            System.out.println("2 : INPUT BALANCE");
            System.out.println("3 : OUTPUT BALANCE");
            System.out.println("4 : EXIT");
            System.out.print("SELECT >> ");

            try {
                i = sc.nextInt();
                if (i < 0 || i > 4) {
                    System.out.println("out of range");
                    continue;
                }
            } catch (Exception e) {
                System.out.println("can't use word");
                sc.next();
                continue;
            }

            int temp;
            switch (i) {
                case 1:
                    System.out.println("NOW BALANCE : " + test.checkBalance());
                    break;
                case 2:
                    try {
                        System.out.print("INPUT : ");
                        num = sc.nextInt();
                    } catch (Exception e) {
                        System.out.println("is not a number");
                        sc.next();
                        break;
                    }
                    temp = test.inputBalance(num);
                    if(temp >= 0)
                        System.out.println("add : " + temp);
                    else
                        System.out.println("error : can't input negative");
                    break;
                case 3:
                    try {
                        System.out.print("OUTPUT : ");
                        num = sc.nextInt();
                    } catch (Exception e) {
                        System.out.println("is not a number");
                        sc.next();
                        break;
                    }
                    temp = test.outputBalance(num);
                    if(temp >= 0)
                        System.out.println("sub : " + temp);
                    else
                        System.out.println("error : over the balance");
                    break;
                case 4:
                    check = false;
                    break;
                default:
                    break;
            }
        }
        System.out.println("GOOD BYE!");
    }
}
