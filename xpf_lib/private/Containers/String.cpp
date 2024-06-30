/**
 * @file        xpf_lib/private/Containers/String.cpp
 *
 * @brief       In this file there are the platform specific APIs for
 *              converting between wide and utf8 strings.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */

#include "xpf_lib/xpf.hpp"

 /**
  * @brief   By default all code in here goes into the paged section.
  */
XPF_SECTION_PAGED;

#if defined XPF_PLATFORM_LINUX_UM

static NTSTATUS XPF_API
XpfPerformIConv(
    _In_ _Const_ const char* FromCodec,
    _In_ _Const_ const char* ToCodec,
    _Inout_ void* InputBuffer,
    _In_ size_t InputBufferSize,
    _Inout_ void* OutputBuffer,
    _In_ size_t OutputBufferSize
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Sanity check - validate parameters.
    //
    if ((nullptr == FromCodec) || (nullptr == ToCodec))
    {
        return STATUS_INVALID_PARAMETER;
    }
    if ((nullptr == InputBuffer) || (0 == InputBufferSize))
    {
        return STATUS_INVALID_PARAMETER;
    }
    if ((nullptr == OutputBuffer) || (0 == OutputBufferSize))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Now let's open a handle to iconv.
    //
    iconv_t iconvHandle = iconv_open(ToCodec, FromCodec);
    if (iconvHandle == (iconv_t)(-1))
    {
        return NTSTATUS_FROM_PLATFORM_ERROR(errno);
    }

    //
    // Get the input and output as character buffer.
    // We'll need to pass a char** as input and output.
    //
    char* inputBuffer = static_cast<char*>(InputBuffer);
    char* outputBuffer = static_cast<char*>(OutputBuffer);

    //
    // Do the actual conversion.
    //
    size_t result = iconv(iconvHandle,
                          &inputBuffer,
                          &InputBufferSize,
                          &outputBuffer,
                          &OutputBufferSize);

    //
    // Now close the handle to iconv.
    // This should always succeed.
    //
    const int closeResult = iconv_close(iconvHandle);
    XPF_DEATH_ON_FAILURE(0 == closeResult);

    //
    // In case of error, iconv() returns (size_t) -1
    // and sets errno to indicate the error.
    //
    if (result == static_cast<size_t>(-1))
    {
        return NTSTATUS_FROM_PLATFORM_ERROR(errno);
    }

    //
    // All good.
    //
    return STATUS_SUCCESS;
}

#endif  // XPF_PLATFORM_LINUX_UM


