#include "stdafx.h"
#include "MicrosoftCPU.h"

InstructionSet::~InstructionSet()
{
}

InstructionSet& InstructionSet::GetInstance()
{
	static InstructionSet instance;

	return instance;
}

InstructionSet::InstructionSet()
{
    std::array<int, 4> cpui;

    // Calling __cpuid with 0x0 as the function_id argument
    // gets the number of the highest valid function ID.
    __cpuid(cpui.data(), 0);
    nIds_ = cpui[0];

    for (int i = 0; i <= nIds_; ++i)
    {
        __cpuidex(cpui.data(), i, 0);
        data_.push_back(cpui);
    }

    // Calling __cpuid with 0x80000000 as the function_id argument
    // gets the number of the highest valid extended ID.
    __cpuid(cpui.data(), 0x80000000);
    nExIds_ = cpui[0];

    char brand[0x40];
    memset(brand, 0, sizeof(brand));

    for (int i = 0x80000000; i <= nExIds_; ++i)
    {
        __cpuidex(cpui.data(), i, 0);
        extdata_.push_back(cpui);
    }

    // Interpret CPU brand string if reported
    if (nExIds_ >= 0x80000004)
    {
        memcpy(brand, extdata_[2].data(), sizeof(cpui));
        memcpy(brand + 16, extdata_[3].data(), sizeof(cpui));
        memcpy(brand + 32, extdata_[4].data(), sizeof(cpui));
        m_CpuInfo = brand;
    }
}
