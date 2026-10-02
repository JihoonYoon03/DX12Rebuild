# DX12 Rebuild 학습 인수인계

최종 정리: 2026-10-03. 이 문서는 다른 컴퓨터/새 대화에서 학습을 이어가기 위한 기록이다. 실제 코드는 이후 변경될 수 있으므로 다음 조력자는 작업 전에 현재 파일을 확인한다.

**최신 진행 기준은 12절이다.** 3~6절, 9~11절은 이전 시점의 기록이다. PSO/정점 버퍼 미구현, UPLOAD 버퍼 직접 사용, 현재 과제 등의 설명이 최신 상태와 다르면 12절을 우선한다.

## 1. 가장 중요한 협업 규칙

- 사용자가 직접 구현한다. AI는 개념 설명, 단계별 가이드, 질문, 코드 리뷰를 담당한다. 요청 없이 완성 코드나 붙여 넣을 답안을 제공하거나 소스를 수정하지 않는다.
- 한 번에 한 학습 단계만 진행한다. 이해 확인 질문에 답하고 코드를 작성하면 검토한다.
- 빌드, 실행, 긴 테스트 등 오래 걸리는 작업은 기본적으로 수행하지 않는다. 필요하면 사용자가 실행하도록 요청한다. 가벼운 파일/차이 검토는 가능하다.
- 별도의 빌드 환경 문제 등에 대해 사용자가 명시적으로 수정을 요청한 경우에만 해당 범위를 직접 고칠 수 있다. 학습 과제 전체의 구현 권한으로 확대하지 않는다.
- 한국어로 설명한다. 사용자는 DX12 프레임워크를 배우는 중이다.
- Review의 프레임워크 구조를 그대로 복제하지 않는다. ComPtr 등 현대 C++ 편의 기능을 적극 사용하되, shared_ptr을 모든 객체에 무조건 적용하지 않는다.
- 기존 수정 내용을 보존한다. 현재 코드를 읽지 않고 이전 리뷰가 여전히 유효하다고 단정하지 않는다.

## 2. 목표와 프로젝트

- 목표: Review 프로젝트의 DescriptorHeap 브랜치를 참고하여 Rebuild에서 DDS 파일을 텍스처로 로드하고 화면에 표시하기까지 학습한다.
- 참고 프로젝트는 별도로 준비해야 한다. 노트북에서 Review의 실제 경로와 브랜치를 확인할 것. 이 문서가 Review의 최신 코드를 검증했다는 의미는 아니다.
- Rebuild는 의도적으로 미완성인 학습 프로젝트다.
- 솔루션: `DX12Framework.sln`
- 프로젝트: `DX12Rebuild/DX12Rebuild.vcxproj`
- 주요 파일: `GameFramework.h/.cpp`, `WinMain.h/.cpp`, `Timer.h/.cpp`, `pch.h`, `Shaders.hlsl` (모두 DX12Rebuild 폴더 안).
- 과거 솔루션/진입 파일 이름은 DX12Rebuild였으나 현재 솔루션은 DX12Framework, 진입 파일은 WinMain이다.

## 3. 지금까지의 진행

다음은 학습 및 코드 리뷰 완료 범위다. 모든 항목이 자동 테스트나 실제 실행으로 검증되었다는 뜻은 아니다.

1. 디버그 계층, DXGI/D3D12 디버그 인터페이스 및 링크 의존성.
2. Adapter 열거, Device 생성, WARP 대체 경로, ComPtr 출력 매개변수 처리.
3. DIRECT CommandQueue / CommandAllocator / CommandList 생성과 HRESULT 처리.
4. SwapChain 생성, 실제 클라이언트 영역 크기, 초기 RTV/DSV 생성.
5. Win32 메시지 루프, 초기화 순서, WM_QUIT 및 메시지 전달.
6. Timer 초기화/Reset/샘플 관리, FPS 산출, QPC 기반 타이머 이해.
7. Fence 동기화. 단일 Fence에 단조 증가하는 하나의 UINT64 값을 사용한다. 현재는 매 프레임 GPU 완료를 기다리는 단순 구조이며 Allocator도 하나다.
8. 창 모드 Resize: GPU 완료 대기 → 기존 백 버퍼/깊이 버퍼 Reset → ResizeBuffers → 현재 버퍼 인덱스 조회 → RTV/DSV 재생성. 힙 자체는 재사용한다. 초기화 전 WM_SIZE 및 최소화를 처리한다.
9. Viewport/Scissor: CPU 구조체 갱신과 매 프레임 명령 리스트 설정을 구분한다.
10. 빈 Root Signature: 설명 작성 → 직렬화 → 객체 생성 → 명령 리스트 설정. 각 HRESULT 처리 완료.
11. HLSL VS/PS 작성 및 D3DCompileFromFile 컴파일 함수 코드 리뷰 완료.

