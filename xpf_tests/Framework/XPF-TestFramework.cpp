/**
 * @file        xpf_tests/Framework/XPF-TestFramework.cpp
 *
 * @brief       This file contains the definitions for the testing framework.
 *              googletest was not sufficient for us as we wanted to be able to
 *              run the tests in KM as well. This is a minimalistic framework.
 *
 * @note        This will allocate all run callbacks in a dedicated section.
 *              xpfts$t - this is a trick to allocate all callbacks in "a global" list
 *              without having to fill a huge array.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */



#include "xpf_tests/Framework/XPF-TestFramework.hpp"
#include "xpf_tests/XPF-TestIncludes.hpp"

 /**
  * @brief This can be paged. We won't run tests at IRQL DISPATCH.
  *        So simply page all code here.
  */
XPF_SECTION_PAGED;



/**
 * @brief       On Linux User Mode we'll register a signal handler.
 *              This will set this global variable to signal whether death was
 *              signaled or not.
 *              On Windows platforms this will be signaled from a SEH handler.
 *
 * @note        It's the responsibility of each test to reset this global properly
 *              before registering the signal handler.
 */
bool mXpfConditionHasGeneratedDeath = false;


_Must_inspect_result_
NTSTATUS XPF_API
xpf_test::RunAllTests(
    void
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Sanity check that xpfTsA, xpfTsT and xpfTsZ are somewhat valid.
    //
    // In this scenario something went wrong with the build.
    // Check the compile options and the above declaration.
    //
    if ((xpf::AlgoPointerToValue(&gXpfStartMarker) >= xpf::AlgoPointerToValue(&gXpfTestMarker)) ||
        (xpf::AlgoPointerToValue(&gXpfEndMarker) <= xpf::AlgoPointerToValue(&gXpfTestMarker)))
    {
        XPF_DEATH_ON_FAILURE(false);
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Now we can start executing tests.
    //
    uint64_t startDelta = xpf::ApiCurrentTime();
    uint64_t noTests = 0;
    uint64_t passedTests = 0;
    xpf_test::LogTestInfo("Starting test execution... \r\n");

    //
    // Now we can iterate over the section. Start at gXpfStartMarker and go until gXpfEndMarker.
    // We need to keep track of the padding added, so we skip nulls.
    //
    for (auto crtEntry = &gXpfStartMarker; crtEntry != &gXpfEndMarker; crtEntry++)
    {
        if (nullptr == (*crtEntry))
        {
            continue;
        }

        //
        // Start test execution.
        //
        xpf_test::LogTestInfo("\r\n[================================] \r\n");
        NTSTATUS testResult = STATUS_SUCCESS;

        uint64_t testStartTime = xpf::ApiCurrentTime();
        ((const xpf_test::TestScenarioCallback)(*crtEntry))(&testResult);                                              // NOLINT(*)
        uint64_t testEndTime = xpf::ApiCurrentTime();

        xpf_test::LogTestInfo("    > %llu (100 ns) test start time; \r\n",
                              static_cast<unsigned long long>(testStartTime));                                         // NOLINT(*)

        xpf_test::LogTestInfo("    > %llu (100 ns) test end time; \r\n",
                              static_cast<unsigned long long>(testEndTime));                                           // NOLINT(*)

        //
        // End test execution.
        //
        xpf::StringView<char> result = (NT_SUCCESS(testResult)) ? "SUCCESS"
                                                                : "FAILURE";
        xpf_test::LogTestInfo("[*] [%s] Test finished with status 0x%08x. Delta %llu (ms). \r\n",
                              result.Buffer(),
                              testResult,
                              static_cast<unsigned long long>(((testEndTime - testStartTime) / 10000)));                // NOLINT(*)
        xpf_test::LogTestInfo("[================================] \r\n");

        //
        // Count the success.
        //
        if (NT_SUCCESS(testResult))
        {
            passedTests++;
        }

        //
        // Successfully executed a test. Count it.
        //
        noTests++;
    }

    //
    // Ran all tests. They may have failed individually.
    // But we still return success.
    //
    uint64_t endDelta = xpf::ApiCurrentTime();
    xpf_test::LogTestInfo("\r\nFinished execution of %llu tests in %llu (ms).\r\n",
                          static_cast<unsigned long long>(noTests),                                                            // NOLINT(*)
                          static_cast<unsigned long long>((endDelta - startDelta) / 10000));                                   // NOLINT(*)

    //
    // Log statistics about failures. In windows KM it's a bit more
    // trickier to use floating point. This is just a statistics,
    // so to not complicate things we'll  take the precision loss
    // of using decimals to compute the percent.
    //
    xpf_test::LogTestInfo("Passed tests: %llu out of %llu (%llu%%). \r\n\r\n",
                          static_cast<unsigned long long>(passedTests),                                                        // NOLINT(*)
                          static_cast<unsigned long long>(noTests),                                                            // NOLINT(*)
                          static_cast<unsigned long long>((noTests > 0) ? ((passedTests * 100) / noTests) : 100));             // NOLINT(*)

    return (passedTests != noTests) ? STATUS_UNSUCCESSFUL
                                    : STATUS_SUCCESS;
}
