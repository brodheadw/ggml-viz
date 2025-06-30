// simple_test.cpp - Simple program to test library injection
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    std::cout << "=== Simple Injection Test ===" << std::endl;
    std::cout << "This program will load any injected libraries..." << std::endl;
    
    // Give some time for auto-initialization
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "Program running for a moment to allow initialization..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    std::cout << "Exiting program..." << std::endl;
    return 0;
}