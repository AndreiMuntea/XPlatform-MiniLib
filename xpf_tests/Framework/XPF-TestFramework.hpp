/**
 * @file        xpf_tests/Framework/XPF-TestFramework.hpp
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


#pragma once

 /**
  *
  * @brief We depend only on the xpf lib.
  */
#include <xpf_lib/xpf.hpp>


namespace xpf_test
{
//
// ************************************************************************************************
// This is the section containing the definition for user-exposed test api.
// ************************************************************************************************
//

/**
 * @brief This method is used to setup the required resources
 *        for running the test. It is usually called in the entry point.
 *        It will wait for all tests to finish execution.
 *        The result will be displayed on the console (in user mode),
 *        or to debug prompt for windows KM.
 * 
 * @return a proper NTSTATUS error code to signal the success.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
RunAllTests(
    void
) noexcept(true);

/**
 * @brief This rotuine is used to log to the default console.
 *        It is platform dependant.
 *
 * @param[in] Format - The format specifier - see printf doc for this.
 *
 */

inline void
LogTestInfo(
    _In_ const char* Format,
    ...
) noexcept(true)
{
    va_list argList;
    xpf::ApiZeroMemory(&argList, sizeof(argList));

    va_start(argList, Format);

    #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_LINUX_UM
        (void) vfprintf(stdout, Format, argList);
    #elif defined XPF_PLATFORM_WIN_KM
        (void) vDbgPrintEx(DPFLTR_IHVDRIVER_ID,
                           DPFLTR_ERROR_LEVEL,
                           Format,
                           argList);
    #else
        #error Unknown Platform
    #endif

    va_end(argList);
}


/**
 * @brief   Definition for test scenario callback type which will be executed.
 */
typedef void(XPF_API* TestScenarioCallback)([[maybe_unused]] NTSTATUS* _XpfArgScenario) noexcept(true);

};  // namespace xpf_test

/**
 * @brief       On Linux User Mode we'll register a signal handler.
 *              This will set this global variable to signal whether death was
 *              signaled or not.
 *              On Windows platforms this will be signaled from a SEH handler.
 *
 * @note        It's the responsibility of each test to reset this global properly
 *              before registering the signal handler.
 */
extern bool mXpfConditionHasGeneratedDeath;


/**
 * 
 * @brief       Create two new sections, one before and one after ".xpfTsT".
 *              This will help us walk the sections in order.
 *              The linker will ensure they are alphabetically sorted.
 * 
 *              See https://devblogs.microsoft.com/oldnewthing/20181107-00/?p=100155
 *              gcc and clang seem to have the same behavior.
 * 
 *                  objdump -x ./XPF_Tests  | grep "xpfts"
 *                  25 xpfts$a       00000090  0000000000532610  0000000000532610  00131610  2**3
 *                  26 xpfts$t       00000098  00000000005326a0  00000000005326a0  001316a0  2**3
 *                  27 xpfts$z       00000090  0000000000532738  0000000000532738  00131738  2**3
 * 
 * @note        xpfts$a - will be the "start marker" 
 *              xpfts$z - will be the "end marker"
 *              xpfts$t - is where the tests will actually go.
 * 
 * @note        In order for these to work, we need to insert first a variable in the "a" section.
 *              So these actually need to go in the hpp file. So yeah, we'll allocate 3 pointers
 *              each time this is included. But this is only for tests, so it should be ok.
 *              We can come back to this to improve later on. For now this suffice.
 *
 * @note        These will also be volatile to tell the compiler it shouldn't remove them
 *              even though they are not used anywhere.
 */
#if defined XPF_COMPILER_MSVC
    #pragma section("xpfts$a", read)
    #pragma section("xpfts$t", read)
    #pragma section("xpfts$z", read)
    #pragma comment(linker, "/merge:xpfts=.rdata")
#endif

/**
 * @brief       First section is "xpfts$a" -- we need this first as we need to define the $a section.
 */
XPF_DECLSPEC_SELECTANY()
XPF_DECLSPEC_EXPORT()
XPF_ALLOC_SECTION("xpfts$a")
const xpf_test::TestScenarioCallback* gXpfStartMarker = nullptr;

/**
 * @brief       Allocate something in between - "xpfts$t".
 */
XPF_DECLSPEC_SELECTANY()
XPF_DECLSPEC_EXPORT()
XPF_ALLOC_SECTION("xpfts$t")
const xpf_test::TestScenarioCallback* gXpfTestMarker = nullptr;

/**
 * @brief       Last section is "xpfts$z" -- we need this last as we need to define the $z section after the other 2.
 */
