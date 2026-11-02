# DX12 Rebuild 학습 인수인계


최종 정리: 2026-11-03. 이 문서는 다른 컴퓨터/새 대화에서 학습을 이어가기 위한 기록이다. 실제 코드는 이후 변경될 수 있으므로 다음 조력자는 작업 전에 현재 파일을 확인한다.

**최신 진행 기준은 17절이다. 먼저 17절을 읽는다.** 전체 방향은 GAME_FRAMEWORK_PLAN.md를 함께 읽는다. 16절의 전환 중 오류와 다음 과제는 과거 기록이다. 모델 리팩토링 보완과 Transform/GameObject는 진행됐고, 사용자 요청으로 Shader 범용 설정화는 건너뛴다. 다음은 Scene 객체 소유·순회다. 현재 파일이 기록과 다르면 실제 코드를 먼저 확인한다.

## 1. 가장 중요한 협업 규칙

- 사용자가 직접 구현한다. AI는 한국어 개념 설명, 단계별 가이드, 질문, 코드 리뷰를 담당한다. 요청 없이 완성 코드나 붙여 넣을 답안을 제공하거나 소스를 수정하지 않는다.
- 한 번에 한 학습 단계만 진행한다. 이해 확인 질문에 답하고 사용자가 작성한 코드를 검토한다.
- 빌드·실행·긴 테스트는 기본적으로 사용자가 수행한다. AI는 가벼운 파일·차이 검토를 수행한다.
- 명시적인 환경 수정 요청은 해당 범위에만 적용한다. 학습 과제 전체의 구현 권한으로 확대하지 않는다.
- 기존 변경과 사용자 선택을 보존하고, 이전 리뷰를 현재 코드 확인 없이 반복하지 않는다.
- Review의 구조를 그대로 복제하지 않는다. ComPtr 등 현대 C++ 기능을 활용하되 shared_ptr을 모든 객체에 적용하지 않는다.

## 2. 목표와 프로젝트 위치

- 기존 DDS 출력 목표는 달성했다. 현재 목표는 게임 제작에 적합한 Mesh/Material/Object/Scene/Camera API이며, 모델 구조와 GPU 버퍼 통합을 먼저 정리하는 중이다.
- 현재 PC 저장소: `C:\Users\wlgns\Documents\Github\DX12Rebuild`. 노트북에서는 실제 경로를 다시 확인한다.
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


## 14. 최신 인수인계: DDS 로딩 완료, GPU 업로드 전 (2026-10-14)

### 확인 기준과 현재 위치

- 현재 Git 로그의 최신 커밋: `7f8a8a9` (DDS 형식 텍스처 리소스 파일 CPU 데이터로 불러오기). 이전 커밋은 `0ea37a7` (SRV Heap Desc 만들기), `cbb2083` (UV 정보를 담는 정점 구조체 추가).
- 실제 GameFramework, Shader, Mesh, HLSL, pch 헬퍼 및 프로젝트 설정을 읽어 확인했다. git status는 “this operation must be run in a work tree” 오류가 발생해 미커밋 여부는 확정하지 않았다. 파일 검토와 문서 저장은 가능했으며 Git 설정은 수정하지 않았다.
- AI는 이번 기록 작업에서 빌드·실행·소스 변경·커밋·푸시를 수행하지 않는다. 사용자 실행 확인과 정적 코드 확인을 구분한다.
- 현재 위치: **상수 버퍼/월드 행렬, UV 전달, SRV 힙/루트 테이블/정적 샘플러 준비를 거쳐 DDS CPU 로딩까지 진행했다. 다음 한 단계는 텍스처 GPU 업로드다.** DDS 화면 출력까지 완료한 것은 아니다.

| 항목 | 진행 상태 |
|---|---|
| Mesh/Shader 분리, DEFAULT 정점 업로드 | 이전 완료 범위 유지 |
| UPLOAD 상수 버퍼와 월드 행렬 | 구현·리뷰 및 사용자 변환 테스트 성공 보고 |
| 위치/색상/UV 정점과 VS→PS 전달 | 구현·리뷰 확인. 삼각형 두 개의 사각형, PS는 UV 색상 출력 |
| SRV 힙·루트 테이블·정적 샘플러 | 수정 후 사용자 정상 실행 보고 |
| DDS 파일 읽기·DEFAULT 텍스처 리소스 생성 | 사용자 디버깅에서 데이터 정상 확인 보고. 구체적 크기/형식/밉 값은 기록되지 않음 |
| 텍스처 UPLOAD 준비·GPU 복사·읽기 상태 전환 | 아직 미구현 |
| 실제 SRV 작성·테이블 GPU 핸들 연결·Sample 출력 | 아직 미구현 |

### 상수 버퍼와 행렬 단계 기록

- CreateBufferResource로 256바이트 UPLOAD/GENERIC_READ 버퍼를 데이터 없이 만들고 한 번 Map한다. AnimateObjects에서 행렬의 실제 64바이트를 기록한다.
- 루트 매개변수 0은 VS용 루트 CBV, b0/space0이며 Draw 전에 SetGraphicsRootConstantBufferView(0, GPU 주소)를 사용한다.
- GPU 완료를 매 프레임 기다리는 기존 구조 덕분에 단일 상수 버퍼 재사용 시 이전 GPU 읽기와 다음 CPU 쓰기가 겹치지 않는다. 프레임 병렬화에 그대로 일반화하지 않는다.
- 사용자가 반시계 회전 및 추가 변환 테스트 성공을 보고했다. _41을 전달용 행렬 저장 후 수정해 w가 바뀌었던 문제를 설명했고 원인을 이해했다고 보고했다.
- 현재 AnimateObjects는 XMMatrixTranspose로 전달용 행렬을 저장한다. 회전 증가 코드는 주석 처리되어 있고 360 기준 초기화는 남아 있다. 회전 활성화 시 라디안/XM_2PI 기준으로 초과분을 보존하도록 정리해야 한다. 생성자의 단위 행렬 inverse는 불필요하지만 현재 단위 행렬 결과 자체는 동일하다.
- 이해 확인 답변 교정: 256바이트는 CBV 정렬/크기 요구와 관련되며 모든 리소스의 물리 할당 단위라는 뜻은 아니다. b0와 루트 매개변수 인덱스는 별개다. 월드 행렬은 원본 정점을 유지하며 객체 변환을 적용한다.

