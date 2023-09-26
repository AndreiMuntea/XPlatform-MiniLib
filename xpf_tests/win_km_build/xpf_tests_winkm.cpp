

#include "CppSupport/CppSupport.hpp"
#include "xpf_tests/XPF-TestIncludes.hpp"


/**
 * @brief   By default everything goes into paged object section.
 *          The code here is expected to be run only at passive level.
 *          It is safe to place everything in paged area. 
 */
XPF_SECTION_PAGED;


/**
 * @brief   We need this for name mangling.
 */
XPF_EXTERN_C_START();


/**
 * @brief This is the driver exit routine.
 *        It will be called when "sc stop service_name" is invoked.
 *        No cleanup is required as we are doing everything in Driver Entry.
 * 
 * @param[in] DriverObject - Descriptor for the test driver.
 *
 * @return None.
 */
VOID
DriverExit(
    _In_ struct _DRIVER_OBJECT* DriverObject
)
{
    XPF_UNREFERENCED_PARAMETER(DriverObject);

    //
    // We should always be called at passive.
    //
    XPF_MAX_PASSIVE_LEVEL();
}

/**
 * @brief We need to pre-declare this as DRIVER_INITIALIZE.
 *        It is implemented right below.
 */
DRIVER_INITIALIZE DriverEntry;

/**
 * @brief This is the driver entry routine.
 *        It will be called when "sc start service_name" is invoked.
 *        CPP support will be initialized, and then the tests will start running.
 *        This routine won't return untill all tests finished.
 *
 * @param[in] DriverObject - Descriptor for the test driver.
 * 
 * @param[in] RegistryPath - The path to registry allocated for this driver.
 *                           We're not using this at the moment.
 * 
 * @return Proper NTSTATUS error code signaling whether the tests
 *         finished successfully or they encountered any errors.
 *
 * @note A timeout might be encountered if there are lots of unit tests.
 *       If the caller does not want to block until the test execution finishes,
 *       A separated thread may be created to run the tests.
 *       It can be joined on DriverUnload routine.
 *       However, this is not the case for us, as we just want a minimalistic support.
 *       We might revist this in future.
 */
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    XPF_UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    //
    // We should always be called at passive.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Opt-In for NX support - must be done before any allocation.
    // So we do it the first thing in Driver entry.
    //
    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);
    DriverObject->DriverUnload = DriverExit;

    //
    // Initialize cpp support - Must be done before running the tests.
    //
    status = XpfInitializeCppSupport();
    if (!NT_SUCCESS(status))
    {
        XPF_DEATH_ON_FAILURE(false);
        return status;
    }

    //
    // Now run all tests and save result.
    //
    status = xpf_test::RunAllTests();
    XPF_ASSERT(NT_SUCCESS(status));

    //
    // Cleanup cpp support before returning.
    //
    XpfDeinitializeCppSupport();

    return status;
}


/**
 * @brief   We need this for name mangling.
 */
XPF_EXTERN_C_END();
