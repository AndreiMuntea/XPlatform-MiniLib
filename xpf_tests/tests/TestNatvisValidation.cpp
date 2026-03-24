/**
 * @file        xpf_tests/tests/TestNatvisValidation.cpp
 *
 * @brief       This test constructs one instance of every visualized type
 *              with known values. Set a breakpoint on the final line to
 *              inspect all variables in the Locals/Watch window.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */


#include "xpf_tests/XPF-TestIncludes.hpp"
#include "xpf_tests/Mocks/TestMocks.hpp"
#include "xpf_tests/Mocks/TestMockEvents.hpp"

/**
 * @brief       This scenario constructs one instance of every type that has
 *              a natvis visualizer so that all of them can be inspected from
 *              a single breakpoint.  The final XPF_TEST_EXPECT_TRUE is the
 *              line to set the breakpoint on.
 */
XPF_TEST_SCENARIO(TestNatvisValidation, BreakpointTarget)
{
    //
    // Buffer (with bytes visible)
    //
    xpf::Buffer buffer;
    NTSTATUS status = buffer.Resize(64);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // Vector<int>
    //
    xpf::Vector<int> intVector;
    status = intVector.Emplace(10);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    status = intVector.Emplace(20);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    status = intVector.Emplace(30);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // StringView<char> and StringView<wchar_t>
    //
    xpf::StringView<char> charView("hello natvis");
    xpf::StringView<wchar_t> wcharView(L"hello natvis");

    //
    // String<char> and String<wchar_t>
    //
    xpf::String<char> charString;
    status = charString.Append(charView);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::String<wchar_t> wcharString;
    status = wcharString.Append(wcharView);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // Span<int>
    //
    xpf::Span<int> intSpan(&intVector[0], intVector.Size());

    //
    // Optional<int> (empty and with value)
    //
    xpf::Optional<int> emptyOptional;
    xpf::Optional<int> fullOptional;
    fullOptional.Emplace(42);

    //
    // UniquePointer<int>
    //
    auto uniqueInt = xpf::MakeUnique<int>(100);
    XPF_TEST_EXPECT_TRUE(!uniqueInt.IsEmpty());

    //
    // SharedPointer<int>
    //
    auto sharedInt = xpf::MakeShared<int>(200);
    XPF_TEST_EXPECT_TRUE(!sharedInt.IsEmpty());

    //
    // BusyLock
    //
    xpf::BusyLock busyLock;

    //
    // ReadWriteLock
    //
    xpf::Optional<xpf::ReadWriteLock> rwLock;
    status = xpf::ReadWriteLock::Create(&rwLock);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // ExclusiveLockGuard and SharedLockGuard
    // (live at breakpoint inside scope)
    //
    xpf::ExclusiveLockGuard exclusiveGuard{busyLock};
    XPF_TEST_EXPECT_TRUE(true);

    //
    // We need a separate lock for the shared guard since busyLock is held exclusive.
    //
    xpf::BusyLock sharedLockTarget;
    xpf::SharedLockGuard sharedGuard{sharedLockTarget};
    XPF_TEST_EXPECT_TRUE(true);

    //
    // RundownProtection
    //
    xpf::RundownProtection rundownProtection;

    //
    // RundownGuard (live at breakpoint)
    //
    xpf::RundownGuard rundownGuard{rundownProtection};
    XPF_TEST_EXPECT_TRUE(rundownGuard.IsRundownAcquired());

    //
    // TwoLockQueue (push and pop so it stays alive)
    //
    xpf::TwoLockQueue tlq;
    xpf::XPF_SINGLE_LIST_ENTRY tlqEntry = { nullptr };
    xpf::TlqPush(tlq, &tlqEntry);

    //
    // LookasideListAllocator (alloc + free so the compiler keeps it alive)
    //
    xpf::LookasideListAllocator lookasideAllocator(128, false);
    void* lookasideBlock = lookasideAllocator.AllocateMemory(64);
    XPF_TEST_EXPECT_TRUE(nullptr != lookasideBlock);
    lookasideAllocator.FreeMemory(lookasideBlock);

    //
    // RedBlackTree<int, int> with RedBlackTreeNode and RedBlackTreeIterator
    //
    xpf::RedBlackTree<int, int> rbTree;
    status = rbTree.Emplace(int{1}, int{100});
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    status = rbTree.Emplace(int{2}, int{200});
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    status = rbTree.Emplace(int{3}, int{300});
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    auto treeIterator = rbTree.Begin();
    auto treeEnd = rbTree.End();
    XPF_TEST_EXPECT_TRUE(treeIterator != treeEnd);

    //
    // Map<int, int>
    //
    xpf::Map<int, int> intMap;
    status = intMap.Emplace(int{10}, int{1000});
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    status = intMap.Emplace(int{20}, int{2000});
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // Set<int>
    //
    xpf::Set<int> intSet;
    status = intSet.Insert(int{100});
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    status = intSet.Insert(int{200});
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // thread::Thread
    //
    xpf::thread::Thread thread;

    //
    // Signal 
    //
    xpf::Optional<xpf::Signal> signal;
    status = xpf::Signal::Create(&signal, true);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // ThreadPool
    //
    xpf::Optional<xpf::ThreadPool> threadPool;
    status = xpf::ThreadPool::Create(&threadPool);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // StreamWriter and StreamReader
    //
    xpf::Buffer streamBuffer;
    status = streamBuffer.Resize(32);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::StreamWriter writer(streamBuffer);
    XPF_TEST_EXPECT_TRUE(writer.WriteNumber(uint32_t{0xDEADBEEF}));

    const xpf::Buffer& constStreamBuffer = streamBuffer;
    xpf::StreamReader reader(constStreamBuffer);
    uint32_t readValue = 0;
    XPF_TEST_EXPECT_TRUE(reader.ReadNumber(readValue));
    XPF_TEST_EXPECT_TRUE(readValue == 0xDEADBEEF);

    //
    // EventBus with a registered listener (EventListenerData visible via expansion)
    //
    xpf::EventBus eventBus;
    xpf::mocks::MockEventListener mockListener(xpf::EVENT_ID{1});
    xpf::EVENT_LISTENER_ID listenerId = { 0 };
    status = eventBus.RegisterListener(&mockListener, &listenerId);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // Protobuf (stateless serializer)
    //
    xpf::Protobuf protobuf;

    //
    // Signal the signal so we can see its state.
    //
    (*signal).Set();

    XPF_TEST_EXPECT_TRUE(buffer.GetSize() == 64);

    //
    // Cleanup: unregister listener and pop tlq entry before locals are destroyed.
    //
    status = eventBus.UnregisterListener(listenerId);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::XPF_SINGLE_LIST_ENTRY* poppedEntry = xpf::TlqPop(tlq);
    XPF_TEST_EXPECT_TRUE(poppedEntry == &tlqEntry);
}