### UV 단계 기록

- VertexTexcoord: XMFLOAT3 위치 → XMFLOAT4 색상 → XMFLOAT2 UV. UV offset=28, stride=36바이트.
- 입력 레이아웃 TEXCOORD0/R32G32_FLOAT/offset28 및 Mesh 생성의 새 정점 타입이 일치한다.
- 정점 여섯 개로 사각형을 구성했고 공유 위치의 UV는 동일하다. VS가 UV를 전달하고 PS는 R=u/G=v/B=0, 기존 alpha=1을 반환한다.
- 사용자 질문 답변: 크기 계산과 래스터라이저 보간 설명이 맞았다. 월드 행렬을 UV에 적용하지 않으며, 텍스처 스크롤 등 목적의 별도 UV 변환은 가능하다고 보완했다.

### 디스크립터 바인딩 구조 기록

- CreateTexture 안에서 CBV_SRV_UAV, NumDescriptors=1, SHADER_VISIBLE, NodeMask=0 힙을 생성한다. FrameAdvance는 SetDescriptorHeaps로 이를 연결한다.
- 루트 매개변수 1은 PS용 DESCRIPTOR_TABLE이다. 범위는 SRV 하나, BaseShaderRegister=0(t0), space0, APPEND 오프셋이다.
- 정적 샘플러 s0/space0/PS, 일반 MIN_MAG_MIP_LINEAR 필터, CLAMP 주소 방식이다. 정적 샘플러이므로 별도 SAMPLER 힙을 만들 필요가 없다.
- 처음 작성한 ROOT_PARAMETER_TYPE_SRV와 COMPARISON 필터를 각각 DESCRIPTOR_TABLE과 일반 필터로 바꿨고 사용자가 “잘 됨”이라고 확인했다.
- 개념 교정: SHADER_VISIBLE은 셰이더가 힙의 디스크립터를 참조하기 위한 조건이며 PS에만 해당하지 않는다. CLAMP는 직전 화면 픽셀이 아니라 UV 경계의 텍스처 가장자리를 사용한다. WRAP은 좌표를 반복한다.
- 사용자가 힙/테이블/디스크립터 관계와 t0 연결을 잘 모르겠다고 했고 다음 흐름을 설명했다: 텍스처=이미지 데이터, SRV=읽는 방법, 힙=설명 저장 슬롯, 테이블=힙 구간과 레지스터 매핑, 루트 시그니처=바인딩 규칙.
- CPU 핸들로 CreateShaderResourceView가 슬롯을 채우고, GPU 핸들로 SetGraphicsRootDescriptorTable이 테이블 시작을 지정한다. 루트 인덱스 1, 힙 슬롯 0, t0는 서로 다른 번호다. 개념 설명 후 재답변으로 숙달을 확인한 것은 아니다.

### DDS 로더와 파일 경로

- DDSTextureLoader12.h/.cpp가 프로젝트에 등록되어 있다. 로더 cpp는 Debug/x64에서만 PrecompiledHeader=NotUsing으로 확인된다. 다른 구성까지 빌드 성공했다고 단정하지 않는다.
- d3dx12 및 분리된 종속 헤더들은 DX12Rebuild/directx 폴더에 준비되어 있다. 헤더는 include 경로에서 검색 가능하면 별도 프로젝트 항목 등록이 필수는 아니다. 같은 버전 묶음을 사용하고 중복 d3dx12.h 위치에 주의하도록 설명했다.
- CreateTexture는 LoadDDSTextureFromFileEx를 호출한다. 파일은 저장소의 Resources/moon.dds이며 현재 문자열은 L"../Resources/moon.dds"다.
- 기존 단일 백슬래시 문자열의 C++ 이스케이프 문제와 편집기 경로 인식/실행 작업 디렉터리 차이를 설명했다. 상대 경로는 실행 작업 디렉터리 기준이다.
- DEFAULT 텍스처는 m_cpTextureResource 멤버로 유지한다. ddsData(unique_ptr), subData(vector), alphaMode, isCubeMap은 CreateTexture 지역 변수다. GetDesc 결과를 지역 texResDesc로 조회한다.
- 현재 실제 CreateShaderResourceView는 없고 SRV 설명 코드는 주석이다. 향후 SRV 형식과 밉 개수는 로드한 리소스 설명에 맞춰야 하며 R8G8B8A8_UNORM을 무조건 지정하지 않는다.
- HLSL의 texture/sampler2D 선언은 아직 사용되지 않고 PS는 UV 색상만 반환한다. 실제 샘플링 단계에서는 Texture2D와 SamplerState로 정리한다.

### 가장 중요한 수명과 이해 확인

LoadDDSTextureFromFileEx는 텍스처를 만들고 CPU 초기 데이터와 서브리소스 설명을 반환한다. GPU 업로드/상태 전환/SRV 바인딩까지 수행하는 함수가 아니다.

- ddsData가 실제 CPU 데이터 메모리를 소유한다.
- subData의 pData는 그 메모리 내부를 가리키며 RowPitch/SlicePitch는 원본 데이터의 행/슬라이스 간격이다. 목록 자체는 이미지 데이터를 소유하지 않는다.
- 따라서 현재처럼 함수가 끝나 ddsData가 사라진 뒤 subData 포인터만 재사용하면 안 된다. CreateTexture 안에서 CPU→UPLOAD 복사를 끝내거나 두 결과를 준비 완료까지 유지한다.
- CPU ddsData는 UPLOAD 버퍼에 복사될 때까지 필요하다. UPLOAD 리소스는 GPU 복사 완료까지 필요하다. DEFAULT 텍스처는 GPU 샘플링 사용 완료까지 필요하다.
- 업로드는 이미지 내용을 GPU로 옮기는 작업, SRV 생성은 읽는 방법을 설명하는 작업, 바인딩은 이를 t0에 연결하는 작업이다. 어느 하나가 다른 작업을 대신하지 않는다.
- 사용자 DDS 질문 답변: “바인딩하지 않아 샘플링 불가”를 업로드/읽기 상태 준비도 필요하다고 보완했다. ddsData 수명과 SRV/업로드 차이는 모르겠다고 답해 위 내용을 설명했다. 이후 재질문으로 이해를 확인하지는 않았다.

