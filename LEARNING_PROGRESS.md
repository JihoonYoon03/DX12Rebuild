# DX12 Rebuild 학습 인수인계 — 메인 PC용

최종 정리: 2026-10-05. 이 문서는 다른 컴퓨터/새 대화에서 학습을 이어가기 위한 기록이다. 실제 코드는 이후 변경될 수 있으므로 다음 조력자는 작업 전에 현재 파일을 확인한다.

**최신 진행 기준은 13절이다.** 12절은 Mesh 분리 완료 시점, 그 이전 절은 과거 기록이다. Shader 미구현이나 현재 과제 등의 설명이 최신 상태와 다르면 13절을 우선한다.

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

## 13. 최신 진행 및 리소스 생성 헬퍼 검토 (2026-10-05)

### 확인 기준과 검증 수준

- HEAD: `f7f04ed` (CShader가 PSO 및 셰이더 바이트코드 관리하도록 리팩토링).
- 현재 미커밋 변경: GameFramework.h/.cpp, Mesh.h, pch.h/.cpp. 문서 저장 전부터 존재한 사용자 작업이며 보존한다.
- AI는 현재 파일과 이전 대화를 정적으로 검토했다. 빌드·실행은 수행하지 않았으며 소스 변경·커밋·푸시도 하지 않는다.
- 사용자는 리소스 생성 헬퍼를 “작동되게 수정했다”고 보고했다. 현재 호출 조건의 동작 확인으로 기록하고 모든 힙/상태/텍스처/실패 경로 검증으로 확대하지 않는다.

### 완료 및 진행 중인 범위

| 항목 | 진행도 |
|---|---|
| DX12 초기화·Resize·PSO·RGB 삼각형 | 기존 완료 범위 유지 |
| Mesh 분리·DEFAULT 정점 버퍼 업로드 | 구현 및 기존 사용자 정상 출력 확인 유지 |
| Shader 분리 | 구현·정적 리뷰 완료. 별도 실행 검증 범위는 사용자 보고에 한정 |
| 공통 리소스 생성 헬퍼 | pch.h/.cpp로 추출, 현재 정점 업로드 호출 연결. 범용화 보완 중 |
| 상수 버퍼·월드 행렬 | 과제 설명 완료, Framework 멤버 선언만 추가됨. 생성·갱신·루트 CBV·HLSL 변환 미구현 |
| UV·SRV·샘플러·DDS 출력 | 후속 단계 |
| 종료/초기화 실패 정책·최종 누수 보고 | 기존 보류 항목 유지 |

### Shader 분리 결과와 학습 기록

- CShader가 VS/PS Blob과 PSO를 ComPtr 멤버로 소유한다. Framework는 unique_ptr<CShader>를 소유하고 루트 시그니처는 Framework에 유지한다.
- CreateShader에서 가상 CompileShader를 호출하고 HRESULT 실패 시 반환한 뒤 Blob 주소·크기로 PSO를 만든다. 생성자는 컴파일하지 않는다.
- 생성자에서 가상 함수를 호출하면 파생 구현이 선택되지 않으며, 컴파일 HRESULT 무시 시 null Blob 역참조가 발생할 수 있다는 문제를 수정했다.
- public 가상 소멸자 추가와 protected Blob 접근 경로를 확인했다. 현재 PSO도 protected이며 이를 필수 오류로 취급하지 않는다.
- 파생 CompileShader는 성공 반환 시 현재 PSO에 필요한 VS/PS Blob을 모두 준비해야 한다. 컴파일 대상을 바꿔도 입력 레이아웃이 자동 변경되지는 않는다.
- FrameAdvance는 Reset(nullptr) → Framework 루트 시그니처 설정 → Shader의 SetPipelineState → 기존 화면 설정/Clear → Mesh Draw 흐름이다. ReleaseObjects는 Shader/Mesh 소유권을 해제한다.
- 사용자는 Shader/Mesh 책임, Draw 이전 PSO 설정, 루트 시그니처 설정 필요성, 상태 기록과 GPU 실행의 차이에 답했다. Shader는 파이프라인 전체가 아닌 셰이더/PSO 상태를 담당하며, 기록 함수의 반환은 큐 제출이나 GPU 완료를 의미하지 않는다고 보완했다.
- D3D12_SHADER_BYTECODE는 포인터·크기만 가지며 Blob을 소유하지 않는다. Blob은 CreateGraphicsPipelineState 호출 완료까지 살아 있어야 한다. 이후 원본 Blob 해제는 가능하지만 여러 PSO 생성/재사용을 고려해 사용자가 Blob 멤버 보관을 선택했다.

### 현재 헬퍼가 실제로 하는 일

pch.h/.cpp의 전역 CreateCommittedResource는 단순 생성 래퍼가 아니라 다음 과정을 수행한다.

