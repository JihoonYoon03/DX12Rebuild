# DX12 Rebuild 학습 인수인계 — 메인 PC용

최종 정리: 2026-10-03. 이 문서는 다른 컴퓨터/새 대화에서 학습을 이어가기 위한 기록이다. 실제 코드는 이후 변경될 수 있으므로 다음 조력자는 작업 전에 현재 파일을 확인한다.

**최신 진행 기준은 12절이다.** 3~6절, 9~11절은 이전 시점의 기록이다. PSO/정점 버퍼 미구현, UPLOAD 버퍼 직접 사용, 현재 과제 등의 설명이 최신 상태와 다르면 12절을 우선한다.

## 1. 가장 중요한 협업 규칙

- 사용자가 직접 구현한다. AI는 한국어 개념 설명, 단계별 가이드, 질문, 코드 리뷰를 담당한다. 요청 없이 완성 코드나 붙여 넣을 답안을 제공하거나 소스를 수정하지 않는다.
- 한 번에 한 학습 단계만 진행한다. 이해 확인 질문에 답하고 사용자가 작성한 코드를 검토한다.
- 빌드·실행·긴 테스트는 기본적으로 사용자가 수행한다. AI는 가벼운 파일·차이 검토를 수행한다.
- 명시적인 환경 수정 요청은 해당 범위에만 적용한다. 학습 과제 전체의 구현 권한으로 확대하지 않는다.
- 기존 변경과 사용자 선택을 보존하고, 이전 리뷰를 현재 코드 확인 없이 반복하지 않는다.
- Review의 구조를 그대로 복제하지 않는다. ComPtr 등 현대 C++ 기능을 활용하되 shared_ptr을 모든 객체에 적용하지 않는다.

## 2. 목표와 프로젝트 위치

- 최종 목표: Review 프로젝트의 DescriptorHeap 브랜치를 참고하여 DDS 텍스처를 로드하고 화면에 표시하기까지 학습한다.
- 이번에 확인한 저장소: `C:\Users\user\Documents\GitHub\DX12Rebuild`.
- 메인 PC에서는 실제 저장소 경로를 확인한다. GameSoftware 프로젝트와 혼동하지 않는다.
- 솔루션: `DX12Framework.sln`.
- 프로젝트: `DX12Rebuild/DX12Rebuild.vcxproj`.
- 주요 파일: `GameFramework.h/.cpp`, `Mesh.h/.cpp`, `WinMain.h/.cpp`, `Timer.h/.cpp`, `pch.h`, `Shaders.hlsl`.
- Review는 별도 프로젝트다. 메인 PC에서 경로와 DescriptorHeap 브랜치를 확인하며, 이 문서는 해당 참고 코드의 최신 상태를 검증한 기록이 아니다.

## 3. 완료 범위와 검증 수준

1. 디버그 계층, Adapter/Device 생성, WARP 대체 경로, ComPtr 및 HRESULT 처리 학습.
2. DIRECT Queue/Allocator/CommandList, SwapChain, RTV/DSV, Win32 메시지 루프 구성.
3. Timer/FPS 및 QPC, 단일 Fence의 전역 증가값을 통한 GPU 대기.
4. 창 모드 Resize, 최소화/복원, Viewport/Scissor 설정.
5. 빈 루트 시그니처 생성·직렬화·바인딩.
6. POSITION/COLOR HLSL VS/PS와 D3DCompileFromFile 런타임 컴파일.
7. 입력 레이아웃과 PSO 생성, OnCreate 호출 및 CommandList Reset 연결.
8. UPLOAD 힙 정점 버퍼 생성, Map/복사/Unmap, 버퍼 뷰, 첫 RGB 삼각형 출력.

검증 구분:
- 사용자는 과거 창 크기 변경·최대화·최소화 후 복원 시 배경색 정상 출력을 확인했다.
- 이후 PSO 적용 실행에 문제가 없다고 확인했고, 정점 버퍼·삼각형 출력 코드도 정상 동작한다고 확인했다.
- AI는 해당 코드의 정적 리뷰에서 추가로 수정해야 할 오류를 발견하지 못했다. AI가 빌드나 실행을 직접 수행한 것은 아니다.
- 삼각형 추가 이후 Resize 전체 절차, 모든 빌드 구성, 메인 PC 실행까지 검증했다고 확대 해석하지 않는다.
- 전체화면은 비활성화 상태로 미검증이다.

## 4. 현재 코드 구조와 중요한 선택

### Framework와 프레임

