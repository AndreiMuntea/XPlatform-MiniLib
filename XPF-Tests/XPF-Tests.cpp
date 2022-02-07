// X-Platform.cpp : Defines the entry point for the application.
//

#include "XPF-Tests.h"
using namespace std;


int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}