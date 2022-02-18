/// 
/// MIT License
/// 
/// Copyright(c) 2020 - 2022 MUNTEA ANDREI-MARIUS (munteaandrei17@gmail.com)
/// 
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files(the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions :
/// 
/// The above copyright notice and this permission notice shall be included in all
/// copies or substantial portions of the Software.
/// 
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
/// SOFTWARE.
/// 

#ifndef __XPLATFORM_PLATFORM_INCLUDES_HPP__
#define __XPLATFORM_PLATFORM_INCLUDES_HPP__

///
/// This header contains all includes needed for full access of the cross-platform (X-Platform) APIset.
/// This header file can be included in the precompiled header to save compile time.
/// Do not include other files separately.
///


///
/// This will force the definition of the placement new. If needed, it can be uncommented.
///

// #define XPLATFORM_PLACEMENT_NEW_DEFINITION

///
/// This will force the definition of the initializer list. If needed, it can be uncommented.
///

// #define XPLATFORM_INITIALIZER_LIST_DEFINITION


#include "inc/XPlatformCore.hpp"


#include "inc/XPlatformNumericLimits.hpp"
#include "inc/XPlatformTypeTraits.hpp"
#include "inc/XPlatformUtilsApiSet.hpp"
#include "inc/XPlatformSpecificApi.hpp"
#include "inc/XPlatformMemoryAllocator.hpp"


#include "inc/XPlatformSharedPointer.hpp"
#include "inc/XPlatformUniquePointer.hpp"


#include "inc/XPlatformList.hpp"
#include "inc/XPlatformVector.hpp"
#include "inc/XPlatformString.hpp"
#include "inc/XPlatformRedBlackTree.hpp"
#include "inc/XPlatformSet.hpp"
#include "inc/XPlatformMap.hpp"
#include "inc/XPlatformBitSet.hpp"


#include "inc/XPlatformInitializerList.hpp"
#include "inc/XPlatformPair.hpp"


#include "inc/XPlatformLocking.hpp"
#include "inc/XPlatformReadWriteLock.hpp"


#include "inc/XPlatformThread.hpp"
#include "inc/XPlatformSemaphore.hpp"
#include "inc/XPlatformThreadPool.hpp"

#endif // __XPLATFORM_PLATFORM_INCLUDES_HPP__