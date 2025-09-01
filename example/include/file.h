#ifndef FILE_H_
#define FILE_H_

/**
 * This is an example structure
 */
struct MyStruct {
  int a;
};

/**
 * This is another example structure used to demonstrate cross-references
 * between types
 */
struct OtherStruct {
  MyStruct b; /**< Something */
};

/**
 * A function that does nothing
 * - `a`: First parameter
 * - `b`: Second parameter
 */
void function(int a, struct OtherStruct b);

#endif