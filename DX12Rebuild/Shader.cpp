#include "pch.h"
#include "Shader.h"


D3D12_GRAPHICS_PIPELINE_STATE_DESC CreateDefaultPipelineState(const D3D12_INPUT_ELEMENT_DESC* inputElemDesc)
{
	//PSO Creation
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	::ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	//BlendDesc
	psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
	psoDesc.BlendState.IndependentBlendEnable = FALSE;
	psoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
	psoDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
	psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	psoDesc.SampleMask = 0xffffffff;

	//RS
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
	psoDesc.RasterizerState.DepthBias = 0;
	psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	psoDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
	psoDesc.RasterizerState.DepthClipEnable = TRUE;
	psoDesc.RasterizerState.MultisampleEnable = FALSE;
	psoDesc.RasterizerState.AntialiasedLineEnable = TRUE;
	psoDesc.RasterizerState.ForcedSampleCount = 0;
	psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	//Depth Stencil
	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.DepthStencilState.StencilReadMask = 0x00;
	psoDesc.DepthStencilState.StencilWriteMask = 0x00;
	psoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	psoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	D3D12_INPUT_LAYOUT_DESC inputDesc;
	::ZeroMemory(&inputDesc, sizeof(D3D12_INPUT_LAYOUT_DESC));
	inputDesc.NumElements = 4;
	inputDesc.pInputElementDescs = inputElemDesc;

	psoDesc.InputLayout = inputDesc;

	//Triangle Strip Cut
	psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	psoDesc.NumRenderTargets = 1;

	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//Multisample
	psoDesc.SampleDesc.Count = 1;

	psoDesc.NodeMask = 0;

	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	return psoDesc;
}

HRESULT CompileShaderFromFile(ComPtr<ID3DBlob>& blob, const LPCWSTR fileName, const LPCSTR entryPoint, const LPCSTR version)
{
	UINT nCompileFlags = 0;

#if defined _DEBUG
	nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ComPtr<ID3DBlob> errBlob;
	HRESULT hr;
	hr = D3DCompileFromFile(fileName, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, version, nCompileFlags, NULL, blob.ReleaseAndGetAddressOf(), errBlob.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		if (errBlob)
			OutputDebugStringA((char*)errBlob->GetBufferPointer());
		return hr;
	}

	return S_OK;
}


//----------------------------------
// 
// CShader
// 
//----------------------------------
CShader::CShader()
{
}

CShader::~CShader()
{
}

HRESULT CShader::CreateRootSignature(const ComPtr<ID3D12Device>& cpDevice)
{
	D3D12_DESCRIPTOR_RANGE descRange;
	descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descRange.NumDescriptors = 1;
	descRange.BaseShaderRegister = 0;
	descRange.RegisterSpace = 0;
	descRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParams[4];

	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParams[0].Constants.ShaderRegister = 0;
	rootParams[0].Constants.RegisterSpace = 0;
	rootParams[0].Constants.Num32BitValues = 48;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[1].DescriptorTable.pDescriptorRanges = &descRange;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParams[2].Constants.ShaderRegister = 1;
	rootParams[2].Constants.RegisterSpace = 0;
	rootParams[2].Constants.Num32BitValues = 5;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	//노멀맵용. 나중에 단순화 필요
	D3D12_DESCRIPTOR_RANGE descRange2;
	descRange2.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descRange2.NumDescriptors = 1;
	descRange2.BaseShaderRegister = 1;
	descRange2.RegisterSpace = 0;
	descRange2.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[3].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[3].DescriptorTable.pDescriptorRanges = &descRange2;
	rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc;
	::ZeroMemory(&rootSigDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));
	rootSigDesc.NumParameters = 4;
	rootSigDesc.pParameters = rootParams;
	rootSigDesc.NumStaticSamplers = 1;
	rootSigDesc.pStaticSamplers = &samplerDesc;
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> cpd3dBlob, cpd3dErrBlob;
	HRESULT hr = D3D12SerializeRootSignature(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		cpd3dBlob.GetAddressOf(),
		cpd3dErrBlob.GetAddressOf()
	);
	if (FAILED(hr))
	{
		if (cpd3dErrBlob)
			OutputDebugStringA((char*)cpd3dErrBlob->GetBufferPointer());
		return hr;
	}

	hr = cpDevice->CreateRootSignature(
		0,
		cpd3dBlob->GetBufferPointer(),
		cpd3dBlob->GetBufferSize(),
		IID_PPV_ARGS(m_cpRootSignature.GetAddressOf())
	);
	if (FAILED(hr)) { OutputDebugString(L"CreateRootSignature(): CreateRootSignature() Failed\n"); return hr; }

	return S_OK;
}