- Framework가 Device, SwapChain, Queue, 단일 Allocator/List, Fence, RTV/DSV 및 리소스를 ComPtr로 소유한다.
- 백 버퍼 2개, RTV 형식 R8G8B8A8_UNORM, DSV 형식 D24_UNORM_S8_UINT, 실제 샘플 설정 Count=1/Quality=0.
- Fence는 하나의 UINT64 증가값을 사용하며 매 프레임 GPU 완료를 기다린다. 프레임 병렬화는 아직 하지 않는다.
- OnCreate에서 루트 시그니처 → 셰이더 컴파일 → CreatePSO → CreateVertexBuffer 순으로 준비하고 실패를 전파한다.
- FrameAdvance에서 Allocator Reset → PSO를 전달한 List Reset → SetGraphicsRootSignature → Viewport/Scissor → 백 버퍼 전환·Clear → IA 설정·Draw → PRESENT 전환 → 제출·GPU 대기·Present를 수행한다.
- Resize 실패/최소화 시 m_bStopRender로 렌더링을 중단한다. Resize 시작의 Viewport/Scissor 갱신 위치는 사용자가 유지하기로 했으므로 필수 버그로 반복 지적하지 않는다.

### PSO와 셰이더

- 입력은 POSITION=R32G32B32_FLOAT/offset 0, COLOR=R32G32B32A32_FLOAT/offset 12. 슬롯/시맨틱 인덱스 0, 정점별 데이터.
- VS는 float3 위치에 w=1을 붙여 출력하고 색상을 전달한다. PS는 색상을 반환한다. 아직 행렬·UV·텍스처는 없다.
- PSO는 TRIANGLE 계열, RGBA 전체 쓰기, 깊이 테스트/쓰기 및 LESS 비교, 스텐실 비활성화.
- 현재 CullMode는 BACK, FrontCounterClockwise는 FALSE이다. 첫 출력 가이드에서는 NONE을 권했지만 실제 정점 순서와 실행은 정상 확인되었다. 현재 BACK 자체를 오류로 취급하지 않는다.
- AntialiasedLineEnable=TRUE는 차후 사용을 위해 사용자가 명시적으로 유지했다. 현재 삼각형 과제에서 불필요하다는 이유로 다시 변경을 요구하지 않는다.
- PSO의 루트 시그니처 지정은 CommandList의 SetGraphicsRootSignature 호출을 대체하지 않는다.

### 정점 버퍼

- Mesh.h의 VertexDiffused는 XMFLOAT3 pos와 XMFLOAT4 color로 구성된다. stride 28바이트, 정점 세 개 총 84바이트.
- 위쪽 → 오른쪽 아래 → 왼쪽 아래 위치에 RGB 정점 세 개를 배치한다. z=0, alpha=1.
- CreateVertexBuffer는 UPLOAD 힙/BUFFER/ROW_MAJOR/GENERIC_READ로 리소스를 만든다.
- Map의 읽기 범위는 {0,0}; memcpy 후 Unmap(0,NULL). GPU 가상 주소·전체 크기·stride로 뷰를 구성한다.
- m_cpVertexBufferTest가 리소스를 소유하고 m_VertexBufferViewTest가 뷰를 보관한다. 생성 후 버퍼를 다시 쓰지 않는 현재 흐름이다.
- IASetPrimitiveTopology(TRIANGLELIST), 슬롯 0 IASetVertexBuffers, DrawInstanced(3,1,0,0)를 Clear 후 PRESENT 전환 전에 기록한다.
- Mesh.h/.cpp는 프로젝트에 등록되어 있지만 CMesh는 빈 클래스다. 실제 Mesh 역할 분리는 아직 수행하지 않았다.
- BuildObjects/ReleaseObjects는 아직 비어 있다. Shader 클래스도 아직 없다.

## 5. 이해 확인 답변과 교정 기록

사용자는 PSO 단계와 정점 버퍼 단계 질문에 답했다. 아래 교정은 설명 완료했지만, 설명 후 재답변으로 숙달을 확인한 것은 아니다.

