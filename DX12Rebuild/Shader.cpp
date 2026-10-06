#include "pch.h"
#include "Shader.h"

CShader::CShader()
{
}

CShader::~CShader()
{
}

HRESULT CShader::CompileShaderFromFile(ComPtr<ID3DBlob>& blob, const LPCWSTR fileName, const LPCSTR entryPoint, const LPCSTR version)
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

HRESULT CShader::CompileShader()
{
	HRESULT hr;
	hr = CompileShaderFromFile(m_cpVS, L"Shaders.hlsl", "VSMain", "vs_5_0");
	if (FAILED(hr)) return hr;
	hr = CompileShaderFromFile(m_cpPS, L"Shaders.hlsl", "PSMain", "ps_5_0");
	if (FAILED(hr)) return hr;

	return S_OK;
}

HRESULT CShader::CreateShader(ComPtr<ID3D12Device>& cpDevice, ComPtr<ID3D12RootSignature>& cpRootSignature)
{
	//=====================================================================
	//PSO Creation
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	::ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.pRootSignature = cpRootSignature.Get();

	HRESULT hr = CompileShader();
	if (FAILED(hr)) return hr;

	//Shaders
	psoDesc.VS.BytecodeLength = m_cpVS->GetBufferSize();
	psoDesc.VS.pShaderBytecode = m_cpVS->GetBufferPointer();
	psoDesc.PS.BytecodeLength = m_cpPS->GetBufferSize();
	psoDesc.PS.pShaderBytecode = m_cpPS->GetBufferPointer();

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

	//Input Layout
	D3D12_INPUT_ELEMENT_DESC inputElemDesc[3];
	inputElemDesc[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElemDesc[1] = { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElemDesc[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC inputDesc;
	::ZeroMemory(&inputDesc, sizeof(D3D12_INPUT_LAYOUT_DESC));
	inputDesc.NumElements = 3;
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

	hr = cpDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_cpPipelineState.ReleaseAndGetAddressOf()));
	if (FAILED(hr)) { OutputDebugString(L"PSO Creation Failed\n"); return hr; }

	return S_OK;
}

void CShader::SetPipelineState(ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	cpCommandList->SetPipelineState(m_cpPipelineState.Get());
}
