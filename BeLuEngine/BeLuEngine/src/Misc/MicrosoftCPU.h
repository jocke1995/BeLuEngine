#ifndef MICROSOFTCPU_H
#define MICROSOFTCPU_H
// Taken from msdn, modified some.
// Source: https://docs.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=msvc-160

// Uses the __cpuid intrinsic to get information about
// CPU extended instruction set support.

#include <vector>
#include <bitset>
#include <array>
#include <string>
#include <intrin.h>

class InstructionSet
{
public:
    virtual ~InstructionSet();

    static InstructionSet& GetInstance();
    static std::string GetCPU(void) { return m_CpuInfo; }

private:
    InstructionSet();

    int nIds_;
    int nExIds_;
    inline static std::string m_CpuInfo;
    std::vector<std::array<int, 4>> data_;
    std::vector<std::array<int, 4>> extdata_;
};

#endif