### 다음 한 단계: 텍스처 GPU 업로드

아직 구체적인 업로드 구현 과제는 제시하지 않았다. 다음 조력자는 현재 코드/로더/사용 가능한 d3dx12 헬퍼를 읽고 한 소단계씩 안내한다.

1. DDS 서브리소스 전체에 필요한 UPLOAD 크기와 레이아웃을 GetCopyableFootprints 또는 그 기반 헬퍼로 계산한다. 파일 크기나 폭×높이만으로 추정하지 않는다.
2. UPLOAD/GENERIC_READ 버퍼를 만들고 원본 RowPitch와 GPU 복사용 행 정렬 차이를 반영해 데이터를 배치한다. d3dx12 UpdateSubresources 같은 헬퍼를 사용할 수 있으나 무엇을 복사/기록하는지 설명한다.
3. BuildObjects의 기록 중인 CommandList에 텍스처 복사를 기록한다. DDS 로더가 만든 목적지 상태와 복사 전후 상태를 확인한다. 일반 버퍼 CopyResource 경로를 DDS 텍스처에 그대로 쓰지 않는다.
4. PS용 읽기 상태로 전환을 기록하고 기존 Close/Execute/WaitForGPUComplete 경로로 제출·완료를 확인한 뒤 텍스처 UPLOAD 버퍼를 해제한다. Mesh의 업로드 해제와 텍스처 업로드 해제를 구분한다.
5. 그다음 실제 SRV 작성 → 루트 인덱스 1 GPU 테이블 바인딩 → PS Sample 출력 순으로 이어간다. 업로드와 화면 출력 전체를 한번에 구현하도록 요구하지 않는다.

범위는 일반 2D 텍스처 한 장이다. 실제 GetDesc/isCubeMap으로 차원·배열·밉·형식을 확인하고 큐브맵/배열/볼륨은 범위 밖으로 둔다. 사용자 데이터 확인 보고에 없는 구체적인 값은 추정해 기록하지 않는다.

### 계속 남아 있는 보완 항목

- CreateBufferResource의 DEFAULT 업로드 Map 대상은 UPLOAD로 수정됐고 COPY_DEST→요청 상태 전환도 확인된다. 하지만 null Device/필수 CommandList/업로드 출력, 같은 출력 변수, 크기0, 지원 힙 검사 및 READBACK 초기 데이터 거부는 아직 보완 대상이다.
- DEFAULT에 pData가 없으면 요청 상태 대신 COPY_DEST로 남고, 최종 상태가 COPY_DEST여도 동일 상태 배리어를 기록한다. 범용화 계약 보완은 계속 필요하다.
- 기존 소유 리소스를 출력 ComPtr에 교체할 때 GPU 수명을 보장해야 한다. 생성 실패 시 출력 보존은 지역 ComPtr 성공 후 전달 방식으로 개선 가능하다.
- 종료 GPU 대기 실패/초기화 부분 실패/Present/device removed/최종 Live Objects 보고 순서의 이전 보류 사항도 유지한다.
- 협업 규칙: 사용자가 코드 작성, AI는 한국어 설명/질문/리뷰, 한 번에 한 단계, 빌드·실행은 사용자 담당. ComPtr Blob 멤버와 AntialiasedLineEnable=TRUE 선택 유지.

### 새 대화 시작용 문구

> LEARNING_PROGRESS.md의 14절부터 읽고 현재 DX12Rebuild 코드를 확인해줘. Mesh/Shader 분리, 월드 행렬 상수 버퍼, UV 전달, SRV 힙/루트 테이블/정적 샘플러 준비를 했고 DDS 로딩 데이터도 정상 확인했어. 아직 텍스처 GPU 업로드·실제 SRV·테이블 바인딩·Sample 출력은 없어. 다음은 DDS CPU 데이터를 UPLOAD 버퍼에 배치하고 DEFAULT 텍스처로 복사를 기록하는 한 단계부터 설명해줘. 특히 CreateTexture의 지역 ddsData/subData 수명을 유지해야 해. 코드는 내가 작성하고 너는 가이드·질문·리뷰만 해줘. 임의 소스 수정이나 빌드·실행은 하지 마.

## 15. DDS 출력 완료와 게임 제작용 API로 방향 변경 (2026-10-14)

