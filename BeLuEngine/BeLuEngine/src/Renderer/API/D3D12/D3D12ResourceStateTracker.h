#ifndef D3D12RESOURCESTATETRACKER_H
#define D3D12RESOURCESTATETRACKER_H

// This file holds 2 classes.
// GlobalStateTracker && LocalStateTracker
// They work in similar manner, but there is only 1 GlobalStateTracker for each resource that needs it
// But there can be 1 local state tracker for each context that uses that specific resource.

struct PendingTransitionBarrier
{
	D3D12LocalStateTracker* localStateTracker = nullptr;
	unsigned int subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	D3D12_RESOURCE_STATES afterState = (D3D12_RESOURCE_STATES)-1;
};

class ID3D12Resource1;
class ID3D12GraphicsCommandList;

class D3D12GlobalStateTracker
{
public:
	D3D12GlobalStateTracker(ID3D12Resource1* resource, unsigned int numSubResources);
	virtual ~D3D12GlobalStateTracker();

	void SetState(D3D12_RESOURCE_STATES state, unsigned int subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
private:
	std::vector<D3D12_RESOURCE_STATES> m_ResourceStates = {};
	
	ID3D12Resource1* m_pResource = nullptr;
};

class D3D12LocalStateTracker
{
public:
	D3D12LocalStateTracker(ID3D12Resource1* resource, unsigned int numSubResources);
	virtual ~D3D12LocalStateTracker();

	void ResolveLocalResourceState(D3D12_RESOURCE_STATES desiredState, std::vector<PendingTransitionBarrier>& m_PendingResourceBarriers, ID3D12GraphicsCommandList* commandList, unsigned int subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
private:
	std::vector<D3D12_RESOURCE_STATES> m_ResourceStates = {};

	ID3D12Resource1* m_pResource = nullptr;
};

#endif