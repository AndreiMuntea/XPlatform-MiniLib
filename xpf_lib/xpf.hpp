/**
 * @file        xpf_lib/xpf.hpp
 *
 * @brief       This file contains all cpp stl like includes in the correct order.
 *              This can be included in precompiled headers (precomp.hpp)
 *              So it can be available throughout the project.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 * 
 * @license     See top-level directory LICENSE file.
 */


#pragma once


#include "public/core/Core.hpp"
#include "public/core/TypeTraits.hpp"
#include "public/core/Algorithm.hpp"
#include "public/core/PlatformApi.hpp"

#include "public/Memory/MemoryAllocator.hpp"
#include "public/Memory/CompressedPair.hpp"
#include "public/Memory/UniquePointer.hpp"
#include "public/Memory/SharedPointer.hpp"
#include "public/Memory/Optional.hpp"
#include "public/Memory/LookasideListAllocator.hpp"

#include "public/Containers/TwoLockQueue.hpp"
#include "public/Containers/String.hpp"
#include "public/Containers/Vector.hpp"

#include "public/Locks/Lock.hpp"
#include "public/Locks/BusyLock.hpp"
#include "public/Locks/ReadWriteLock.hpp"

#include "public/Multithreading/Thread.hpp"
#include "public/Multithreading/Signal.hpp"
#include "public/Multithreading/RundownProtection.hpp"
#include "public/Multithreading/ThreadPool.hpp"

#include "public/Utility/EventFramework.hpp"
#include "public/Utility/ISerializable.hpp"
