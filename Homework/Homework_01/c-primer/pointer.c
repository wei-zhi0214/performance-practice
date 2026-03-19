// Copyright (c) 2012 MIT License by 6.172 Staff

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char * argv[]) {  // What is the type of argv?
  int i = 5;
  // The & operator here gets the address of i and stores it into pi
  int * pi = &i;
  // The * operator here dereferences pi and stores the value -- 5 --
  // into j.
  int j = *pi;

  char c[] = "6.172";
  char * pc = c;  // Valid assignment: c acts like a pointer to c[0] here.
  char d = *pc;
  printf("char d = %c\n", d);  // What does this print?

  // compound types are read right to left in C.
  // pcp is a pointer to a pointer to a char, meaning that
  // pcp stores the address of a char pointer.
  char ** pcp;
  pcp = argv;  // Why is this assignment valid?

  const char * pcc = c;  // pcc is a pointer to char constant
  char const * pcc2 = c;  // What is the type of pcc2?

  // For each of the following, why is the assignment:
  *pcc = '7';  // invalid? pcc is const pointer to char, so *pcc is a char that cannot be modified.
  pcc = *pcp;  // valid?   pcp is a pointer to a pointer to char, so *pcp is a pointer to char, which can be assigned to pcc.
  pcc = argv[0];  // valid? argv is an array of pointers to char, so argv[0] is a pointer to char, which can be assigned to pcc.

  char * const cp = c;  // cp is a const pointer to char
  // For each of the following, why is the assignment: 
  cp = *pcp;  // invalid? cp is a const pointer to char, so cp itself cannot be modified to point to something else.
  cp = *argv;  // invalid?  argv is an array of pointers to char, so *argv is a pointer to char, which cannot be assigned to cp because cp is a const pointer.
  *cp = '!';  // valid? cp is a const pointer to char, so *cp is a char that can be modified.

  const char * const cpc = c;  // cpc is a const pointer to char const
  // For each of the following, why is the assignment:
  cpc = *pcp;  // invalid?  cpc is a const pointer to char const, so cpc itself cannot be modified to point to something else.
  cpc = argv[0];  // invalid? argv is an array of pointers to char, so argv[0] is a pointer to char, which cannot be assigned to cpc because cpc is a const pointer.
  *cpc = '@';  // invalid? cpc is a const pointer to char const, so *cpc is a char constant that cannot be modified.

  return 0;
}
