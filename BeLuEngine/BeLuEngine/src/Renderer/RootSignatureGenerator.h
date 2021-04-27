#ifndef ROOTSIGNATUREGENERATOR_H
#define ROOTSIGNATUREGENERATOR_H

#include <vector>

struct ID3D12RootSignature;
struct ID3D12Device;

class RootSignatureGenerator
{
public:
	void AddDescriptorTableRanges(const std::vector<D3D12_DESCRIPTOR_RANGE>& ranges);

	void AddRootDescriptor(
		D3D12_ROOT_PARAMETER_TYPE descriptorType,
		unsigned int shaderRegister,
		unsigned int registerSpace,
		D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL);

	void AddRootConstant(
		unsigned int shaderRegister,
		unsigned int registerSpace,
		unsigned int numRootConstants ,
		D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL);

	// Generates the RootSignature with values from the m_RootParameters.
	ID3D12RootSignature* Generate(ID3D12Device* device, bool isLocal);

private:
	std::vector<D3D12_ROOT_PARAMETER> m_RootParameters;
};

#endif