- 사용자는 DDS GPU 업로드 후 실행에 문제가 없다고 확인했고, 이후 SRV/테이블/HLSL Sample 연결 후 텍스처가 문제없이 표시된다고 보고했다. 기존 DDS 출력 목표는 달성했다.
- 현재 CreateTexture는 COMMON→COPY_DEST 배리어, UpdateSubresources, COPY_DEST→PIXEL_SHADER_RESOURCE 배리어를 기록한다. Subresource에 개수를 잘못 넣던 문제는 ALL_SUBRESOURCES로 수정됐다.
- BuildObjects는 Close/Execute/WaitForGPUComplete 성공 확인 뒤 텍스처와 Mesh의 업로드 버퍼를 해제한다. ddsData/subData는 함수 내부의 CPU→UPLOAD 복사가 완료될 때까지 유지한다.
- 실제 SRV는 CreateShaderResourceView의 pDesc=nullptr와 SRV 힙 CPU 시작 핸들로 작성한다. FrameAdvance는 힙 설정 뒤 루트1에 GPU 테이블 핸들, 루트0에 월드 CBV를 설정한다. HLSL Texture2D t0/SamplerState s0의 Sample 결과를 출력한다.
- d3dx12 최신 분리 헤더와 설치 SDK의 Tight Alignment 식별자 불일치 오류가 있었다. 사용자가 제공한 이전 단일 헤더의 업로드 헬퍼 존재/호환 가능성을 정적으로 확인했다. 현재 사용하는 헤더 경로·버전은 환경 변경 시 다시 확인한다.
- 이해 확인 보완: CPU 핸들은 디스크립터 작성용, GPU 핸들은 Draw의 테이블 위치 설정용이다. 루트 인덱스와 t0는 별개다. 업로드/읽기 상태 준비만으로 셰이더 바인딩이 이루어지지 않는다. ddsData는 CPU 메모리 소유자이고 subData는 그 내부 주소와 pitch 설명이며 데이터 자체를 소유하지 않는다.
- 새 목표: 과제에서 Texture 로딩, Mesh 파일 파싱, Object의 Mesh/Material 지정, Material의 Shader 선택, Scene의 객체 관리와 렌더링, Camera/입력을 간결한 클래스/API로 사용할 수 있게 한다.
- 새 계획은 **GAME_FRAMEWORK_PLAN.md의 7단계**다. 기간 제약으로 최초 14단계에서 범위까지 줄여 압축했다. 매번 한 단계씩 진행하며 리뷰/보완은 나눌 수 있다. 구현은 사용자가 한다는 기존 규칙은 유지한다.
- Review 참고 체크아웃은 실제 DescriptorTable이다. Shader가 Object를 소유하는 흐름은 복제하지 않고 Scene 소유/Renderer 렌더 책임/공유 에셋의 구성을 제안했다. 클래스/API 세부 이름은 각 단계에서 확정한다.
- 다음은 새 계획 1단계인 **리소스 생성 API와 Texture/공용 SRV 구조**다. 이후 Mesh/단일 포맷 파싱 → Shader/Material → Transform/GameObject → Scene/Renderer → Camera/입력 → 에셋 공유/통합/필수 정리로 진행한다. 별도 GraphicsDevice 추출, 고급 할당/캐시/비동기 구조는 미룬다. 안전한 실패 전파와 GPU 수명은 각 단계에서 유지하고 나머지 종료 점검은 7단계에 통합한다.
- 기존 GPU 대기 실패, Present/device removed, 부분 실패, 최종 Live Objects 보고 및 공통 버퍼 헬퍼 계약 문제는 새 계획의 해당 단계에서 해결한다.
- 이번 계획 작성에서는 기존 GameFramework.cpp/Shaders.hlsl 사용자 변경을 보존했다. 문서만 생성/갱신했으며 빌드·실행·소스 변경·커밋·푸시는 하지 않았다.

새 대화 시작 문구:

> LEARNING_PROGRESS.md의 15절과 GAME_FRAMEWORK_PLAN.md를 읽고 이어가자. DDS 텍스처 출력까지 완료했고 지금은 게임 제작에 적합한 Texture/Mesh/Material/Object/Scene/Camera API를 만드는 학습으로 방향을 바꿨어. 새 계획 1단계부터 한 소과제씩 안내해줘. 코드 작성과 빌드/실행/커밋은 내가 하고, 너는 설명·질문·리뷰만 담당해. 실제 코드를 확인해서 이미 완료된 기능을 되풀이하지 마.

## 16. 최신 인수인계: 모델·재질 출력 이후 구조 정리 중 (2026-11-03)

### 가장 먼저 확인할 현재 위치

- **다음 기능으로 넘어가지 말고, 모델 데이터 구조 리팩토링을 이어간다.** 사용자는 메시마다 GPU 버퍼를 생성하는 방식의 개선을 필수로 요청했고, 불필요한 Reader 자료형·상속·복잡한 참조 관계를 줄이고 싶어 한다.
- 7단계 계획 중 Texture/공용 SRV, 인덱스 Mesh, 과제 모델 파싱과 계층 렌더링까지 진행했다. Shader/Material 단계에서는 서브메시별 albedoColor 출력을 구현했다. Shader 설정 API 전체, GameObject, Scene/Renderer, Camera 클래스/입력, AssetManager는 아직 완료되지 않았다.
- 고정 View/Projection은 모델 표시를 위해 먼저 연결했다. 이것을 Camera 시스템 완료로 취급하지 않는다.
- 사용자는 계층 모델 정상 출력과 재질 색상 적용을 보고했다. **그 뒤 진행한 최신 구조 변경까지 빌드·실행이 확인된 것은 아니다.** 최신 저장 파일은 헤더와 구현부가 전환 중인 상태다.
- 이번 인수인계에서는 문서만 수정한다. 소스 수정·빌드·실행·테스트·커밋·푸시는 하지 않는다.

### 협업 규칙과 사용자 선택

- 사용자가 구현하고 AI는 한국어 개념 설명·작은 과제·질문·리뷰를 담당한다. 완성 코드 답안이나 임의 소스 수정을 제공하지 않는다.
- 한 번에 한 소단계만 진행한다. 오래 걸리는 작업과 빌드는 사용자가 실행한다. 커밋도 사용자 담당이다.
- 사용자 코드를 먼저 읽는다. 이미 고친 오류를 반복 지적하지 말고, 제안된 설계와 실제 적용된 변경을 구분한다.
- ComPtr은 COM 자원, unique_ptr/값은 단독 소유, shared_ptr은 실제 공유에 사용한다. GPU 완료와 참조 카운트는 별개다.
- 전체 개수의 정확한 일치 검증은 사용자가 후속 처리로 미뤘다. 이를 파서 진행의 반복적인 선결 조건으로 요구하지 않는다. 다만 실제 배열 접근과 Draw 범위의 안전성은 해당 경계에서 확보한다.
- 불필요한 클래스·래퍼·상속을 늘리지 않는다. 특히 FrameDataReader처럼 값만 담는 상속 자료형을 유지해야 한다고 강요하지 않는다.

### 완료 이력과 검증 수준

| 범위 | 현재까지 확인한 내용 |
|---|---|
| Texture/공용 SRV | Framework가 allocator를 단독 소유하고 allocator가 공용 shader-visible SRV 힙을 소유. Texture는 슬롯 핸들/리소스를 보관. 사용자가 DDS 두 장을 각각 로딩하고 선택 출력하는 데 성공 |
| 인덱스 Mesh | 정점/인덱스 DEFAULT 버퍼, UPLOAD 중간 버퍼, VBV/IBV, R32_UINT, DrawIndexedInstanced. 사용자 사각형 정상 출력 확인 |
| CPU 모델 파싱 | Abrams 계층 233프레임 파싱 확인. 부모는 프레임 벡터 위치로 연결. 재귀 중 벡터 재할당에 무효화되는 참조를 유지하지 않음 |
| GPU 계층 모델 | 원래 233개 프레임 위치에 대응하는 메시 목록 중 GPU 버퍼가 있는 메시 230개. 사용자 개수와 부모 변환 계산 결과 확인 후 모델 정상 출력 보고 |
| 행렬 전달 | 사용자 선택으로 root CBV 방식에서 루트 상수로 변경. World/View/Projection을 한 b0에 배치하고 구간별로 설정 |
| 기본 재질 | b1의 float4 albedoColor를 Pixel Shader에 전달. PS가 정점 색상과 기본 색상을 곱함. 사용자 색상 적용 확인 |
| 모델 버퍼 통합 | 최신 코드가 메시 배열을 합치고 CMesh 하나의 VB/IB를 생성하도록 변경됨. 정적 검토에서 누적 오프셋 계산 방향을 확인. 후속 데이터 구조 변경 및 최종 실행 검증은 진행 중 |

