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

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::StringConversion::WideToUTF8(
    _In_ _Const_ const xpf::StringView<wchar_t>& Input,
    _Out_ xpf::String<char>& Output
) noexcept(true)
{
    size_t outSizeInBytes = 0;
    size_t inSizeInBytes = 0;

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Buffer outBuffer;

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
    // Now we query the required size.
    // This is platform specific.
    //
    #if defined XPF_PLATFORM_WIN_UM
        {
            //
            // This API returns the required size in number of characters.
            //
            const int cchOutSize = ::WideCharToMultiByte(CP_UTF8,
                                                         WC_ERR_INVALID_CHARS,
                                                         Input.Buffer(),
                                                         static_cast<int>(inSizeInBytes / sizeof(wchar_t)),
                                                         NULL,
                                                         0,
                                                         NULL,
                                                         NULL);
            outSizeInBytes = static_cast<size_t>(cchOutSize);
        }
    #elif defined XPF_PLATFORM_WIN_KM
        {
            //
            // This API does not work at other IRQL than PASSIVE.
            //
            if (PASSIVE_LEVEL != ::KeGetCurrentIrql())
            {
                return STATUS_INVALID_STATE_TRANSITION;
            }

            //
            // This API returns the required size in number of bytes.
            //
            ULONG ccbOutSize = 0;
            status = ::RtlUnicodeToUTF8N(NULL,
                                         0,
                                         &ccbOutSize,
                                         Input.Buffer(),
                                         static_cast<ULONG>(inSizeInBytes));
            outSizeInBytes = static_cast<size_t>(ccbOutSize);
        }
    #elif defined XPF_PLATFORM_LINUX_UM
        {
            //
            // UTF-8 grows with a factor of 4. Use the max here.
            // We'll adjust below.
            //
            outSizeInBytes = inSizeInBytes * 4;
        }
    #else
        #error Unknown Platform
    #endif

    //
    // Sanity check that the size is valid.
    // We won't allow conversions larger than int32 max.
    // And the size must also be a multiple of the output character type.
    //
    if ((outSizeInBytes > static_cast<size_t>(xpf::NumericLimits<int32_t>::MaxValue())) ||
        (outSizeInBytes == 0) ||
        (outSizeInBytes % sizeof(char) != 0))
    {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    //
    // Now we allocate the output buffer.
    //
    status = outBuffer.Resize(outSizeInBytes);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //
    // Now we do the actual conversion.
    //
    #if defined XPF_PLATFORM_WIN_UM
        {
            const int cchOutSize = ::WideCharToMultiByte(CP_UTF8,
                                                         WC_ERR_INVALID_CHARS,
                                                         Input.Buffer(),
                                                         static_cast<int>(inSizeInBytes / sizeof(wchar_t)),
                                                         reinterpret_cast<LPSTR>(outBuffer.GetBuffer()),
                                                         static_cast<int>(outBuffer.GetSize()),
                                                         NULL,
                                                         NULL);
            outSizeInBytes = static_cast<size_t>(cchOutSize);
        }
    #elif defined XPF_PLATFORM_WIN_KM
        {
            //
            // This API returns the required size in number of bytes.
            //
            ULONG ccbOutSize = 0;
            status = ::RtlUnicodeToUTF8N(reinterpret_cast<PCHAR>(outBuffer.GetBuffer()),
                                         static_cast<ULONG>(outBuffer.GetSize()),
                                         &ccbOutSize,
                                         Input.Buffer(),
                                         static_cast<ULONG>(inSizeInBytes));
            if ((!NT_SUCCESS(status)) || (STATUS_SOME_NOT_MAPPED == status))
            {
                return STATUS_FAIL_CHECK;
            }
            outSizeInBytes = static_cast<size_t>(ccbOutSize);
        }
    #elif defined XPF_PLATFORM_LINUX_UM
        {
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
            // Now let's open a handle to iconv from WCHAR_T to UTF-8.
            //
            iconv_t iconvHandle = iconv_open("UTF-8",
                                             "WCHAR_T");
            if (iconvHandle == (iconv_t)-1)
            {
                return NTSTATUS_FROM_PLATFORM_ERROR(errno);
            }

            //
            // This will make the code easier to read.
            //
            char* inputBufferAsChar = reinterpret_cast<char*>(&duplicatedInput[0]);
            size_t inputBufferSizeCopy = inSizeInBytes;

            char* outputBufferAsChar = reinterpret_cast<char*>(outBuffer.GetBuffer());
            size_t outputBufferSizeCopy = outSizeInBytes;

            size_t result = iconv(iconvHandle,
                                  &inputBufferAsChar,
                                  &inputBufferSizeCopy,
                                  &outputBufferAsChar,
                                  &outputBufferSizeCopy);

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
            // Now we need to resize the out buffer.
            //
            outSizeInBytes -= outputBufferSizeCopy;
            status = outBuffer.Resize(outSizeInBytes);
            if (!NT_SUCCESS(status))
            {
                return status;
            }
        }
        }
    #else
        #error Unknown Platform
    #endif

    //
    // Sanity check that the required size is what we got previously
    //
    if (outSizeInBytes != outBuffer.GetSize())
    {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    //
    // And now we just construct the output.
    //
    xpf::StringView<char> outView{ reinterpret_cast<const char*>(outBuffer.GetBuffer()),
                                   outBuffer.GetSize() };
    Output.Reset();
    return Output.Append(outView);
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::StringConversion::UTF8ToWide(
    _In_ _Const_ const xpf::StringView<char>& Input,
    _Out_ xpf::String<wchar_t>& Output
) noexcept(true)
{
    size_t outSizeInBytes = 0;
    size_t inSizeInBytes = 0;

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Buffer outBuffer;

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
    // Now we query the required size.
    // This is platform specific.
    //
    #if defined XPF_PLATFORM_WIN_UM
        {
            //
            // This API returns the required size in number of characters.
            //
            const int cchOutSize = ::MultiByteToWideChar(CP_UTF8,
                                                         MB_ERR_INVALID_CHARS,
                                                         Input.Buffer(),
                                                         static_cast<int>(inSizeInBytes / sizeof(char)),
                                                         NULL,
                                                         0);
            outSizeInBytes = static_cast<size_t>(cchOutSize) * sizeof(wchar_t);
        }
    #elif defined XPF_PLATFORM_WIN_KM
        {
            //
            // This API does not work at other IRQL than PASSIVE.
            //
            if (PASSIVE_LEVEL != ::KeGetCurrentIrql())
            {
                return STATUS_INVALID_STATE_TRANSITION;
            }

            //
            // This API returns the required size in number of bytes.
            //
            ULONG ccbOutSize = 0;
            status = ::RtlUTF8ToUnicodeN(NULL,
                                         0,
                                         &ccbOutSize,
                                         Input.Buffer(),
                                         static_cast<ULONG>(inSizeInBytes));
            outSizeInBytes = static_cast<size_t>(ccbOutSize);
        }
    #elif defined XPF_PLATFORM_LINUX_UM
        {
            //
            // wchar_t whould be large enough to store any utf-8 character.
            // Use the max here. We'll adjust below.
            //
            outSizeInBytes = inSizeInBytes * sizeof(wchar_t);
        }
    #else
        #error Unknown Platform
    #endif

    //
    // Sanity check that the size is valid.
    // We won't allow conversions larger than int32 max.
    // And the size must also be a multiple of the output character type.
    //
    if ((outSizeInBytes > static_cast<size_t>(xpf::NumericLimits<int32_t>::MaxValue())) ||
        (outSizeInBytes == 0) ||
        (outSizeInBytes % sizeof(wchar_t) != 0))
    {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    //
    // Now we allocate the output buffer.
    //
    status = outBuffer.Resize(outSizeInBytes);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //
    // Now we do the actual conversion.
    //
    #if defined XPF_PLATFORM_WIN_UM
        {
            //
            // This API returns the required size in number of characters.
            //
            const int cchOutSize = ::MultiByteToWideChar(CP_UTF8,
                                                         MB_ERR_INVALID_CHARS,
                                                         Input.Buffer(),
                                                         static_cast<int>(inSizeInBytes / sizeof(char)),
                                                         reinterpret_cast<LPWSTR>(outBuffer.GetBuffer()),
                                                         static_cast<int>(outBuffer.GetSize() / sizeof(wchar_t)));
            outSizeInBytes = static_cast<size_t>(cchOutSize) * sizeof(wchar_t);
        }
    #elif defined XPF_PLATFORM_WIN_KM
        {
            //
            // This API returns the required size in number of bytes.
            //
            ULONG ccbOutSize = 0;
            status = ::RtlUTF8ToUnicodeN(reinterpret_cast<PWCHAR>(outBuffer.GetBuffer()),
                                         static_cast<ULONG>(outBuffer.GetSize()),
                                         &ccbOutSize,
                                         Input.Buffer(),
                                         static_cast<ULONG>(inSizeInBytes));
            if ((!NT_SUCCESS(status)) || (STATUS_SOME_NOT_MAPPED == status))
            {
                return STATUS_FAIL_CHECK;
            }
            outSizeInBytes = static_cast<size_t>(ccbOutSize);
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
            // Now let's open a handle to iconv from UTF-8 to WCHAR_T.
            //
            iconv_t iconvHandle = iconv_open("WCHAR_T",
                                             "UTF-8");
            if (iconvHandle == (iconv_t)-1)
            {
                return NTSTATUS_FROM_PLATFORM_ERROR(errno);
            }

            //
            // This will make the code easier to read.
            //
            char* inputBufferAsChar = &duplicatedInput[0];
            size_t inputBufferSizeCopy = inSizeInBytes;

            char* outputBufferAsChar = reinterpret_cast<char*>(outBuffer.GetBuffer());
            size_t outputBufferSizeCopy = outSizeInBytes;

            size_t result = iconv(iconvHandle,
                                  &inputBufferAsChar,
                                  &inputBufferSizeCopy,
                                  &outputBufferAsChar,
                                  &outputBufferSizeCopy);

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
            // Now we need to resize the out buffer.
            //
            outSizeInBytes -= outputBufferSizeCopy;
            status = outBuffer.Resize(outSizeInBytes);
            if (!NT_SUCCESS(status))
            {
                return status;
            }
        }
    #else
        #error Unknown Platform
    #endif

    //
    // Sanity check that the required size is what we got previously
    //
    if (outSizeInBytes != outBuffer.GetSize())
    {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    //
    // And now we just construct the output.
    //
    xpf::StringView<wchar_t> outView{ reinterpret_cast<const wchar_t*>(outBuffer.GetBuffer()),
                                      outBuffer.GetSize() / sizeof(wchar_t) };
    Output.Reset();
    return Output.Append(outView);
}
