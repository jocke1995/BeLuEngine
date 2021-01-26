#include "stdafx.h"
#include "View.h"

#include "Resource.h"
#include "../DescriptorHeap.h"

View::View(DescriptorHeap* descriptorHeap, const Resource* const resource)
{
    m_pResource = const_cast<Resource*>(resource);
    m_DescriptorHeapIndex = descriptorHeap->GetNextDescriptorHeapIndex(1);
}

View::~View()
{
}

Resource* const View::GetResource() const
{
    return m_pResource;
}

const unsigned int View::GetDescriptorHeapIndex() const
{
    return m_DescriptorHeapIndex;
}