1. 전달받은 heapProperties로 GENERIC_READ 리소스 생성.
2. Map/memcpy/Unmap으로 원본 데이터를 기록.
3. 호출자 heapProperties.Type을 DEFAULT로 변경하여 두 번째 리소스 생성.
4. CopyResource 기록.
5. d3dResourceStates에서 VERTEX_AND_CONSTANT_BUFFER로 전환 기록.

Mesh는 UPLOAD 힙과 BUFFER 설명을 전달하고 기본 초기 상태 COPY_DEST를 사용하므로 현재 경로와 맞는다. Mesh의 HRESULT 실패 전달은 수정 확인했다. 명령 제출·GPU 대기·업로드 해제는 계속 Framework가 담당한다.

이전 링크 오류는 pch.h의 const ComPtr 참조/const void* 선언과 pch.cpp의 비const 정의가 서로 달랐기 때문이다. 현재 선언·정의 타입 일치가 확인된다. 기본 인자는 헤더에만 둔다.

### 범용화 전에 보완할 사항

**1. 힙 종류별로 역할과 상태를 분리한다.** 현재 첫 생성의 힙 종류를 강제/검사하지 않고 이후 항상 DEFAULT로 변경하므로 힙 종류를 선택하는 범용 생성 함수가 아니다.

| 힙 종류 | 기본 사용법과 상태 규칙 |
|---|---|
| DEFAULT | 일반적인 GPU 전용 리소스. 표준 DEFAULT 힙은 CPU Map으로 데이터를 쓰지 않는다. 초기 업로드는 COPY_DEST로 만들거나 복사 전 해당 상태로 전환하고, 복사 후 용도별 최종 상태로 전환 |
| UPLOAD | CPU 쓰기/GPU 읽기용 버퍼. GENERIC_READ로 생성하고 상태를 변경하지 않는다. 상시 갱신용 상수 버퍼는 DEFAULT 복사 없이 직접 사용 가능 |
| READBACK | GPU 쓰기 결과를 CPU가 읽는 버퍼. COPY_DEST로 생성하고 상태를 변경하지 않는다. GPU 복사 완료를 Fence로 확인한 후 Map해 읽음. 초기 데이터를 memcpy해 넣는 경로가 아님 |

CUSTOM/GPU_UPLOAD 힙은 이번 범위에서 지원한다고 가정하지 않는다. 표준 힙 규칙 출처: [Microsoft D3D12_HEAP_TYPE](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_heap_type).

**2. 초기 상태와 최종 상태를 분리한다.** 현재 배리어의 StateBefore를 인자와 맞췄지만 CopyResource 이전에 목적지가 COPY_DEST인지는 여전히 보장하지 않는다. 초기 상태 인자만 바꿔 넘기는 것으로는 해결되지 않는다. 업로드 전용 경로라면 목적지 초기 상태를 COPY_DEST로 고정하고 최종 상태를 인자로 받는 방식이 단순하다. 최종 상태가 이미 COPY_DEST라면 동일 상태 배리어는 생략한다. 정점/상수 버퍼 상태를 고정하면 인덱스 버퍼·SRV·READBACK에 재사용할 수 없다.

**3. 호출자 입력에 숨은 변경을 만들지 않는다.** heapProperties를 참조로 받아 Type을 DEFAULT로 변경한다. 호출자가 같은 변수를 다음 생성에 재사용하면 첫 리소스가 DEFAULT로 만들어져 Map 실패로 이어질 수 있다. const 입력이나 값 복사와 내부 지역 힙 설정을 사용해 입력을 보존하는 방향을 권한다.

