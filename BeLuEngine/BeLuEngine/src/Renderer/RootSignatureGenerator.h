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

	void AddStaticSampler(
		D3D12_FILTER filter,
		D3D12_TEXTURE_ADDRESS_MODE adressMode,
		unsigned int shaderRegister,
		unsigned int registerSpace,
		D3D12_COMPARISON_FUNC comparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
		unsigned int maxAnisotropy = 16, // Only if Filter is Anisotropic
		D3D12_STATIC_BORDER_COLOR borderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK // Only if addressMode is Border
		);

	// Generates the RootSignature with values from the m_RootParameters.
	ID3D12RootSignature* Generate(ID3D12Device* device, bool isLocal);

private:
	std::vector<D3D12_ROOT_PARAMETER> m_RootParameters;

	std::vector<D3D12_STATIC_SAMPLER_DESC> m_StaticSamplerDescs;
};

#endif