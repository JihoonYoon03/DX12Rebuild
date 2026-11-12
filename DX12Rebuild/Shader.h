#pragma once

//열거형을 통해 PSO 전환에 사용
enum class ERenderState
{
	Default = 0,
	Texture,
	Skybox,
	Unlit,
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	UI,
	//-----------------------------------
	NormalMap,
	NumStates //상태 종류 카운트
};

struct CB_SceneInfo
{
	XMFLOAT4X4 view;
	XMFLOAT4X4 proj;
	XMFLOAT4X4 viewSky;
};

//Root Signature와 Shader Bytecode 지정 필요
D3D12_GRAPHICS_PIPELINE_STATE_DESC CreateDefaultPipelineState(const D3D12_INPUT_ELEMENT_DESC* inputElemDesc);
HRESULT CompileShaderFromFile(ComPtr<ID3DBlob>& blob, const LPCWSTR fileName, const LPCSTR entryPoint, const LPCSTR version);

class CShader {
public:
	CShader();
	virtual ~CShader();

	HRESULT CreateCBV(
		const ComPtr<ID3D12Device>& cpDevice,
		const ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
		CDescHeapAllocator& descHeapAllocator
	);
	HRESULT CreateRootSignature(const ComPtr<ID3D12Device>& cpDevice);
	HRESULT CreateShader(
		const ComPtr<ID3D12Device>& cpDevice,
		const ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
		CDescHeapAllocator& descHeapAllocator
	);
	
	//아직 미완
	HRESULT AddPipelineState(
		const ComPtr<ID3D12Device>& cpDevice,
		const ERenderState& state,
		const D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineState
	);

	void SetGraphicsRootSignature(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList);
	void SetPipelineState(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList, const ERenderState& state);
	//View/Proj/ViewSky
	void SetSceneMatrix(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList, const CB_SceneInfo& data, bool bHud = false);

protected:
	ComPtr<ID3D12RootSignature>		m_cpRootSignature;
	//PSO
	std::unordered_map<ERenderState, ComPtr<ID3D12PipelineState>> m_cpPipelineState;

	//CBV: View/Proj/ViewSky Matrix Buffer
	DescHeapInfo					m_descHeapInfo;
	DescHeapInfo					m_hudDescHeapInfo;
	ComPtr<ID3D12Resource>			m_cpSceneInfoBuffer;
	UINT8*							m_mappedSceneInfoBuffer = nullptr;
};