HRESULT CShader::CreateShader(const ComPtr<ID3D12Device>& cpDevice)
{
	HRESULT hr = CreateRootSignature(cpDevice);
	if (FAILED(hr)) return hr;

	//Input Layout
	static D3D12_INPUT_ELEMENT_DESC inputElemDesc[4];
	inputElemDesc[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElemDesc[1] = { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElemDesc[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElemDesc[3] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = CreateDefaultPipelineState(inputElemDesc);
	psoDesc.pRootSignature = m_cpRootSignature.Get();

	ComPtr<ID3DBlob> cpVS;
	ComPtr<ID3DBlob> cpPS;
	ComPtr<ID3DBlob> cpPSTex;
	ComPtr<ID3DBlob> cpPSSky;
	ComPtr<ID3DBlob> cpPSUnlit;

	hr = CompileShaderFromFile(cpVS, L"Shaders.hlsl", "VSMain", "vs_5_0");
	if (FAILED(hr)) return hr;
	hr = CompileShaderFromFile(cpPS, L"Shaders.hlsl", "PSMain", "ps_5_0");
	if (FAILED(hr)) return hr;
	hr = CompileShaderFromFile(cpPSTex, L"Shaders.hlsl", "PSTex", "ps_5_0");
	if (FAILED(hr)) return hr;
	hr = CompileShaderFromFile(cpPSSky, L"Shaders.hlsl", "PSSky", "ps_5_0");
	if (FAILED(hr)) return hr;
	hr = CompileShaderFromFile(cpPSUnlit, L"Shaders.hlsl", "PSUnlit", "ps_5_0");
	if (FAILED(hr)) return hr;

	//Shaders
	psoDesc.VS.BytecodeLength = cpVS->GetBufferSize();
	psoDesc.VS.pShaderBytecode = cpVS->GetBufferPointer();
	psoDesc.PS.BytecodeLength = cpPS->GetBufferSize();
	psoDesc.PS.pShaderBytecode = cpPS->GetBufferPointer();

	m_cpPipelineState[ERenderState::Default] = ComPtr<ID3D12PipelineState>{};
	hr = cpDevice->CreateGraphicsPipelineState(
		&psoDesc,
		IID_PPV_ARGS(m_cpPipelineState[ERenderState::Default].ReleaseAndGetAddressOf())
	);
	if (FAILED(hr)) { OutputDebugString(L"PSO Creation Failed\n"); return hr; }

	m_cpPipelineState[ERenderState::Texture] = ComPtr<ID3D12PipelineState>{};
	psoDesc.PS.BytecodeLength = cpPSTex->GetBufferSize();
	psoDesc.PS.pShaderBytecode = cpPSTex->GetBufferPointer();
	hr = cpDevice->CreateGraphicsPipelineState(
		&psoDesc,
		IID_PPV_ARGS(m_cpPipelineState[ERenderState::Texture].ReleaseAndGetAddressOf())
	);
	if (FAILED(hr)) { OutputDebugString(L"Tex PSO Creation Failed\n"); return hr; }

	//Skybox용 PSO
	m_cpPipelineState[ERenderState::Skybox] = ComPtr<ID3D12PipelineState>{};
	psoDesc.PS.BytecodeLength = cpPSSky->GetBufferSize();
	psoDesc.PS.pShaderBytecode = cpPSSky->GetBufferPointer();
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	hr = cpDevice->CreateGraphicsPipelineState(
		&psoDesc, 
		IID_PPV_ARGS(m_cpPipelineState[ERenderState::Skybox].ReleaseAndGetAddressOf())
	);
	if (FAILED(hr)) { OutputDebugString(L"Sky PSO Creation Failed\n"); return hr; }

	//Unlit용 PSO
	m_cpPipelineState[ERenderState::Unlit] = ComPtr<ID3D12PipelineState>{};
	psoDesc.PS.BytecodeLength = cpPSUnlit->GetBufferSize();
	psoDesc.PS.pShaderBytecode = cpPSUnlit->GetBufferPointer();
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	hr = cpDevice->CreateGraphicsPipelineState(
		&psoDesc,
		IID_PPV_ARGS(m_cpPipelineState[ERenderState::Unlit].ReleaseAndGetAddressOf())
	);
	if (FAILED(hr)) { OutputDebugString(L"Sky PSO Creation Failed\n"); return hr; }

	return S_OK;
}

HRESULT CShader::AddPipelineState(const ComPtr<ID3D12Device>& cpDevice, const ERenderState& state, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineState)
{
	if (!m_cpPipelineState.at(state)) {
		//Unlit용 PSO
		m_cpPipelineState[state] = ComPtr<ID3D12PipelineState>{};
		HRESULT hr = cpDevice->CreateGraphicsPipelineState(
			&pipelineState,
			IID_PPV_ARGS(m_cpPipelineState[ERenderState::Unlit].ReleaseAndGetAddressOf())
		);
		if (FAILED(hr)) { OutputDebugString(L"Add PSO Failed\n"); return hr; }
	}
}

void CShader::SetGraphicsRootSignature(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	cpCommandList->SetGraphicsRootSignature(m_cpRootSignature.Get());
}

void CShader::SetPipelineState(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList, const ERenderState& state)
{
	try
	{
		ComPtr<ID3D12PipelineState>& PSO = m_cpPipelineState.at(state);
		cpCommandList->SetPipelineState(PSO.Get());
	}
	catch (...)
	{
		//해당 상태 PSO 존재하지 않음
		return;
	}
}