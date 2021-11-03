package com.company;

import java.util.ArrayList;
import java.util.Random;
import java.util.Scanner;

public class Main {
    public static void main(String[] args) {
        Scanner sc = new Scanner(System.in);
        int win1 = 0;
        int win2 = 0;
        int win3 = 0;
        int win4 = 0;
        int win5 = 0;
        int total = 0;

        System.out.println("LOTTO 45");
        System.out.println("1 : auto");
        System.out.println("2 : manual");
        System.out.println("3 : info");
        System.out.println("4 : exit");

        while(true) {
            Random rand = new Random();
            int matchNum = 0;
            int matchBonus = 0;
            ArrayList<Integer> a = new ArrayList<>(6);
            ArrayList<Integer> ans = new ArrayList<>(6);
            int ansBonus = 0;

            //take user number
            try {
                System.out.print("choose a menu >> ");
                int d = sc.nextInt();
                System.out.println();
                if (d == 2) {
                    System.out.println("put 6 numbers, do not overlap");
                    System.out.print(">> ");
                    for (int i = 0; i < 6; i++) {
                        a.add(sc.nextInt());
                    }
                    System.out.println();
                } else if (d == 1) {
                    System.out.println("use auto");
                    for (int i = 0; i < 6; i++) {
                        boolean check = true;
                        while (check) {
                            int temp = rand.nextInt(45) + 1;
                            check = false;
                            for (int j = 0; j < i; j++) {
                                if (a.get(j) == temp) {
                                    check = true;
                                    break;
                                }
                            }
                            if (!check) {
                                a.add(temp);
                            }
                        }
                    }
                } else if (d == 4) {
                    break;
                } else if (d == 3){
                    System.out.println("total : " + total);
                    System.out.println("1st : " + win1);
                    System.out.println("2nd : " + win2);
                    System.out.println("3rd : " + win3);
                    System.out.println("4th : " + win4);
                    System.out.println("5th : " + win5);
                    long sum = (total * -1000) + (win5 * 5000) + (win4 * 50000) + (win3 * 5000000) + (win2 * 50000000) + (win1 * 1000000000);
                    System.out.println("money : " + sum + " won");
                    System.out.println();
                    continue;
                }
                else {
                    System.out.println("is not correct number");
                    System.out.println("so start 1 : auto");
                    d = 1;
                    for (int i = 0; i < 6; i++) {
                        boolean check = true;
                        while (check) {
                            int temp = rand.nextInt(45) + 1;
                            check = false;
                            for (int j = 0; j < i; j++) {
                                if (a.get(j) == temp) {
                                    check = true;
                                    break;
                                }
                            }
                            if (!check) {
                                a.add(temp);
                            }
                        }
                    }
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
            a.sort(null);

            //select 6 numbers
            for (int i = 0; i < 7; i++) {
                boolean check = true;
                while (check) {
                    int temp = rand.nextInt(45) + 1;
                    check = false;
                    for (int j = 0; j < i; j++) {
                        if (ans.get(j) == temp) {
                            check = true;
                            break;
                        }
                    }
                    if (i >= 6 && !check) {
                        ansBonus = temp;
                    } else if (!check) {
                        ans.add(temp);
                    }
                }
            }
            ans.sort(null);

            //checking how many numbers match
            for (int i = 0; i < 6; i++) {
                for (int j = 0; j < 6; j++) {
                    if (a.get(i) == ans.get(j)) {
                        matchNum++;
                        break;
                    }
                }
                if (a.get(i) == ansBonus) {
                    matchBonus++;
                }
            }

            //show result
            System.out.print("user  : ");
            for (int i = 0; i < 6; i++) {
                System.out.format("%02d ", a.get(i));
            }
            System.out.println();
            System.out.print("lotto : ");
            for (int i = 0; i < 6; i++) {
                System.out.format("%02d ", ans.get(i));
            }
            System.out.format("  Bonus : %02d \n", ansBonus);
            System.out.println("match : " + matchNum + "  bonus : " + matchBonus);
            if (matchNum == 6) {
                System.out.println(" 1st prize!");
                win1++;
            } else if (matchNum == 5) {
                if (matchBonus == 1) {
                    System.out.println(" 2nd prize!");
                    win2++;
                } else {
                    System.out.println(" 3rd prize!");
                    win3++;
                }
            } else if (matchNum == 4) {
                System.out.println(" 4th prize!");
                win4++;
            } else if (matchNum == 3) {
                System.out.println(" 5th prize!");
                win5++;
            } else {
                System.out.println(" you lose...");
            }
            total++;
            System.out.println();
        }
        System.out.println("GOOD BYE!");
    }
}
