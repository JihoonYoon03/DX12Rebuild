#pragma once

//열거형을 통해 PSO 전환에 사용
enum class ERenderState
{
	Default = 0,
	Skybox,
	NumStates //상태 종류 카운트
};

class CShader {
public:
	CShader();
	virtual ~CShader();

	HRESULT CompileShaderFromFile(ComPtr<ID3DBlob>& blob, const LPCWSTR fileName, const LPCSTR entryPoint, const LPCSTR version);
	virtual HRESULT CompileShader();
	HRESULT CreateShader(const ComPtr<ID3D12Device>& cpDevice, const ComPtr<ID3D12RootSignature>& cpRootSignature);
	
	//아직 미완
	HRESULT AddPipelineState(
		const ComPtr<ID3D12Device>& cpDevice,
		const ERenderState& state,
		const D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineState
	);

	void SetPipelineState(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList, const ERenderState& state);

protected:

	//PSO
	std::unordered_map<ERenderState, ComPtr<ID3D12PipelineState>> m_cpPipelineState;

	//Shaders
	ComPtr<ID3DBlob>					m_cpVS;
	ComPtr<ID3DBlob>					m_cpPS;

};

class CTextureShader : public CShader
{
	HRESULT CompileShader() override;
};