_Must_inspect_result_
NTSTATUS
XPF_API
xpf::StringConversion::WideToUTF8(
    _In_ _Const_ const xpf::StringView<wchar_t>& Input,
    _Inout_ xpf::String<char>& Output
) noexcept(true)
{
    size_t outSizeInBytes = 0;
    size_t inSizeInBytes = 0;

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Buffer outBuffer{ Output.GetAllocator() };

    //
    // We don't expect this to be called at higher IRQL.
    // So assert here to catch invalid usage.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // If input is empty, we just return an empty output.
    //
    if (Input.IsEmpty())
    {
        Output.Reset();
        return STATUS_SUCCESS;
    }

    //
    // We don't support inputs larger than int32 max.
    //
    if (Input.BufferSize() > static_cast<size_t>(xpf::NumericLimits<int32_t>::MaxValue()))
    {
        return STATUS_INVALID_BUFFER_SIZE;
    }
    inSizeInBytes = Input.BufferSize() * sizeof(wchar_t);
    if (inSizeInBytes > static_cast<size_t>(xpf::NumericLimits<int32_t>::MaxValue()))
    {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    //
    // Maximum growth factor for utf8 is 4 bytes.
    //
    outSizeInBytes = 4 * inSizeInBytes;

    //
    // Sanity check that the size is valid.
    // We won't allow conversions larger than int32 max.
    // And the size must also be a multiple of the output character type.
    //
    if ((outSizeInBytes / 4 != inSizeInBytes) ||
        (outSizeInBytes > static_cast<size_t>(xpf::NumericLimits<int32_t>::MaxValue())) ||
        (outSizeInBytes == 0) ||
        (outSizeInBytes % sizeof(char) != 0))
    {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    //
    // One more to null terminate the string.
    //
    outSizeInBytes += sizeof('\0');

    //
    // Now we allocate the output buffer.
    // And do a zero memory over it.
    //
    status = outBuffer.Resize(outSizeInBytes);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    xpf::ApiZeroMemory(outBuffer.GetBuffer(), outSizeInBytes);

    //
    // Now we do the actual conversion.
    //
    #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_WIN_KM
        {
            //
            // This API returns the required size in number of bytes.
            //
            ULONG ccbOutSize = 0;
            status = ::RtlUnicodeToUTF8N(static_cast<PCHAR>(outBuffer.GetBuffer()),
                                         static_cast<ULONG>(outBuffer.GetSize()),
                                         &ccbOutSize,
                                         Input.Buffer(),
                                         static_cast<ULONG>(inSizeInBytes));
            if ((!NT_SUCCESS(status)) || (STATUS_SOME_NOT_MAPPED == status))
            {
                return STATUS_FAIL_CHECK;
            }
        }
    #elif defined XPF_PLATFORM_LINUX_UM
        {
            //
            // iconv requires a non-const pointer... We need to duplicate
            // as it's not safe to const cast.
            //
            xpf::String<wchar_t> duplicatedInput;

            status = duplicatedInput.Append(Input);
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            //
            // Do the actual conversion.
            //
            status = XpfPerformIConv("WCHAR_T",
                                     "UTF-8",
                                     static_cast<void*>(&duplicatedInput[0]),
                                     inSizeInBytes,
                                     outBuffer.GetBuffer(),
                                     outSizeInBytes);
            if (!NT_SUCCESS(status))
            {
                return status;
            }
        }
    #else
        #error Unknown Platform
    #endif

    //
    // And now we just construct the output.
    // We'll go until we get the null terminated character.
    //
    xpf::StringView<char> outView{ static_cast<char*>(outBuffer.GetBuffer()) };
    Output.Reset();
    return Output.Append(outView);
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::StringConversion::UTF8ToWide(
    _In_ _Const_ const xpf::StringView<char>& Input,
    _Inout_ xpf::String<wchar_t>& Output
) noexcept(true)
{
    size_t outSizeInBytes = 0;
    size_t inSizeInBytes = 0;

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Buffer outBuffer{ Output.GetAllocator() };
    //
    // We don't expect this to be called at higher IRQL.
    // So assert here to catch invalid usage.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // If input is empty, we just return an empty output.
    //
    if (Input.IsEmpty())
    {
        Output.Reset();
        return STATUS_SUCCESS;
    }

    //
    // We don't support inputs larger than int32 max.
    //
    if (Input.BufferSize() > static_cast<size_t>(xpf::NumericLimits<int32_t>::MaxValue()))
    {
        return STATUS_INVALID_BUFFER_SIZE;
    }
    inSizeInBytes = Input.BufferSize() * sizeof(char);
    if (inSizeInBytes > static_cast<size_t>(xpf::NumericLimits<int32_t>::MaxValue()))
    {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    //
    // Maximum growth factor for utf8 is 4 bytes.
    //
    outSizeInBytes = 4 * inSizeInBytes;


    //
    // Sanity check that the size is valid.
    // We won't allow conversions larger than int32 max.
    // And the size must also be a multiple of the output character type.
    //
    if ((outSizeInBytes / 4 != inSizeInBytes) ||
        (outSizeInBytes > static_cast<size_t>(xpf::NumericLimits<int32_t>::MaxValue())) ||
        (outSizeInBytes == 0) ||
        (outSizeInBytes % sizeof(wchar_t) != 0))
    {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    //
    // One more to null terminate the string.
    //
    outSizeInBytes += sizeof(L'\0');

    //
    // Now we allocate the output buffer.
    // And do a zero memory over it.
    //
    status = outBuffer.Resize(outSizeInBytes);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    xpf::ApiZeroMemory(outBuffer.GetBuffer(), outSizeInBytes);

    //
    // Now we do the actual conversion.
    //
    #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_WIN_KM
        {
            //
            // This API returns the required size in number of bytes.
            //
            ULONG ccbOutSize = 0;
            status = ::RtlUTF8ToUnicodeN(static_cast<PWCHAR>(outBuffer.GetBuffer()),
                                         static_cast<ULONG>(outBuffer.GetSize()),
                                         &ccbOutSize,
                                         Input.Buffer(),
                                         static_cast<ULONG>(inSizeInBytes));
            if ((!NT_SUCCESS(status)) || (STATUS_SOME_NOT_MAPPED == status))
            {
                return STATUS_FAIL_CHECK;
            }
        }
    #elif defined XPF_PLATFORM_LINUX_UM
        {
            //
            // iconv requires a non-const pointer... We need to duplicate
            // as it's not safe to const cast.
            //
            xpf::String<char> duplicatedInput;
            status = duplicatedInput.Append(Input);
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            //
            // Do the actual conversion.
            //
            status = XpfPerformIConv("UTF-8",
                                     "WCHAR_T",
                                     static_cast<void*>(&duplicatedInput[0]),
                                     inSizeInBytes,
                                     outBuffer.GetBuffer(),
                                     outSizeInBytes);
            if (!NT_SUCCESS(status))
            {
                return status;
            }
        }
    #else
        #error Unknown Platform
    #endif

    //
    // And now we just construct the output.
    //
    xpf::StringView<wchar_t> outView{ static_cast<const wchar_t*>(outBuffer.GetBuffer()) };
    Output.Reset();
    return Output.Append(outView);
}