### 모델 파일과 번호의 의미

- 현재 기본 입력: Resources/AbramsTank.txt. 원본 참고 경로는 C:/Users/wlgns/Documents/Github/3DGP_Assignment4/3DGP_Assignment4/Models/ 아래 AbramsTank.txt 및 Apache.txt.
- 파일은 매우 긴 데이터 행을 포함한다. 전체 행을 그대로 출력하지 말고 태그/개수만 읽거나 출력 길이를 제한한다.
- Abrams: 233프레임, Mesh 블록 232개, 실제 정점/인덱스가 있는 메시 230개. 루트는 메시가 없고, Box01 및 Line04 메시가 비어 있다.
- 67,962정점, 370,263인덱스, 서브메시 444개. VertexTexcoord는 위치3+색4+UV2로 36바이트. 순수 VB/IB 데이터 합은 약 3.75 MiB다.
- 기존 개별 committed VB/IB 460개는 64 KiB 단위 올림 계산 시 DEFAULT 할당 약 29.3 MiB로 추정했다. 실행 측정값이 아니며 텍스처·추가 비용을 포함하지 않는다. 개선 목표는 모델당 DEFAULT VB/IB 두 개다.
- 두 모델 파일에는 UV가 없다. 현재 모델은 텍스처 매핑이 아니라 정점 색상(기본 흰색)과 파일 albedoColor를 이용해 표시한다. 노멀은 CPU로 파싱하지만 현재 입력 형식/셰이더에서 조명에 사용하지 않는다.
- 루트 Full_Abrams_1의 로컬 행렬에는 X=-5 이동이 있다. 원점을 바라볼 때 왼쪽에 보인 것은 원본 부모 변환이 적용된 결과다. 이 값이 현재도 유지되는지는 실제 파일/코드를 확인한다.
- SubMesh 태그 첫 숫자는 서브메시 번호, 다음은 인덱스 개수, 그 뒤는 정점 인덱스 값들이다. 현재 포맷은 같은 번호의 프레임 내부 Material 항목과 대응시키는 방식이다. 전역 재질 ID나 중복 제거 ID가 아니다.
- Material 태그 번호는 프레임 내부 번호다. 현재 파일은 0부터 순차적으로 저장되어 있어 push_back 순서와 벡터 위치가 일치한다. 기존 파서는 Material 태그의 숫자를 읽지 않고 추가하므로 임의 순서/결번을 지원한다고 설명하지 않는다.
- 같은 값의 재질도 다른 번호로 저장되어 있다. 실제 Abrams 9번 headlight 프레임의 재질 0/1은 저장된 속성이 같지만 별도 항목이다. 현재 파일이 중복 재질을 자동 통합한다고 가정하지 않는다.
- Apache는 이전 확인에서 Children 수와 실제 계층 종료 태그가 맞지 않는 원본 문제를 발견했다. Abrams를 우선 대상으로 사용하며 Apache 원본을 임의로 수정하지 않는다.

### 현재 바인딩 계약

| 루트 인덱스 | 타입/값 개수 | HLSL 연결 | Visibility |
|---|---|---|---|
| 0 | 루트 상수 48개 | b0, space0: World/View/Projection | Vertex |
| 1 | SRV 테이블 1개 | t0, space0 | Pixel |
| 2 | 루트 상수 4개 | b1, space0: float4 기본 색상 | Pixel |

- 0번의 World/View/Projection 목적지 오프셋은 각각 0/16/32이며 단위는 32비트 값이다. 각 행렬 설정 개수는 16. HLSL은 한 cbuffer에 행렬 3개를 순서대로 선언한다.
- 명시적 packoffset을 쓴다면 c0/c4/c8이다. c 하나는 16바이트로, API 오프셋 단위와 다르다.
- 루트 상수 pSrcData는 CPU 값의 주소다. GPU 주소를 전달하지 않는다. 호출 시 복사되므로 전치한 지역 XMFLOAT4X4의 주소를 사용할 수 있다.
- World를 바꿔도 같은 루트 파라미터의 나머지 View/Projection 구간은 유지된다. Reset 후 사용하는 구간을 다시 초기화한다. 일반 상수 버퍼의 동일 메모리 덮어쓰기와 구분한다.
- 전체 비용은 48+1+4=53 DWORD. 색상 개수를 16으로 선언하면 65가 되어 64 DWORD 한도를 넘는다. float4는 16바이트이지만 32비트 값은 4개다.
- 같은 visibility에서 같은 b 레지스터/space에 CBV와 루트 상수를 중복 선언하지 않는다. RegisterSpace는 이름 공간이며 루트 인덱스/버퍼 내부 오프셋이 아니다.
- 행 벡터 기준 Local * ParentWorld. CPU 로컬 행렬은 보존하고 전달 직전에 전치한다. VS는 위치 * World * View * Projection, PS는 input.color * matBaseColor.
- View/Projection은 Framework의 임시 고정 시점이다. 시야각은 XMConvertToRadians, 현재 near=0.01/far=100. 종횡비는 아직 config 상수 나눗셈이며 실제 클라이언트 크기의 실수 나눗셈/Resize 연계는 Camera 단계에서 정리한다.

### 버퍼 통합에서 유지할 계약

