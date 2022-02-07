#ifndef __TEST_DUMMY_STRUCT_HPP__
#define __TEST_DUMMY_STRUCT_HPP__


class DummyTestStruct
{
public:
    DummyTestStruct(
        _In_ int Number,
        _In_ char Character,
        _In_ float FloatNumber
    ) noexcept : Number{ Number },
        Character{ Character },
        FloatNumber{ FloatNumber }
    {}
    virtual ~DummyTestStruct() noexcept = default;

    // Copy semantics
    DummyTestStruct(const DummyTestStruct& Other) noexcept = default;
    DummyTestStruct& operator=(const DummyTestStruct& Other) noexcept = default;

    // Move semantics
    DummyTestStruct& operator=(DummyTestStruct&& Other) noexcept = default;
    DummyTestStruct(DummyTestStruct&& Other) noexcept = default;

    // Operator ==
    inline bool operator==(const DummyTestStruct& Other) const noexcept
    {
        return (Number == Other.Number) && (Character == Other.Character) && (FloatNumber == Other.FloatNumber);
    }

public:
    int Number = 0;
    char Character = '\0';
    float FloatNumber = 0.0f;
};

class DummyTestStructDerived : public DummyTestStruct
{
public:
    DummyTestStructDerived(
        _In_ int Number,
        _In_ char Character,
        _In_ float FloatNumber,
        _In_ double DoubleNumber
    ) noexcept : DummyTestStruct(Number, Character, FloatNumber),
        DoubleNumber{ DoubleNumber }
    {}

    virtual ~DummyTestStructDerived() noexcept = default;

    // Copy semantics
    DummyTestStructDerived(const DummyTestStructDerived& Other) noexcept = default;
    DummyTestStructDerived& operator=(const DummyTestStructDerived& Other) noexcept = default;

    // Move semantics
    DummyTestStructDerived& operator=(DummyTestStructDerived&& Other) noexcept = default;
    DummyTestStructDerived(DummyTestStructDerived&& Other) noexcept = default;

    // Operator ==
    inline bool operator==(const DummyTestStructDerived& Other) const noexcept
    {
        return DummyTestStruct::operator==(Other) && (DoubleNumber == Other.DoubleNumber);
    }

public:
    double DoubleNumber = 0.0;
};


#endif // __TEST_DUMMY_STRUCT_HPP__