#import <Foundation/Foundation.h>

#include <iostream>

int main(int argc, const char * argv[]) {
    try {
      @autoreleasepool {
          // insert code here...
        if(argc != 3) {
            std::cerr << "Invalid args" << std::endl;
            return 1;
        }

        NSLog(@"Hello, World!");
      }
    }
    catch (...) {
    }

    return 0;
}