- 모든 메시의 정점을 한 배열에, 인덱스를 한 배열에 이어 붙인다. GPU CMesh 생성은 합친 뒤 한 번 수행한다.
- 각 프레임의 baseVertex는 그 메시를 추가하기 전 누적 정점 개수다. 같은 프레임의 서브메시는 이를 공유해도 된다.
- 통합 startIndex는 메시 추가 전 누적 인덱스 개수 + 원래 서브메시의 로컬 startIndex다. indexAmount는 그대로 유지한다.
- 실제 인덱스 값은 원래 메시 내부의 로컬 번호를 유지한다. Draw의 BaseVertexLocation으로 정점 기준 위치를 더한다. 인덱스 값에도 같은 오프셋을 더하면 이중 보정이다.
- startIndex/baseVertex는 바이트가 아닌 원소 개수다. 전체 VBV/IBV를 바인딩하고 DrawIndexedInstanced의 개수/시작/정점 기준 위치를 맞춘다.
- 메시가 없는 부모도 로컬 행렬/계층에 남긴다. 그릴 범위가 비어 있으면 Draw를 생략하되 모델 렌더링 함수 전체에서 return하지 않는다.
- 원본 CPU 배열은 업로드 헬퍼의 CPU memcpy가 끝나면 불필요해질 수 있다. GPU가 참조하는 UPLOAD 리소스는 Framework 제출/Fence 완료 뒤 해제한다.

### 최신 저장 코드: 완료된 보완과 전환 중인 부분

작성 중 사용자의 저장 내용이 변경되었다. 아래는 문서 작성 시 마지막으로 읽은 파일 기준이며, 다음 조력자는 현재 파일을 다시 확인한다.

- CreateBuffer 실패 반환값을 무시하던 문제는 **이미 수정됨**: 실패 시 LoadModelData가 false를 반환한다. 이를 미수정 과제로 반복 제시하지 않는다.
- Mesh.h는 ModelData에 합쳐진 CPU meshData를 추가했고, CModel 보관 데이터를 vector<FrameData> m_frameData로 바꾸는 중이다.
- Mesh.h의 SubMeshData에는 MaterialData materialData와 materialIndex가 추가됐지만, FrameData의 materialDatas 벡터도 남아 있다. 재질 포함 구조로 전환 중이며 아직 양쪽이 완전히 정리되지 않았다.
- CModel의 메시 소유 타입은 unique_ptr<CMesh>로 변경됐으나 이름은 m_spMesh이고, Mesh.cpp 생성부에는 make_shared<CMesh>가 남아 있었다. 소유 타입과 생성 방식/이름을 맞춰야 한다.
- FrameDataReader : FrameData 상속 자료형과 parseByTag의 Reader 인자가 남아 있다. 현재 ModelData.frames는 FrameData 목록이므로 Reader를 요구하는 호출과 맞지 않는다.
- Mesh.cpp는 아직 프레임별 frame.meshData를 접근하고, GetWorldMatrix/RenderModel 등에서 옛 m_modelData를 참조하는 부분이 남아 있다. 새 m_frameData 및 합쳐진 ModelData.meshData 흐름으로 전환이 필요하다.
- CMaterial의 SetDefaultMaterialData는 멤버를 저장하지만 BindMaterial은 PSO만 설정한다. SetMaterialData는 멤버를 바꾸지 않고 전달받은 albedoColor를 루트 상수로 기록한다. 저장된 기본값은 실제 출력에 사용되지 않는다.
- 공용 VBV/IBV를 서브메시 Draw마다 다시 바인딩하는 코드가 남아 있다. 모델 시작 시 Bind, 각 범위 Draw로 정리하는 것은 후속 작업이다.
- 옛 SetFramesData/행렬 버퍼 관련 주석과 미사용 흔적이 남아 있다. 최종 정리 시 선언·정의·호출을 함께 확인한다.
- 위 상태는 소스 정적 확인 결과다. 컴파일을 실행하지 않았으며 최신 저장본이 빌드된다고 보장하지 않는다.

### 합의한 정리 방향: 제안이며 아직 완료 아님

1. FrameDataReader/ModelDataReader 및 데이터 상속을 없앤다. 별도 Reader 클래스를 더 만들 필요는 없다. 읽는 함수와 읽은 결과 자료형을 구분한다.
2. FrameData에는 계층/로컬 행렬/baseVertex/서브메시 메타데이터만 둔다. SubMeshData에는 Draw 범위와 MaterialData 값을 직접 넣는 방향을 사용자가 검토·선택했다.
3. ModelData 하나를 로딩 중 임시 결과로 사용한다. 합쳐진 MeshData와 최종 프레임 목록을 보관하고, CModel은 GPU 생성 성공 후 프레임 목록만 가져간다. 런타임에 원본 정점·인덱스를 보관해야 한다고 가정하지 않는다.
4. 재질 블록은 서브메시 뒤에 나온다. Material 번호로 대응하는 서브메시를 찾아 값을 채운다. 항상 subMeshes.back()만 수정하지 않는다. 없거나 잘못된 대응은 기본 흰색으로 처리하고 미설정 루트 상수를 그대로 사용하지 않는다.
5. CMesh는 단독 소유(값/unique_ptr)와 GPU 버퍼 Bind/범위 Draw에 집중한다. 프레임 계층이나 재질 목록 전체를 알지 않게 한다. 여러 게임 객체의 에셋 공유는 이후 CModel 공유를 기준으로 검토한다.
6. CMaterial은 현재 역할에 맞춰 Shader와 전달받은 재질 값을 적용하는 Bind로 단순화한다. 기본값을 보관한다면 대체 재질에 실제로 사용하고, 사용하지 않으면 관련 멤버/API를 제거한다. 여러 객체가 공유하는 재질 값을 Draw마다 불필요하게 영구 변경하는 설계는 피한다.
7. 구조를 먼저 일관되게 완성한 뒤 파일 분리를 판단한다. 기능 없는 래퍼/Loader/Reader 클래스를 이름만 늘리지 않는다.

### 다음 조력자의 진행 순서

