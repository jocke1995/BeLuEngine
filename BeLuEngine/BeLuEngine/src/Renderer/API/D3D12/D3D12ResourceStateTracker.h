#ifndef D3D12RESOURCESTATETRACKER_H
#define D3D12RESOURCESTATETRACKER_H

// This file holds 2 classes.
// GlobalStateTracker && LocalStateTracker
// They work in similar manner, but there is only 1 GlobalStateTracker for each resource that needs it
// But there can be 1 local state tracker for each context that uses that specific resource.

class ID3D12Resource1;
class D3D12GraphicsContext;

class D3D12GlobalStateTracker
{
public:
	D3D12GlobalStateTracker(ID3D12Resource1* resource, unsigned int numSubResources);
	virtual ~D3D12GlobalStateTracker();

	void SetState(D3D12_RESOURCE_STATES resourceState, unsigned int subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	D3D12_RESOURCE_STATES GetState(unsigned int subResource) const;
	ID3D12Resource1* GetNativeResource() const;
private:
	std::vector<D3D12_RESOURCE_STATES> m_ResourceStates = {};
	
	ID3D12Resource1* m_pResource = nullptr;
};

class D3D12LocalStateTracker
{
public:
	D3D12LocalStateTracker(D3D12GraphicsContext* pContextOwner, D3D12GlobalStateTracker* globalStateTracker, ID3D12Resource1* resource, unsigned int numSubResources);
	virtual ~D3D12LocalStateTracker();

	D3D12GlobalStateTracker* GetGlobalStateTracker() const;
	D3D12_RESOURCE_STATES GetState(unsigned int subResource);

	void ResolveLocalResourceState(D3D12_RESOURCE_STATES desiredState, unsigned int subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

private:
	std::vector<D3D12_RESOURCE_STATES> m_ResourceStates = {};
	bool m_SubResourcesIdentical = false;

	ID3D12Resource1* m_pResource = nullptr;

	D3D12GraphicsContext* m_pOwner = nullptr;
	D3D12GlobalStateTracker* m_pGlobalStateTracker = nullptr;
};

#endif