사용자 실행 확인: 창 크기 변경, 최대화, 최소화 후 복원에서도 배경색이 정상 표시되었다. 전체화면은 비활성화 상태라 테스트하지 않았다. 최근 셰이더 단계는 코드 검토 결과이며 실행 성공을 별도로 확정하지 않았다.

## 4. 현재 코드의 핵심 구성

- Framework가 Device, SwapChain, Queue, Allocator, List, Fence, 백 버퍼, 깊이 버퍼 등을 ComPtr로 소유한다.
- 백 버퍼 2개. 컬러 형식 R8G8B8A8_UNORM, 깊이/스텐실 형식 D24_UNORM_S8_UINT.
- 실제 백 버퍼/깊이 버퍼 SampleDesc는 Count=1, Quality=0이다. MSAA 지원 여부 조회 변수와 혼동하지 않는다.
- FrameAdvance는 Clear와 Present까지 실행한다. 아직 정점 버퍼와 Draw로 삼각형을 출력하는 단계는 아니다.
- m_bStopRender로 최소화/Resize 실패 시 렌더링을 중단한다. Resize 성공 시 재개한다.
- Viewport/Scissor 갱신 함수는 SetViewportScissorRect(const RECT&)이다.
- 사용자는 Resize 시작 부분의 갱신 호출 위치를 유지하기로 했다. 최소화/실패 후 렌더링이 중단되는 현재 흐름을 고려해 이를 필수 버그로 반복 지적하지 않는다.
- m_cpRootSignature는 매 프레임 CommandList Reset 후 SetGraphicsRootSignature로 연결한다.
- Shaders.hlsl: VS 입력은 float3 POSITION과 float4 COLOR. VS는 위치에 w=1을 붙여 SV_Position으로 출력하고 색상을 전달한다. PS는 색상을 SV_Target으로 반환한다. 아직 행렬/텍스처를 사용하지 않는다.
- CompileShaderFromFile은 VSMain/vs_5_0, PSMain/ps_5_0을 각각 컴파일하여 m_cpVS, m_cpPS에 저장한다.
- 컴파일 옵션 기본값 0, 오류 Blob null 검사, 두 번째 오류 Blob 출력의 ReleaseAndGetAddressOf, 성공 시 S_OK 반환을 확인했다.
- m_cpPipelineState 멤버는 이미 있지만, 인수인계 시점에는 CreateGraphicsPipelineState 구현이 없다.

## 5. 현재 과제: 입력 레이아웃과 PSO

이미 사용자에게 아래 과제를 제시했다. 다음 대화에서는 구현 여부부터 확인하고 리뷰한다. 아직 사용자의 이해 확인 답변은 받지 않았다.

### 구현 범위