XPF_DECLSPEC_SELECTANY()
XPF_DECLSPEC_EXPORT()
XPF_ALLOC_SECTION("xpfts$z")
const xpf_test::TestScenarioCallback* gXpfEndMarker = nullptr;


/**
 * @brief       Useful macro to stringify a variable.
 */
#define XPF_TEST_STRINGIFY(X)               #X

/**
 * @brief       Useful macro to concatenate and stringify.
 *              Uses "::" as separator between X and Y.
 */
#define XPF_TEST_STRINGIFY_CONCAT(X, Y)     XPF_TEST_STRINGIFY(X::) #Y



/**
 * @brief       This Macro is internally used by XPF_TEST_EXPECT_TRUE
 *              If the condition is not met, the test will be marked as failed.
 *              It should only be called from inside a test as it will use
 *              _XpfArgScenario which makes sense only from a test function perspective.
 */
#define XPF_TEST_EXPECT_TRUE_INTERNAL(Condition, File, Line)                                                            \
        {                                                                                                               \
            if (!(Condition))   /* NOLINT(*) */                                                                         \
            {                                                                                                           \
                /* If we have a debugger this is useful :) */                                                           \
                XPF_ASSERT(false);                                                                                      \
                                                                                                                        \
                /* This will be changed to an actual log. */                                                            \
                xpf_test::LogTestInfo("    [!] [%s::%d] Condition %s is not met! \r\n",                                 \
                                      File,                                                                             \
                                      Line,                                                                             \
                                      XPF_TEST_STRINGIFY(Condition));                                                   \
                                                                                                                        \
                /* Mark the test as failed. */                                                                          \
                (*_XpfArgScenario) = STATUS_UNSUCCESSFUL;                                                               \
            }                                                                                                           \
            _Analysis_assume_((Condition));                                                                             \
        }

/**
 * @brief       This Macro is internally used by XPF_TEST_EXPECT_DEATH
 *              It is platform dependant.
 */
#if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_WIN_KM
    /**
     * @brief       On windows KM we use SEH to catch possible death.
     *              We can't use SEH in functions that require UNWIND.
     *              Wrap the execution in a lambda function.
     */
    #define XPF_TEST_EXPECT_DEATH_INTERNAL(Statement, DeathExpected, File, Line)                                    \
    {                                                                                                               \
        mXpfConditionHasGeneratedDeath = false;                                                                     \
        auto __mXpfConditionSEHLambda = [&]() {                                                                     \
            __try                                                                                                   \
            {                                                                                                       \
                (void)(Statement);                                                                                  \
                mXpfConditionHasGeneratedDeath = false;                                                             \
            }                                                                                                       \
            __except(EXCEPTION_EXECUTE_HANDLER)                                                                     \
            {                                                                                                       \
               mXpfConditionHasGeneratedDeath = true;                                                               \
            }                                                                                                       \
        };                                                                                                          \
        __mXpfConditionSEHLambda();                                                                                 \
        XPF_TEST_EXPECT_TRUE_INTERNAL(mXpfConditionHasGeneratedDeath == DeathExpected, File, Line);                 \
    }
#elif defined XPF_PLATFORM_LINUX_UM
    /**
     * @brief       This is the signal handler registered for death tests.
     *              We'll listen for signal signals. and set a global boolean.
     * 
     * @param[in]    SingalNumber - The identifier of the signal that has been emmited.
     *
     * @return void.
     */
    inline void XpfDeathTestSignalHandler(_In_ int SignalNumber) noexcept(true)
    {
        if (SignalNumber == SIGSEGV)
        {
            mXpfConditionHasGeneratedDeath = true;
        }
    }

    #define XPF_TEST_EXPECT_DEATH_INTERNAL(Statement, DeathExpected, File, Line)                                    \
    {                                                                                                               \
        /* Reset the value of gXpfTestDeathIsSignaled. */                                                           \
        mXpfConditionHasGeneratedDeath = false;                                                                     \
                                                                                                                    \
        /* Register signal handler for the signal raised by Panic(). */                                             \
        XPF_TEST_EXPECT_TRUE_INTERNAL(SIG_ERR != signal(SIGSEGV, XpfDeathTestSignalHandler), File, Line);           \
                                                                                                                    \
        /* Registered a signal handler, Now execute the statement. */                                               \
        { (void)(Statement); }                                                                                      \
                                                                                                                    \
        /* The result should be the same as expected death. */                                                      \
        XPF_TEST_EXPECT_TRUE_INTERNAL(mXpfConditionHasGeneratedDeath == DeathExpected, File, Line);                 \
                                                                                                                    \
        /* And now reset the signal handler. */                                                                     \
        XPF_TEST_EXPECT_TRUE_INTERNAL(SIG_ERR != signal(SIGSEGV, SIG_DFL), File, Line);                             \
    }
