#include <gtest/gtest.h>
#include <iostream>

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Set up test environment
    std::cout << "Running update-agent tests..." << std::endl;
    
    int result = RUN_ALL_TESTS();
    
    std::cout << "Test execution completed." << std::endl;
    return result;
}