- HRESULT 반환 PSO 생성 함수를 추가한다.
- 앞으로 정점 하나를 XMFLOAT3 위치 → XMFLOAT4 색상 순서로 저장할 예정이다.
- D3D12_INPUT_ELEMENT_DESC 두 개: POSITION/COLOR, 인덱스 0, 슬롯 0, 정점별 데이터, InstanceDataStepRate=0. 각 DXGI_FORMAT과 색상의 바이트 오프셋은 사용자가 계산한다.
- D3D12_GRAPHICS_PIPELINE_STATE_DESC에 기존 루트 시그니처, VS/PS Blob 내부 주소·크기, 입력 레이아웃을 연결한다.
- PrimitiveTopologyType은 삼각형 계열. 렌더 타깃 1개. RTV/DSV 형식과 샘플 설정은 실제 리소스와 일치시킨다. SampleMask는 모든 샘플을 허용한다.
- Rasterizer: 면 채우기, 초기에는 컬링 끄기, 깊이 클리핑 켜기.
- Blend: 블렌딩/논리 연산 끄기, RGBA 쓰기 모두 허용.
- DepthStencil: 깊이 테스트/쓰기 켜기, LESS 비교, 스텐실 끄기.
- 구조체를 0으로 초기화하는 것만으로 유효한 기본 상태가 만들어지는 것은 아니다. 관련 열거형과 설정을 유효하게 채운다.
- CreateGraphicsPipelineState 결과를 기존 m_cpPipelineState로 받고 실패를 전파한다.
- OnCreate에서 루트 시그니처와 셰이더 준비 이후 호출한다.
- FrameAdvance의 CommandList Reset 두 번째 인자로 PSO를 설정하는 방법을 학습한다. 별도 SetGraphicsRootSignature는 유지한다.
- 정점 버퍼와 Draw 호출은 다음 단계. 이번에도 배경색만 표시되는 것이 정상이다.

### 사용자에게 제시한 질문

1. HLSL에 float3/float4가 있는데도 입력 레이아웃이 필요한 이유는?
2. 위치 다음에 색상을 저장할 때 색상의 바이트 오프셋은?
3. PSO의 RTV 형식과 백 버퍼 형식을 맞추는 이유는?
4. 색상 쓰기 마스크가 0이면 PS가 색상을 반환해도 표시되는가?

## 6. 남은 계획: 현재 포함 7단계

셰이더 단계 시작 시 총 8단계로 안내했고, 그중 셰이더 코드 검토가 완료되었다. 아래는 유동적인 학습 계획이다.

1. 입력 레이아웃과 PSO 생성 — 현재 과제.
2. 정점 버퍼 생성과 첫 삼각형 출력. 이후 뷰포트/시저의 시각적 효과 확인.
3. Mesh/Shader 역할 분리와 리소스 수명 정리.
4. 상수 버퍼와 변환 행렬 연결.
5. UV 좌표와 텍스처용 루트 시그니처, SRV 힙, 샘플러 구성.
6. DDS 로딩과 GPU 업로드, 업로드 완료 동기화.
7. 텍스처 바인딩/출력 및 남은 오류 경로 점검.

전체화면, 카메라 이동, 프레임 병렬화는 현재 필수 목표 밖이다. 복잡하면 단계를 더 나누되 한 번에 여러 단계를 구현하도록 요구하지 않는다.

## 7. 나중에 점검할 사항

- 프로젝트의 Shaders.hlsl은 여전히 FxCompile로 등록되어 있지만 Debug/x64에는 ExcludedFromBuild=true가 설정되어 있어 해당 구성에서는 빌드 시 셰이더 컴파일이 제외된다. VSMain/Vertex/5.0 설정은 남아 있다. 다른 세 구성에는 명시적인 빌드 제외 설정이 없다. None 항목 전환은 아직 반영되지 않았으며, 런타임 컴파일과의 중복 여부는 구성별로 구분한다.
- CommandAllocator/CommandList Reset 실패는 로그 후 해당 프레임에서 return한다. 프로그램 종료는 아니므로 다음 프레임에 재시도한다. 최종 오류 처리 단계에서 지속 실패 정책을 정리한다.
- Present 반환값 처리와 Device removed 경로 등은 최종 오류 경로 점검에서 다룬다.
- 종료 시 ReportLiveObjects가 Framework의 ComPtr 멤버 해제보다 먼저 실행된다. 이때의 Live 경고만으로 누수라고 단정하지 않는다. 리소스 수명 단계에서 GPU 완료, 자원 해제, 최종 보고 순서를 정리한다.
- 초기화 도중 실패한 경우 HANDLE 등 부분 생성 자원의 정리도 리소스 수명 단계에서 검토한다.
- Adapter 열거 등 나머지 API 실패 경로는 현재 파일을 다시 읽고 판단한다. 지나간 설명만 근거로 정상이라고 보증하지 않는다.
- 공유 Fence에 백 버퍼별 독립 번호를 다시 도입하지 않는다. 프레임 병렬화를 하려면 전역 증가값과 각 프레임이 기다릴 값을 구분하는 별도 설계가 필요하다.

## 8. 노트북 환경과 빌드 이력