#else
    #error Unknown Platform
#endif


/**
 * @brief       This Macro is used to in tests to evaluate a condition to true.
 *              It will end up calling XPF_TEST_EXPECT_TRUE_INTERNAL as we need the file
 *              and line details for more verbose information.
 */
#define XPF_TEST_EXPECT_TRUE(Condition)         XPF_TEST_EXPECT_TRUE_INTERNAL(Condition, __FILE__, __LINE__)

/**
 * @brief       This Macro is used to in tests to evaluate that a statement will cause a crash.
 *              On windows UM and KM platforms the "crash" means a SEH exception which is expcted and will be handled.
 *              On linux UM it is a signal, see ApiPanic().
 */
#define XPF_TEST_EXPECT_DEATH(Statement)        XPF_TEST_EXPECT_DEATH_INTERNAL(Statement, true, __FILE__, __LINE__)

/**
 * @brief       This Macro is used to in tests to evaluate that a statement will not cause a crash.
 *              On windows UM and KM platforms the "crash" means a SEH exception which is expcted and will be handled.
 *              On linux UM it is a signal, see ApiPanic().
 */
#define XPF_TEST_EXPECT_NO_DEATH(Statement)     XPF_TEST_EXPECT_DEATH_INTERNAL(Statement, false, __FILE__, __LINE__)

/**
 * @brief       This Macro is used to in tests to evaluate for example a code path that triggers an assert on debug.
 *              On windows UM and KM platforms the "crash" means a SEH exception which is expcted and will be handled.
 *              On linux UM it is a signal, see ApiPanic().
 */
#if defined XPF_CONFIGURATION_DEBUG
    /*
     * @brief   On debug we expect a death. 
     */
    #define XPF_TEST_EXPECT_DEATH_ON_DEBUG   XPF_TEST_EXPECT_DEATH
#else
    /*
     * @brief   On release we do not expect a death. 
     */
    #define XPF_TEST_EXPECT_DEATH_ON_DEBUG   XPF_TEST_EXPECT_NO_DEATH
#endif  // XPF_PLATFORM_DEBUG


/**
 * @brief       This Macro is used to define what is required for a test scenario.
 */
#define XPF_TEST_SCENARIO(Namespace, Api)                                                                                   \
                                                                                                                            \
    /* First we forward declare the API. It is needed as it is referenced by structures. */                                 \
    /* The actual implementation will be provided by the user. */                                                           \
    namespace Namespace                                                                                                     \
    {                                                                                                                       \
        namespace Api                                                                                                       \
        {                                                                                                                   \
            void XPF_API TestImpl([[maybe_unused]] NTSTATUS* _XpfArgScenario) noexcept(true);                               \
            void XPF_API Test(NTSTATUS* _XpfArgScenario) noexcept(true);                                                    \
        };                                                                                                                  \
    };                                                                                                                      \
                                                                                                                            \
    void XPF_API Namespace::Api::Test(NTSTATUS* _XpfArgScenario) noexcept(true)                                             \
    {                                                                                                                       \
        xpf_test::LogTestInfo("[*] Executing scenario '%s'! \r\n",                                                          \
                              XPF_FUNCSIG());                                                                               \
        Namespace::Api::TestImpl(_XpfArgScenario);                                                                          \
    }                                                                                                                       \
                                                                                                                            \
    /* In xpftst$t section we can only put pointers, so we declare another variable which stores the addres */              \
    /* of the gxpftstScenario_<Namespace>_<Api> declared above. This pointer will be allocated in the */                    \
    /* proper test section. Its name will be gxpftstMarker_<Namespace>_<Api> */                                             \
    XPF_DECLSPEC_EXPORT()                                                                                                   \
    XPF_ALLOC_SECTION("xpfts$t")                                                                                            \
    const xpf_test::TestScenarioCallback* gxpftstMarker##Namespace##Api =                                                   \
                                                        (xpf_test::TestScenarioCallback*)(&Namespace::Api::Test);           \
                                                                                                                            \
    /* And now the caller must provide the implementation for the api. */                                                   \
    /* Scenario is only accessed via test macro. So the compiler might not perceive it as used. */                          \
    /* CPP [[maybe_unused]] comes to rescue to not suppress warnings. */                                                    \
    /* Mangle the name to <_XpfArgScenario> so it won't conflict with user's variable. */                                   \
    void XPF_API Namespace::Api::TestImpl([[maybe_unused]] NTSTATUS* _XpfArgScenario) noexcept(true)
