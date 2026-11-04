#pragma once
class CShader {
public:
	CShader();
	virtual ~CShader();

	HRESULT CompileShaderFromFile(ComPtr<ID3DBlob>& blob, const LPCWSTR fileName, const LPCSTR entryPoint, const LPCSTR version);
	virtual HRESULT CompileShader();
	HRESULT CreateShader(const ComPtr<ID3D12Device>& cpDevice, const ComPtr<ID3D12RootSignature>& cpRootSignature);

	void SetPipelineState(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList);

protected:
	//PSO
	ComPtr<ID3D12PipelineState>			m_cpPipelineState;

	//Shaders
	ComPtr<ID3DBlob>					m_cpVS;
	ComPtr<ID3DBlob>					m_cpPS;

};

class CTextureShader : public CShader
{
	HRESULT CompileShader();
};