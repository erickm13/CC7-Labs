#include "../lib/stdio.h"

void main() {

  int num1, num2, sum;
  float float1,float2, sum2;
  char str[32];

  // Display the program title
  PRINT("Program: Add Two Numbers\n");

  while (1) {

    PRINT("------------------------------\n");

    // Prompt for the first number
    PRINT("Enter first number: ");
    READ("%d", &num1);

    // Prompt for the second number
    PRINT("Enter second number: ");
    READ("%d", &num2);

    // Calculate the sum
    sum = num1 + num2;

    // Display the result
    PRINT("%d + %d = %d \n", num1, num2, sum);

    // Prompt for the first number
    PRINT("Enter first float number: ");
    READ("%f", &float1);

    // Prompt for the second number
    PRINT("Enter second float number: ");
    READ("%f", &float2);

    // Calculate the sum
    sum2 = float1 + float2;

    // Display the result
    PRINT("%f + %f = %f \n", float1, float2, sum2);
  }
}