- 입력 레이아웃: 버퍼 바이트의 형식·오프셋·슬롯을 HLSL 시맨틱에 연결한다. 단순히 PSO마다 다를 수 있어서 필요한 것은 아니다.
- 색상 offset 12: 앞의 XMFLOAT3가 4바이트 float 세 개이기 때문이다. 같은 레지스터를 사용해서가 아니며 상수 버퍼 패킹과 구분한다.
- PSO의 RTV 형식은 실제 바인딩된 RTV 형식과 맞춰야 한다. 현재는 백 버퍼 형식과 같다.
- 색상 쓰기 마스크 0은 검정색 기록이 아니라 모든 색상 채널 쓰기 차단이다. 기존 Clear 색상이 유지된다.
- 입력 레이아웃과 정점 버퍼 뷰의 역할, sizeof 기반 stride/크기, Map CPU 주소와 GPU 가상 주소의 용도는 올바르게 답했다.
- PSO의 TRIANGLE은 계열이고 IA의 TRIANGLELIST/TRIANGLESTRIP은 정점 조립 규칙이다. PSO가 하나여도 구체적인 토폴로지 지정이 필요하다. “PSO를 번갈아 쓰기 때문”이라는 답변을 교정했다.

## 6. 다음 단계: Mesh/Shader 역할 분리와 리소스 수명

다음 계획은 역할 분리 단계다. 아직 구체적인 구현 과제나 이해 확인 질문은 제시하지 않았다. 다음 조력자는 현재 코드를 읽고 작은 단위로 과제를 제시한다.

첫 소단계 제안:
- 현재 Framework의 테스트 정점 버퍼 소유·뷰·정점 수·토폴로지·그리기 책임을 CMesh로 어떻게 옮길지 사용자와 정리한다.
- Framework는 장치·명령 리스트·프레임 동기화를 관리하고 Mesh에 필요한 의존성을 전달하는 방향을 설명한다.
- 먼저 현재 삼각형 출력 동작을 유지하며 Mesh 분리만 진행한다. Shader 분리와 행렬·텍스처까지 한 번에 요구하지 않는다.
- 이후 Shader의 셰이더 컴파일/PSO 책임 및 루트 시그니처 소유 위치를 검토한다. 확정되지 않은 설계를 이미 합의된 것으로 취급하지 않는다.
- GPU 사용 완료 전에 리소스를 해제하지 않는 수명, 명확한 소유권, 초기화 실패 시 정리, 최종 Live Objects 보고 순서를 함께 학습한다.
- Viewport/Scissor의 시각적 효과 비교는 아직 별도 확인 기록이 없다. 필요하면 사용자가 수행할 작은 실험으로 다룬다.

남은 큰 단계:
1. Mesh/Shader 역할 분리와 리소스 수명 정리.
2. 상수 버퍼와 변환 행렬 연결.
3. UV와 텍스처용 루트 시그니처, SRV 힙, 샘플러 구성.
4. DDS 로딩과 DEFAULT 리소스로의 GPU 업로드, 업로드 완료 동기화.
5. 텍스처 바인딩/출력과 남은 오류 경로 점검.

DDS 로더 파일은 있지만 프로젝트 등록·Framework 호출 연결은 아직 없다. 전체화면·카메라 이동·프레임 병렬화는 현재 필수 목표 밖이다.

## 7. 후속 점검 항목

- Shaders.hlsl은 FxCompile 항목이다. Debug/x64만 ExcludedFromBuild=true이며 다른 세 구성에는 명시적 제외 설정이 없다. 런타임 컴파일 중복 여부를 구성별로 판단한다.
- CommandAllocator/List Reset 실패는 로그 후 해당 프레임 return이다. 지속 실패 정책은 후속 오류 처리 단계에서 정리한다.
- Present 반환값 및 Device removed 경로는 후속 점검 대상이다.
- OnDestroy의 ReportLiveObjects는 Framework ComPtr 멤버 해제보다 먼저 실행된다. 이때의 Live 경고만으로 누수라고 단정하지 않는다.
- 초기화 도중 실패했을 때 HANDLE 등 부분 생성 자원 정리와 Adapter 열거 등 API 실패 경로는 현재 코드를 읽고 검토한다.
- 공유 Fence에 백 버퍼별 독립 증가 번호를 다시 도입하지 않는다. 병렬화 시 전역 증가값과 프레임별 대기값을 구분해야 한다.

## 8. 메인 PC 이동 및 환경 확인

> LEARNING_PROGRESS.md의 12절을 우선 읽고 현재 코드를 확인해줘. Mesh 분리와 DEFAULT 버퍼 업로드 구현까지 진행했고 삼각형 정상 출력을 확인했어. 다음은 Shader 분리 단계야. 코드는 내가 작성하고 너는 가이드와 리뷰만 담당해. 완성 코드 제공이나 임의 수정은 하지 말고, 빌드와 실행은 내가 하도록 요청해. 한 단계씩 이어가자.

## 9. 메인 PC 새 대화 시작용 문구

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