1. Mesh.h/.cpp, Material.h/.cpp, GameFramework.h/.cpp, Shaders.hlsl을 읽어 전환 중인 변경이 어디까지 반영됐는지 확인한다.
2. 다음 학습 과제는 **모델 데이터 구조 정리**다. Reader 상속 제거, 합친 CPU 데이터의 임시 수명, FrameData/SubMeshData와 재질 대응, 소유권 타입 일치를 먼저 작은 범위로 안내한다.
3. 사용자가 직접 구현한 뒤, 오류 전파와 오프셋/원래 로컬 인덱스 유지 여부를 정적으로 리뷰한다. 개별 버퍼 460개가 아닌 모델 VB/IB 두 개와 기존 형상·계층·색상이 유지되는지는 사용자가 실행해 확인한다.
4. 이 정리가 완료되면 Shader 설정 API 및 Material의 Texture 선택 등 3단계의 남은 부분으로 돌아간다. 이후 Transform/GameObject → Scene/Renderer → Camera/입력 → 에셋 공유·통합 순서를 유지한다.
5. 반복 로딩 시 기존 자원 교체는 GPU 완료를 보장한다. Present/초기화 부분 실패/최종 Live Objects 보고 순서, 공통 버퍼 헬퍼 계약 등 기존 보류 사항은 해당 단계에서 이어간다.

새 대화 시작 문구:

> LEARNING_PROGRESS.md의 16절과 GAME_FRAMEWORK_PLAN.md의 최신 진행 상태를 먼저 읽고 현재 코드를 확인해줘. 계층 모델과 서브메시별 기본 색상 출력까지 확인했고, 지금은 모델당 VB/IB 하나씩으로 통합하면서 FrameDataReader 상속과 CPU/실행용 데이터의 중복을 정리하는 중이야. 최신 헤더와 구현부는 전환 중이므로 빌드 가능하다고 가정하지 마. 다음 기능보다 이 리팩토링을 한 소과제씩 이어가자. 코드 작성·빌드·실행·커밋은 내가 하고, 너는 가이드·질문·리뷰와 요청한 문서 정리만 담당해.

## 17. 최신 인수인계: 모델 리팩토링 보완 및 Transform/GameObject 완료 (2026-11-03)

### 현재 위치와 검증 수준

- 확인한 최신 커밋은 `9652ccf` (GameObject 클래스 추가), 이전 관련 커밋은 `8c219d1` (Mesh 리팩토링) 및 `a1ad532` (브랜치 병합)이다. 최신 파일을 직접 읽었으며 미커밋 변경 여부와 원격 동기화까지 확인한 것은 아니다.
- **사용자가 공유 모델을 두 객체에 연결한 결과가 잘 작동한다고 보고했다.** AI는 정적 리뷰를 수행했고 마지막 getMatrix/DrawModel 연결 수정까지 확인했다. AI가 빌드·실행을 직접 수행하거나 모든 실패 경로를 검증한 것은 아니다.
- 다음은 **Scene이 객체를 소유하고 일괄 순회하는 작은 단계**다. 구체적인 Scene 구현 과제/API와 이해 확인 질문은 아직 제시하지 않았다. 이름만 있는 Renderer 등의 클래스를 한꺼번에 늘리지 않는다.
- 사용자 지시에 따라 Shader 파일·진입점·입력 레이아웃을 범용 설정화하는 과정은 건너뛰었다. 과제에 필요한 고정 Shader/파생 클래스 방식으로 계속한다. 이 선택을 완료된 설정 API로 기록하거나 다음 대화에서 다시 선결 과제로 요구하지 않는다.

### 모델 리팩토링의 실제 결과

- FrameDataReader/ModelDataReader와 Reader 상속을 제거했다. CModel의 private ModelData는 로딩 중 임시 결과이며, FrameData 목록과 프레임에 대응하는 MeshData 목록을 별도로 가진다. Mesh 없는 프레임에도 빈 MeshData를 추가해 인덱스를 대응시킨다.
- ReadFrameData 선언·정의와 태그 콜백은 새 자료형으로 일치한다. CModel 런타임 목록은 m_vFrameData다. 재귀 중 벡터 요소 참조를 장기간 보관하지 않는다.
- 로딩 후 combinedMesh에 정점/인덱스를 이어 붙이고 CMesh 하나의 DEFAULT VB/IB를 생성한다. 원래 메시 내부 인덱스는 유지하며 frame.baseVertex와 통합 subMesh.startIndex를 DrawIndexedInstanced에 전달한다.
- 지역 unique_ptr<CMesh> upMesh에 CreateBuffer를 수행하고 실패하면 false를 반환한다. 성공한 뒤에만 m_spMaterial, m_upMesh, m_vFrameData를 교체한다. 프레임 목록은 누적 insert가 아닌 move 대입이다.
- 기존 리소스 교체 전 GPU 완료는 여전히 호출자의 책임이다. 생성 함수 성공은 GPU 업로드 완료가 아니며, 실패한 명령 기록을 호출자가 무조건 제출해도 된다는 뜻도 아니다. 현재 초기화 실패 시 반환하는 흐름을 유지한다.
- SubMeshData.materialData를 직접 사용하는 방향은 최종 적용되지 않았다. **현재 실제 출력은 FrameData.materialDatas와 subMesh.materialIndex로 재질을 선택한다.** 16절의 재질 직접 포함 제안을 완료된 구조로 오인하지 않는다.
- RenderModel/DrawModel의 재질 선택은 MaterialData{}로 초기화하고 materialIndex < materialDatas.size()일 때 파일 값을 사용한다. 대응이 없으면 albedoColor=(1,1,1,1)을 매 Draw마다 기록한다.
- 이전 리뷰의 .at 예외, 반대 방향 범위 조건, 기본값 미초기화, GPU 생성 전 기존 Mesh 교체 문제를 순차적으로 수정했다. 이 문제들을 현재 코드 재확인 없이 미수정으로 반복 지적하지 않는다.

### 모델 파일에서 확인한 프레임당 Mesh 계약

- 현재 Resources/AbramsTank.txt는 233프레임 중 루트 1개에 Mesh가 없고 나머지 232개에 각각 Mesh 태그 1개다. 태그가 있어도 데이터가 빈 Mesh가 포함돼 실제 GPU 데이터가 있는 메시 개수와 다르다.
- 이전 Apache 샘플은 32프레임 중 루트 1개가 Mesh 없음, 31개가 Mesh 태그 1개다. 계층 종료 불일치는 기존 기록대로 남아 있다.
- 확인한 두 파일은 프레임당 Mesh가 0~1개이고 그 안에 여러 SubMesh가 들어가는 구조다. exporter 전체 명세를 검증한 보편 규칙은 아니며 현재 지원 범위의 계약이다.

