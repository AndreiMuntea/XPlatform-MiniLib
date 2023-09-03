

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
EXTERN_C_START;

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


DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    XPF_UNREFERENCED_PARAMETER(DriverObject);
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
    status = XpfTestInitializeCppSupport();
    if (!NT_SUCCESS(status))
    {
        XPF_ASSERT(false);
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
    XpfTestDeinitializeCppSupport();

    return status;
}


/**
 * @brief   Don't add anything after this macro.
 */
EXTERN_C_END;