- Windows에서 Visual Studio C++ 도구와 Windows SDK를 준비한다. 프로젝트는 현재 PlatformToolset v145를 지정한다. 노트북에 해당 도구가 있는지 확인하고 무단으로 도구 집합 버전을 바꾸지 않는다.
- 코드는 Git으로 이동한다. 미커밋/미추적 파일, 특히 HLSL과 이 문서도 빠뜨리지 않는다. 이 문서 작성 자체는 커밋이나 푸시를 수행하지 않는다.
- 참고 Review 프로젝트도 별도로 받아야 한다.
- .vs, obj, x64/Debug, PCH 등 로컬 중간 산출물은 옮기지 않고 노트북에서 새로 만든다.
- 최근 빌드 로그에 C1853(PCH 호환 문제)와 MSB8028(중간 폴더 공유 경고)이 있었다. 사용자 요청으로 프로젝트 IntDir만 `$(ProjectDir)obj\$(ProjectName)\$(Platform)\$(Configuration)\`로 변경했다. 학습 소스는 수정하지 않았다. 이 변경 후 실제 빌드 성공은 별도로 확인되지 않았다.
- Shaders.hlsl 상대 경로는 프로그램 작업 디렉터리 기준이다. 노트북에서 실행 시 작업 디렉터리와 HLSL 위치를 확인한다.
- 대화 자동 동기화를 전제로 하지 않는다. 새 대화에서 이 문서를 읽도록 요청하면 된다.

## 9. 새 대화 시작용 문구

> LEARNING_PROGRESS.md의 12절을 우선 읽고 현재 코드를 확인해줘. Mesh 분리와 DEFAULT 버퍼 업로드 구현까지 진행했고 삼각형 정상 출력을 확인했어. 다음은 Shader 분리 단계야. 코드는 내가 작성하고 너는 가이드와 리뷰만 담당해. 완성 코드 제공이나 임의 수정은 하지 말고, 빌드와 실행은 내가 하도록 요청해. 한 단계씩 이어가자.

## 10. 현재 파일 재점검 (2026-09-28)

- 대상: `C:\Users\user\Documents\GitHub\DX12Rebuild`의 `DX12Framework.sln`. 다른 게임 프로젝트의 진행 상태는 이 기록에 적용하지 않는다.
- 확인 기준: Git HEAD `4bef5ec`와 현재 작업 파일. 갱신 전 작업 트리는 깨끗했다.
- 확인 방법: `GameFramework.h/.cpp`, `Shaders.hlsl`, `WinMain.cpp`, 프로젝트 설정 및 관련 심볼을 읽고 비교했다. 빌드·실행은 수행하지 않았으며 과거 사용자 실행 확인과 이번 정적 확인을 구분한다.

| 학습 항목 | 현재 파일 기준 상태 |
|---|---|
| DX12 초기화·스왑 체인·RTV/DSV·Fence | 구현 존재. 단일 Allocator와 매 프레임 GPU 대기 구조 유지 |
| Resize·Viewport·Scissor | 구현 존재. 초기화 전 WM_SIZE 차단 및 Resize 실패 시 렌더링 중단 연결 확인 |
| 빈 루트 시그니처 | 생성 및 매 프레임 바인딩 구현 존재 |
| VS/PS 및 런타임 컴파일 | POSITION/COLOR 셰이더와 VSMain/PSMain 컴파일 구현 존재. 이번 실행 검증 없음 |
| 입력 레이아웃·PSO | 미구현. PSO 멤버만 있고 생성 함수·호출은 없음. CommandList Reset의 두 번째 인자는 여전히 NULL |
| 정점 버퍼·첫 삼각형 | 미구현. 현재 프레임은 Clear/Present까지이며 정점 버퍼 바인딩과 Draw 호출 없음 |
| Mesh/Shader 분리·상수 버퍼·변환 행렬 | 후속 학습 단계 |
| 텍스처용 루트 시그니처·SRV·샘플러·DDS 출력 | 후속 학습 단계. DDS 로더 파일은 있으나 vcxproj 등록 및 Framework 호출 연결은 없음 |

현재 과제는 계속 **입력 레이아웃과 PSO 생성**이다. 5절의 구현 범위와 이해 확인 질문을 유지하며, 완료 처리하거나 정점 버퍼 단계로 넘어가지 않는다. 이해 확인 답변 여부는 파일만으로 판단하지 않는다.

프로젝트 설정 재확인: 네 구성 모두 PlatformToolset=v145이며 공통 IntDir 분리 설정이 유지되어 있다. HLSL의 Debug/x64 빌드 제외 상태는 7절에 정정했다. 이번 갱신은 학습 기록만 수정하며 소스·프로젝트 설정은 변경하지 않는다.

## 11. 최신 진행 기준 (2026-09-30, 데스크톱 복귀)

- 확인 기준: HEAD `4f52168` (`삼각형 띄우기 (단일 업로드 버퍼)`)와 현재 소스. 문서 갱신 전 작업 트리는 깨끗했다.
- 인수인계 문서 10절은 이전 HEAD 기준이라 현재 코드보다 뒤처져 있었다. 아래는 현재 파일을 읽어 확인한 내용이다. 빌드·실행 및 전체 코드 정밀 리뷰는 수행하지 않았다.
- **입력 레이아웃/PSO 구현 존재:** CreatePSO에서 POSITION/COLOR 입력 요소, 루트 시그니처, VS/PS 및 렌더링 상태를 구성하고 CreateGraphicsPipelineState를 호출한다. OnCreate에서 결과를 검사하며 FrameAdvance의 CommandList Reset에 PSO를 전달한다.
- 실제 Rasterizer는 BACK 컬링, FrontCounterClockwise=FALSE다. 이전 과제의 '초기에는 컬링 끄기' 안내와는 다르므로 리뷰 시 실제 상태를 기준으로 한다.
- **정점 버퍼/삼각형 그리기 경로 구현 존재:** CreateVertexBuffer에서 위치·색상을 가진 정점 세 개를 만들고, UPLOAD 힙/GENERIC_READ 버퍼에 Map → memcpy → Unmap으로 기록한다. 버퍼 뷰의 GPU 주소, 전체 크기, stride도 설정한다.
- OnCreate에서 정점 버퍼 생성 결과를 검사한다. FrameAdvance에서 TRIANGLELIST, IASetVertexBuffers, DrawInstanced(3, 1, 0, 0)가 연결되어 있다.
- 현재는 단일 UPLOAD 버퍼를 정점 버퍼로 직접 사용하는 학습 구조다. DEFAULT 힙으로 복사하는 경로는 아직 이 코드에 없다.
- 커밋 제목은 삼각형 출력 작업을 나타내지만, 이 대화에서 사용자의 실제 화면 출력 성공/디버그 오류 없음은 새로 확인하지 않았다. '구현 존재'와 '실행 검증 완료'를 구분한다.
- HLSL은 FxCompile 등록을 유지하며 Debug/x64 빌드 제외 설정이 존재한다.

### 현재 위치와 다음 순서

현재 위치는 **첫 삼각형 그리기 코드 작성까지 진행된 상태**다. PSO를 처음부터 다시 구현하도록 안내하지 않는다. 필요하면 기존 구현과 사용자 실행 결과를 확인한 뒤 다음 학습 단계로 진행한다.

기존 계획 기준 다음은 **Mesh/Shader 역할 분리와 리소스 수명 정리**다. 남은 본 단계는 5개이며, 기존 구현 검토나 보완에 따라 세분화할 수 있다.

1. Mesh/Shader 역할 분리와 리소스 수명 정리.
2. 상수 버퍼와 변환 행렬 연결.
3. UV, 텍스처용 루트 시그니처/SRV 힙/샘플러 구성.
4. DDS 로딩, GPU 업로드와 완료 동기화.
5. 텍스처 바인딩/출력과 오류 경로 점검.

새 대화에서는 'LEARNING_PROGRESS.md의 11절을 우선 읽고 현재 구현을 확인해줘. 첫 삼각형 그리기 코드까지 있으며, 다음은 Mesh/Shader 분리 단계다. 코드 작성은 내가 하고 빌드/실행도 내가 한다'고 요청하면 된다. 이해 확인 질문의 답변 여부는 소스만으로 추정하지 않는다.

## 12. 최신 진행 기준 (2026-10-03, Mesh 분리 완료)

확인 기준은 HEAD `3fd8663` (`CMesh가 정점 버퍼 관련 담당하도록 리팩토링`)과 현재 작업 파일이다. 문서 갱신 전 작업 트리는 깨끗했다. 이번 인수인계 갱신에서는 파일을 읽어 확인했으며 빌드·실행이나 소스 변경, 커밋·푸시는 수행하지 않았다. 커밋은 사용자가 직접 한다.

### 완료 범위와 현재 구성

- 사용자가 Mesh 분리 후 **삼각형이 정상 출력됨**을 보고했다. AI가 직접 실행한 결과는 아니며, 모든 오류 경로/디버그 경고까지 검증됐다는 뜻은 아니다.
- `Mesh.h/.cpp`의 CMesh가 정점 버퍼, 업로드 버퍼, 정점 버퍼 뷰, 정점 개수, 토폴로지를 관리한다.
- Framework는 `std::unique_ptr<CMesh> m_upMesh`로 단독 소유한다. GameFramework.h에는 CMesh 전방 선언이 있고, 생성자/소멸자는 Mesh 정의를 볼 수 있는 .cpp에서 정의한다.
- `CMesh::CreateVertexBuffer<VTYPE>()`는 템플릿이며 구현이 Mesh.h에 있다. 외부 정점 배열과 개수를 받는다. 입력 데이터는 `const VTYPE*`이며 null 데이터/정점 개수 0에는 `E_INVALIDARG`를 반환한다. 이전 리뷰의 S_OK 반환 문제는 수정 확인했다.
- 현재는 **UPLOAD → DEFAULT 복사 방식**이다. CPU memcpy로 UPLOAD 버퍼에 데이터를 쓰고, DEFAULT 버퍼는 COPY_DEST 상태로 생성한다. CopyResource와 VERTEX_AND_CONSTANT_BUFFER 상태 전환을 명령 리스트에 기록한다. 버퍼 뷰는 DEFAULT 버퍼를 가리킨다.
- Framework의 BuildObjects는 HRESULT를 반환한다. 명령 리스트 Reset → 삼각형 데이터 준비/Mesh 생성/복사 기록 → Close → ExecuteCommandLists → WaitForGPUComplete 성공 검사 → 업로드 버퍼 해제 → S_OK 순서다. 각 HRESULT 실패는 OnCreate까지 전달한다.
- 최초 초기화에는 Allocator의 GPU 사용 이력이 없고, 첫 프레임의 Allocator Reset은 업로드 완료 대기 이후에 실행된다. 이를 프레임마다 GPU 완료 확인 없이 Reset해도 된다는 뜻으로 일반화하지 않는다.
- `CMesh::DrawMesh()`가 토폴로지 설정, IASetVertexBuffers, DrawInstanced를 기록한다. Framework의 FrameAdvance는 PSO/루트 시그니처/Viewport/Scissor/RTV/DSV 설정과 Clear 뒤 Mesh의 DrawMesh를 호출한다.
- Framework가 큐 제출, Fence 대기, Present를 담당한다. Mesh는 큐 제출이나 자체 GPU 대기를 하지 않는다.
- `ReleaseUploadBuffer()`는 업로드 ComPtr을 Reset한다. 매 프레임 호출하던 해제는 제거됐고 초기 업로드 완료 확인 뒤 한 번 호출한다.
- `ReleaseObjects()`는 Mesh 소유권을 reset한다. 종료 시 대기는 OnDestroy에서 한 번 수행한다. 이전의 중복 대기는 제거됐다.
- Shader 분리는 **아직 진행하지 않았다**. CompileShaderFromFile, CreatePSO, VS/PS Blob, PSO, 루트 시그니처는 현재 Framework에 있다.

### 이번에 해결한 문제와 이해 확인

- unique_ptr 관련 연쇄 컴파일 오류의 최초 원인은 GameFramework.h에서 CMesh가 선언되지 않은 것이었다. 전방 선언으로 해결했다. make_unique 자체의 오류로 오해하지 않는다.
- CreateVertexBuffer<VertexDiffused>의 미해결 외부 기호는 템플릿 구현이 Mesh.cpp에만 있었기 때문이었다. 헤더로 옮겨 호출 번역 단위에서 구현을 볼 수 있도록 했다.
- 삼각형 미출력의 원인은 닫힌 리스트에 복사를 기록하려 하고 초기화 명령을 제출하지 않은 것이었다. 최초 FrameAdvance의 Reset 및 너무 이른 업로드 버퍼 해제도 연결되어 있었다. 위 초기화 순서를 도입해 해결했다.
- 이후 BuildObjects의 GPU 대기 HRESULT를 검사하지 않던 문제를 수정했다. 대기 성공 전에 업로드 버퍼를 해제하지 않는다.
- 사용자 이해 확인 답변을 받았다: 뷰가 리소스를 소유하지 않으므로 실제 ComPtr 수명이 필요함, 단독 소유에는 unique_ptr가 적절함, 원본 CPU 배열은 memcpy 뒤 없어져도 됨, Draw 기록 함수 반환은 GPU 완료를 뜻하지 않음.
- 설명 보완: CPU 배열의 memcpy 완료와 GPU의 CopyResource 완료는 별개다. UPLOAD 버퍼는 GPU 복사 완료까지 유지하고, DEFAULT 정점 버퍼는 GPU의 그리기 사용 완료까지 유지해야 한다. 리소스 인터페이스를 보유하는 것만으로 명령이 실행되지는 않는다.

### 다음 한 단계: Shader 분리

Mesh 분리는 현재 학습 범위에서 마무리됐다. 다음 대화에서는 Shader 분리의 가이드부터 제시한다. 아직 구체적인 클래스 API나 과제를 사용자에게 제시하지 않았으므로 구현했다고 가정하지 않는다.

- 범위 후보: 셰이더 컴파일, 입력 레이아웃/PSO 생성, 렌더링 상태 준비를 별도 Shader 클래스로 옮기는 책임 설계. 현재 코드와 사용자의 설계 의도를 확인해 한 단계만 제시한다.
- 루트 시그니처를 누가 소유할지는 명확히 설명하고 결정한다. 텍스처/상수 버퍼 바인딩과 연결될 사항이므로 Review 구조를 무조건 복제하지 않는다.
- FrameAdvance가 Shader의 상태 준비 이후 Mesh의 Draw를 호출하는 구성을 목표로 삼을 수 있다. GPU 제출·Fence·Present는 Framework 책임을 유지한다.
- 이번에 Shader 분리와 상수 버퍼, DDS 구현을 한꺼번에 요구하지 않는다.

남은 큰 계획은 기존 기준 5개다. 첫 항목의 Mesh 부분은 완료됐고 Shader/수명 정리가 남아 있다.

1. Shader 분리와 리소스 수명 정리의 남은 부분.
2. 상수 버퍼와 변환 행렬 연결.
3. UV, 텍스처용 루트 시그니처/SRV 힙/샘플러 구성.
4. DDS 로딩, GPU 업로드와 완료 동기화.
5. 텍스처 바인딩/출력과 오류 경로 점검.

### 계속 보류 중인 점검 사항

- OnDestroy는 WaitForGPUComplete 실패를 로그로 남긴 뒤에도 ReleaseObjects를 호출한다. GPU 완료를 확인하지 못한 종료 경로의 리소스 정리 정책은 아직 해결되지 않았다. Shader/리소스 수명 정리 단계에서 다룬다.
- 초기화 실패 경로에서 이미 제출한 명령/부분 생성 자원/HANDLE의 수명도 점검해야 한다. BuildObjects가 실패를 반환한다는 것만으로 전체 종료 정책이 완성됐다고 보지 않는다.
- ReportLiveObjects는 Framework의 여러 ComPtr 해제 이전에 실행된다. 최종 누수 보고의 시점은 아직 보류다.
- Reset 지속 실패, Present 반환값, Device removed 등 기존 7절의 보류 사항은 유지한다.
- 템플릿이 여러 정점 타입을 받는다고 현재 POSITION/COLOR 입력 레이아웃과 PSO가 모든 정점 형식을 자동 지원하는 것은 아니다. 다른 타입을 도입할 때 입력 레이아웃 일치 여부를 확인한다.
- HLSL 빌드 제외, 상대 파일 경로, 노트북 도구 집합 등의 환경 사항은 이전 절을 참고하되 실제 설정을 다시 확인한다.
