#pragma once

//열거형을 통해 PSO 전환에 사용
enum class ERenderState
{
	Default = 0,
	Texture,
	Skybox,
	Unlit,
	NumStates //상태 종류 카운트
};

//Root Signature와 Shader Bytecode 지정 필요
D3D12_GRAPHICS_PIPELINE_STATE_DESC CreateDefaultPipelineState(const D3D12_INPUT_ELEMENT_DESC* inputElemDesc);
HRESULT CompileShaderFromFile(ComPtr<ID3DBlob>& blob, const LPCWSTR fileName, const LPCSTR entryPoint, const LPCSTR version);

class CShader {
public:
	CShader();
	virtual ~CShader();

	HRESULT CreateRootSignature(const ComPtr<ID3D12Device>& cpDevice);
	HRESULT CreateShader(const ComPtr<ID3D12Device>& cpDevice);
	
	//아직 미완
	HRESULT AddPipelineState(
		const ComPtr<ID3D12Device>& cpDevice,
		const ERenderState& state,
		const D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineState
	);

	void SetGraphicsRootSignature(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList);
	void SetPipelineState(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList, const ERenderState& state);

protected:
	ComPtr<ID3D12RootSignature>		m_cpRootSignature;
	//PSO
	std::unordered_map<ERenderState, ComPtr<ID3D12PipelineState>> m_cpPipelineState;
};