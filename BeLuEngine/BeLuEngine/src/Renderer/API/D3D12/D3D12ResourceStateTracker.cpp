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

D3D12_RESOURCE_STATES D3D12LocalStateTracker::GetState(unsigned int subResource)
{
	BL_ASSERT(subResource >= 0);
	return m_ResourceStates[subResource];
}

void D3D12LocalStateTracker::ResolveLocalResourceState(D3D12_RESOURCE_STATES desiredState, unsigned int subResource)
{
	unsigned int numSubResources = m_ResourceStates.size();

	// Maximum number of transitionBarriers that can be done in 1 batch.
	// This would be numMips, which would not likely go over 12 (which would be a 4k texture)
	const unsigned int maxResourceBarriers = 12;
	D3D12_RESOURCE_BARRIER resourceBarriers[maxResourceBarriers];

	unsigned int actualNumberOfResourceBarriers = 0;

	auto manageTransition = [&](unsigned int subResourceIndex)
	{
		// If we crash here, increase the maxTransitionBarriers integer
		BL_ASSERT(subResourceIndex < maxResourceBarriers);

		// Do nothing if we're already in the desired state
		if (m_ResourceStates[subResourceIndex] == desiredState)
			return;

		// If we are in unknown state, we add this barrier to be resolved later
		if (m_ResourceStates[subResourceIndex] == (D3D12_RESOURCE_STATES)-1)
		{
			// Add to be resolved later
			PendingTransitionBarrier pendingBarrier = {};
			pendingBarrier.localStateTracker = this;
			pendingBarrier.subResource = subResourceIndex;
			pendingBarrier.afterState = desiredState;

			m_pOwner->m_PendingResourceBarriers.push_back(pendingBarrier);
		}
		else
		{
			// We know the beforeState, so we can batch it for later recording in this function
			resourceBarriers[actualNumberOfResourceBarriers].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			resourceBarriers[actualNumberOfResourceBarriers].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			resourceBarriers[actualNumberOfResourceBarriers].Transition.pResource = m_pResource;
			resourceBarriers[actualNumberOfResourceBarriers].Transition.Subresource = subResourceIndex;
			resourceBarriers[actualNumberOfResourceBarriers].Transition.StateBefore = m_ResourceStates[subResourceIndex];
			resourceBarriers[actualNumberOfResourceBarriers].Transition.StateAfter = desiredState;

			actualNumberOfResourceBarriers++;
		}

		// Update current state if we use this resource again on the same context
		m_ResourceStates[subResourceIndex] = desiredState;
	};

	if (subResource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
	{
		// Check if the subresources are identical, if they are, they might be able to be batched
		m_SubResourcesIdentical = true;
		for (unsigned int i = 0; i < numSubResources; i++)
		{
			// Can't batch them anyways if we don't know the beforeState yet.
			if (m_ResourceStates[i] == (D3D12_RESOURCE_STATES)-1)
			{
				m_SubResourcesIdentical = false;
				break;
			}
			else
			{
				if (m_ResourceStates[0] != m_ResourceStates[i])
				{
					// Found a missmatch, all subresources are not identical
					m_SubResourcesIdentical = false;
					break;
				}
			}
		}

		// Check if we can do them all in one batch
		if (m_SubResourcesIdentical == true)
		{
			// Do nothing if we're already in the desired state
			if (m_ResourceStates[0] == desiredState)
				return;

			resourceBarriers[actualNumberOfResourceBarriers].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			resourceBarriers[actualNumberOfResourceBarriers].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			resourceBarriers[actualNumberOfResourceBarriers].Transition.pResource = m_pResource;
			resourceBarriers[actualNumberOfResourceBarriers].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			resourceBarriers[actualNumberOfResourceBarriers].Transition.StateBefore = m_ResourceStates[0];
			resourceBarriers[actualNumberOfResourceBarriers].Transition.StateAfter = desiredState;

			actualNumberOfResourceBarriers++;
		}
		else
		{
			for (unsigned int i = 0; i < numSubResources; i++)
			{
				manageTransition(i);
			}
		}
	}
	else
	{
		manageTransition(subResource);
	}

	// Do immediate batched transitions if we know the before state of this subResource
	// This can happen the second (and forth) time a subResource is accessed in this context
	if (actualNumberOfResourceBarriers > 0)
	{
		m_pOwner->m_pCommandList->ResourceBarrier(actualNumberOfResourceBarriers, resourceBarriers);
	}
}