**4. 버퍼와 텍스처 경로를 구분한다.** 현재 uploadBuffer와 buffer에 같은 resDesc를 사용하므로 BUFFER 전용으로 제한해야 한다. 표준 UPLOAD/READBACK 힙에는 텍스처 리소스를 직접 만들 수 없다. DDS 업로드는 DEFAULT 텍스처와 별도의 UPLOAD 버퍼, GetCopyableFootprints에 따른 행/서브리소스 배치 및 CopyTextureRegion 경로가 필요하다. 현재 memcpy/CopyResource 경로를 텍스처까지 지원하는 것으로 기록하지 않는다. 출처: [Microsoft 텍스처 업로드](https://learn.microsoft.com/en-us/windows/win32/direct3d12/upload-and-readback-of-texture-data).

**5. 입력·출력 계약을 검사한다.** Device 및 기록에 필요한 CommandList, 데이터 포인터와 크기 조합, BUFFER 차원, dataSize <= resDesc.Width를 검사해야 한다. 현재 dataSize가 용량을 넘으면 memcpy가 범위를 벗어난다. 데이터 없는 생성은 업로드/복사를 건너뛰는 별도 경로가 필요하다. 두 출력 ComPtr이 같은 변수를 참조하는 호출도 금지해야 한다. Mesh의 정점 수/크기 계산과 UINT 크기 뷰에 대한 범위 검사도 범용화 시 고려한다.

**6. 생성 설정과 실패 시 소유권을 명확히 한다.** d3dHeapFlags는 현재 두 번째 생성에만 적용되고 첫 생성은 고정값이다. 같은 resDesc의 ResourceFlags가 UPLOAD에도 적합한지 확인해야 한다. 일반 리소스 생성까지 확장한다면 최적화 ClearValue와 초기 상태를 지원할 필요가 있다. 실패 시 부분 생성 결과를 유지할지 초기 상태로 복원할지 계약을 정한다. 지역 ComPtr에서 성공 후 결과를 넘기는 방식도 검토한다. 이미 GPU가 사용하는 기존 출력 리소스를 ReleaseAndGetAddressOf로 교체하는 사용은 호출자가 GPU 완료를 보장해야 한다.

**7. 기록과 완료를 구분한다.** S_OK는 생성 및 복사/배리어 기록 성공을 뜻한다. 업로드 리소스는 GPU 복사 완료까지 유지한다. CommandList가 기록 중이어야 하며 호출자는 필요한 상태 전환과 큐 종류를 보장한다. Map/Unmap은 GPU 동기화를 수행하지 않는다. READBACK Map 전에 Fence 확인이 필요하고, UPLOAD 상수 버퍼 덮어쓰기 전에도 이전 GPU 사용 완료를 확인해야 한다.

권장 책임 구분은 “단일 리소스 생성”, “CPU 쓰기”, “DEFAULT 버퍼 초기 업로드”, “GPU 결과 readback”이다. 이를 반드시 한 함수에 모두 구현해야 하는 것은 아니다. 현재 학습에서는 단순 생성과 DEFAULT 버퍼 업로드를 구분하는 작은 설계부터 진행하고, 텍스처/READBACK 구현은 실제 필요 단계에서 추가한다. pch에 공통 선언을 두겠다는 사용자 선택은 유지하며 이번에는 파일 재배치를 요구하지 않는다.

### 다음 진행 순서와 상수 버퍼 과제

현재는 사용자가 상수 버퍼 학습에 앞서 공통 생성 함수 보완을 요청한 상태다. 먼저 위 계약과 상태 규칙을 정리한 뒤 상수 버퍼로 돌아간다. 범용 프레임워크 전체를 한 번에 구현하도록 요구하지 않는다.

- Framework에 mtx44World, m_cpMtxBuffer, m_mappedMtxBuffer 선언이 있다. 아직 행렬 초기화/버퍼 생성/Map/갱신/해제는 연결되지 않았다.
- 루트 시그니처는 NumParameters=0이며 HLSL은 기존 POSITION/COLOR 출력이다. 행렬 상수 버퍼 단계는 완료가 아니다.
- 상수 버퍼 과제: UPLOAD/GENERIC_READ 256바이트 공간, 한 번 Map한 주소에 64바이트 월드 행렬 기록, root CBV 하나(register b0, space0, VS 가시성), Draw 전 루트 인덱스 0에 GPU 주소 설정.
- CPU 행렬은 전치해 저장하고 HLSL 기본 행렬 저장 방식에서 행 벡터 × 행렬로 통일한다. 단위 행렬의 기존 출력 유지 후 작은 평행이동을 확인한다. 현재 매 프레임 GPU 대기 구조를 유지한다.
- 제시한 이해 확인 질문: 64바이트 행렬에 256바이트 공간을 할당하는 이유, b0와 루트 매개변수 인덱스의 차이, CPU 덮어쓰기/GPU 읽기 동시 사용 문제와 현재 대기 방식, 정점 수정과 행렬 이동의 차이. 아직 답변을 받지 않았다.
- 남은 큰 순서: 공통 생성 함수 계약 보완 → 상수 버퍼/행렬 → UV·루트 시그니처·SRV·샘플러 → DDS 텍스처 업로드/동기화 → 텍스처 출력과 오류 경로.
- 종료 GPU 대기 실패, 초기화 부분 실패, Present/device removed, 최종 Live Objects 보고 정책은 이전 보류 항목을 유지한다.

### 새 대화 시작용 문구

> LEARNING_PROGRESS.md의 13절부터 읽고 현재 코드를 확인해줘. Mesh/Shader 분리는 구현했고 pch의 공통 CreateCommittedResource 함수를 보완 중이야. 현재 함수는 정점 버퍼 업로드 경로에 맞으며 DEFAULT/UPLOAD/READBACK 상태, 복사 전후 상태, 입력 크기와 소유권 계약을 정리해야 해. 이를 작은 단계로 검토한 뒤 UPLOAD 상수 버퍼와 월드 행렬 과제로 돌아가자. 코드는 내가 작성하고 너는 가이드·질문·리뷰만 담당해. 임의 소스 수정이나 빌드·실행은 하지 마. ComPtr Blob 멤버와 AntialiasedLineEnable=TRUE는 유지한다.