### Transform과 GameObject 구현

- 현재 둘 다 Mesh.h/.cpp에 있다. 이 단계에서 파일 분리를 요구하지 않았다.
- Transform은 scale=(1,1,1), rotation/position을 보관한다. CGameObject 생성자에서 rotation/position을 (0,0,0)으로 초기화한다.
- Transform::getMatrix는 DirectXMath로 **Scale × Rotation × Translation**을 계산해 XMFLOAT4X4를 반환한다. 회전 단위는 라디안이다. CPU 월드 계산에서는 전치하지 않는다.
- CGameObject는 Transform을 값으로, CModel은 shared_ptr로 보관한다. SetModel 및 위치/회전/크기 변경 함수를 제공한다. DrawObject는 모델이 있을 때 getMatrix 결과를 DrawModel로 전달한다.
- CModel::DrawModel은 객체 World를 값 인자로 받으며 기본값은 단위 행렬이다. 각 프레임의 모델 내부 World에 객체 World를 곱하고 전달 직전에 전치해 루트 상수에 기록한다. 원본 로컬 행렬을 변경하지 않는다.
- Framework는 m_spModel을 한 번 로드하고 두 unique_ptr<CGameObject>에 공유한다. 객체1은 Y축 90도 회전, 객체2는 (2,0,2) 이동을 지정한다. FrameAdvance가 두 DrawObject를 호출한다.
- GPU 업로드 버퍼 해제는 기존 제출/Fence 성공 이후 공유 모델에 한 번 수행한다. 객체마다 모델 GPU 버퍼를 재생성하지 않는다.
- 루트 상수이므로 Draw마다 World 값을 기록하는 현재 방식에 별도 객체 ConstantBuffer를 추가할 필요는 없다. View/Projection 구간 및 재질 상수는 기존 계약을 유지한다.

### 이해 확인과 최근 리뷰

- 사용자는 객체마다 변환이 다름, 공유 모델의 로컬 행렬을 수정하면 모든 객체에 영향, 행렬 곱 순서를 바꾸면 결과가 달라짐을 올바르게 답했다.
- 마지막 답변은 “변환이 튄다”보다 회전/이동 적용 순서가 달라진다는 설명으로 보완했다. 회전 후 이동은 회전한 뒤 배치, 이동 후 회전은 이동한 위치도 원점 주위로 회전하는 결과가 된다.
- 비어 있던 Transform::getMatrix 반환을 구현했고 DrawObject의 중복 계산을 getMatrix 호출로 정리했다. 실제 선언·정의·호출을 확인했다.
- CGameObject 생성자에는 아직 빈 CModel을 make_shared하는 코드가 남아 있다. 불필요한 생성이지만 기능 오류나 다음 단계 차단 조건으로 보지 않는다. 사용자가 원하면 null 기본 포인터로 단순화한다.

### 다음 단계: Scene 객체 소유와 일괄 순회

1. 현재 Framework의 객체 두 개 개별 소유·개별 Draw 호출을 작은 Scene 목록으로 옮기는 가이드부터 제시한다.
2. Scene은 unique_ptr<GameObject> 목록을 소유하고 객체는 shared_ptr<Model>로 에셋을 공유하는 기존 방향을 유지한다. Scene에 추가하면 렌더 순회에 포함되도록 한다.
3. 모델 로딩/업로드 제출/Fence/Present는 당장 새 Scene에 모두 옮기지 않는다. Framework의 GPU 실행 흐름을 유지하며 한 책임씩 분리한다.
4. 현재 모델/재질 렌더링 연결을 활용한다. 별도 Renderer는 실제 책임 분리가 필요할 때 추가하고 큰 추상 계층을 먼저 만들지 않는다.
5. 기존 두 객체의 독립 위치/회전과 재질 색상이 Scene 경유에서도 유지되는 것이 다음 확인 기준이다. 그다음 Camera/입력, 에셋 공유·통합으로 진행한다.

### 남은 보완과 사용자 범위 선택

- SubMeshData.materialData 및 CMaterial의 저장 기본값/설정 API가 실제 출력에서 쓰이지 않는 흔적은 후속 정리 대상이다. 현재 작동하는 FrameData.materialDatas 경로를 잘못된 것으로 단정하거나 직접 포함 구조로 강제 전환하지 않는다.
- 공용 VBV/IBV를 서브메시마다 다시 바인딩하는 중복, getWorldMatrix의 재귀 계산 최적화, 미사용 행렬 버퍼 주석 등의 정리는 후속이다.
- Transform을 CGameObject 외부에서 독립 생성할 경우 rotation/position 기본값 보장 여부는 필요 시 정리한다. 현재 객체 생성자 초기화는 존재한다.
- 파서 전체 개수의 정확한 일치 검증은 사용자 선택으로 미뤘다. 실제 배열 접근/Draw 범위 안전성을 유지하되 이를 반복 선결 과제로 요구하지 않는다.
- 범용 버퍼 헬퍼 입력 계약, 초기화 부분 실패/GPU 대기 실패, Present/device removed, 최종 누수 보고 등 기존 보류 사항은 유지한다.
- 코드 작성·빌드·실행·커밋은 사용자 담당. AI는 한국어 가이드·질문·리뷰 및 요청된 문서 정리만 수행한다. 이번 기록은 소스 수정·빌드·실행·커밋·푸시 없이 문서만 갱신한다.

### 새 대화 시작용 문구

> LEARNING_PROGRESS.md의 17절과 GAME_FRAMEWORK_PLAN.md를 먼저 읽고 실제 코드를 확인해줘. 모델당 VB/IB 통합과 데이터 구조 보완을 했고, 같은 CModel을 두 GameObject가 공유하며 독립 Transform으로 출력되는 것을 확인했어. Shader 범용 설정화는 과제 수준에서 불필요해 건너뛰기로 했어. 다음은 Scene이 객체를 소유하고 일괄 순회하는 작은 단계부터 안내해줘. 지금 getMatrix와 DrawModel 연결은 구현됐으니 다시 요구하지 마. 코드 작성·빌드·실행·커밋은 내가 하고 너는 가이드·질문·리뷰와 요청된 문서 정리만 해줘.
