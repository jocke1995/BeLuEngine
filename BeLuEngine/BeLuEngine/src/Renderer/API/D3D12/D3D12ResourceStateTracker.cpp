#include "stdafx.h"
#include "D3D12ResourceStateTracker.h"

#include "D3D12GraphicsContext.h"
D3D12GlobalStateTracker::D3D12GlobalStateTracker(ID3D12Resource1* resource, unsigned int numSubResources)
{
	BL_ASSERT(resource);
	m_pResource = resource;

	m_ResourceStates.resize(numSubResources);

	// Init to unknownState
	for (unsigned int i = 0; i < numSubResources; i++)
	{
		m_ResourceStates[i] = (D3D12_RESOURCE_STATES)-1;
	}
}

D3D12GlobalStateTracker::~D3D12GlobalStateTracker()
{
}

void D3D12GlobalStateTracker::SetState(D3D12_RESOURCE_STATES resourceState, unsigned int subResource)
{
	unsigned int numSubResources = m_ResourceStates.size();

	if (subResource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
	{
		for (unsigned int i = 0; i < numSubResources; i++)
		{
			m_ResourceStates[i] = resourceState;
		}
	}
	else
	{
		m_ResourceStates[subResource] = resourceState;
	}
}

D3D12_RESOURCE_STATES D3D12GlobalStateTracker::GetState(unsigned int subResource) const
{
	BL_ASSERT(subResource >= 0);
	return m_ResourceStates[subResource];
}

ID3D12Resource1* D3D12GlobalStateTracker::GetNativeResource() const
{
	BL_ASSERT(m_pResource);
	return m_pResource;
}

D3D12LocalStateTracker::D3D12LocalStateTracker(D3D12GraphicsContext* pContextOwner, D3D12GlobalStateTracker* globalStateTracker, ID3D12Resource1* resource, unsigned int numSubResources)
{
	BL_ASSERT(pContextOwner);
	m_pOwner = pContextOwner;

	BL_ASSERT(resource);
	m_pResource = resource;

	BL_ASSERT(numSubResources);
	m_ResourceStates.resize(numSubResources);

	BL_ASSERT(globalStateTracker);
	m_pGlobalStateTracker = globalStateTracker;

	// Init to unknownState
	for (unsigned int i = 0; i < numSubResources; i++)
	{
		m_ResourceStates[i] = (D3D12_RESOURCE_STATES)-1;
	}
}

D3D12LocalStateTracker::~D3D12LocalStateTracker()
{
}

D3D12GlobalStateTracker* D3D12LocalStateTracker::GetGlobalStateTracker() const
{
	BL_ASSERT(m_pGlobalStateTracker);
	return m_pGlobalStateTracker;
}

void D3D12LocalStateTracker::ResolveLocalResourceState(D3D12_RESOURCE_STATES desiredState, unsigned int subResource)
{
	unsigned int numSubResources = m_ResourceStates.size();

	// Maximum number of transitionBarriers that can be done in 1 batch.
	// This would be numMips, which would not likely go over 12 (which would be a 4k texture)
	const unsigned int maxResourceBarriers = 12;
	D3D12_RESOURCE_BARRIER resourceBarriers[maxResourceBarriers];

	unsigned int actualNumberOfTransitionBarriers = 0;

	auto manageTransition = [&](unsigned int subResource)
	{
		// If we crash here, increase the maxTransitionBarriers integer
		BL_ASSERT(subResource < maxResourceBarriers);

		TODO("Do nothing if we're already in the correct state");

		// If we are in unknown state, we add this barrier to be resolved later
		if (m_ResourceStates[subResource] == (D3D12_RESOURCE_STATES)-1)
		{
			// Add to be resolved later
			PendingTransitionBarrier pendingBarrier = {};
			pendingBarrier.localStateTracker = this;
			pendingBarrier.subResource = subResource;
			pendingBarrier.afterState = desiredState;

			m_pOwner->m_PendingResourceBarriers.push_back(pendingBarrier);
		}
		else
		{
			// We know the beforeState, so we can batch it for later recording in this function
			resourceBarriers[actualNumberOfTransitionBarriers].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			resourceBarriers[actualNumberOfTransitionBarriers].Transition.pResource = m_pResource;
			resourceBarriers[actualNumberOfTransitionBarriers].Transition.Subresource = subResource;
			resourceBarriers[actualNumberOfTransitionBarriers].Transition.StateBefore = m_ResourceStates[subResource];
			resourceBarriers[actualNumberOfTransitionBarriers].Transition.StateAfter = desiredState;

			actualNumberOfTransitionBarriers++;
		}

		// Update current state if we use this resource again on the same context
		m_ResourceStates[subResource] = desiredState;
	};

	if (subResource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
	{
		for (unsigned int i = 0; i < numSubResources; i++)
		{
			manageTransition(i);
		}
	}
	else
	{
		manageTransition(subResource);
	}

	m_pOwner->m_pCommandList->ResourceBarrier(actualNumberOfTransitionBarriers, resourceBarriers);
}
