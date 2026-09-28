# DX12 Rebuild 학습 인수인계 — 메인 PC용

최종 갱신: 2026-09-28. 현재 코드와 이번 대화의 사용자 실행 확인을 반영했다. 이전의 “PSO/정점 버퍼 미구현” 기록을 대체한다. 다른 컴퓨터에서는 실제 파일을 먼저 확인한다.

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

- 문서 갱신 직전 HEAD: `4f52168` (삼각형 띄우기 (단일 업로드 버퍼)). 이전 PSO 커밋: `7375406`. 당시 작업 트리는 깨끗했다.
- 이번 문서 갱신은 커밋·푸시를 수행하지 않는다. 원격 반영 여부도 확인하지 않았다. 메인 PC에서 코드와 이 문서가 함께 반영되었는지 확인한다.
- Mesh.h/.cpp, 프로젝트 및 filters, Shaders.hlsl을 포함한 저장소 파일을 이동한다. .vs, obj, PCH 및 빌드 산출물은 복사하지 않고 대상 PC에서 새로 생성한다.
- 네 구성 모두 PlatformToolset=v145, Windows SDK 설정은 10.0이다. 메인 PC의 Visual Studio C++ 도구/SDK를 확인하고 임의로 도구 집합 버전을 바꾸지 않는다.
- 공통 IntDir은 `$(ProjectDir)obj\$(ProjectName)\$(Platform)\$(Configuration)\`이다.
- 과거 C1853(PCH 호환 문제), MSB8028(중간 폴더 공유 경고) 이후 IntDir을 분리한 이력이 있다. 이후 사용자가 현재 코드 정상 실행을 확인했다. 메인 PC 재빌드 성공까지 확인된 것은 아니다.
- 런타임의 Shaders.hlsl 상대 경로는 실행 작업 디렉터리 기준이다. 대상 PC의 디버깅 작업 디렉터리와 파일 위치를 확인한다.
- 대화 자동 동기화에 의존하지 않고 아래 문구와 이 문서로 이어간다.

## 9. 메인 PC 새 대화 시작용 문구

> LEARNING_PROGRESS.md를 먼저 읽고 현재 DX12Rebuild 코드를 확인해줘. PSO와 UPLOAD 정점 버퍼를 통한 RGB 삼각형 출력까지 구현·리뷰했고 내가 정상 실행을 확인했어. 다음은 Mesh/Shader 역할 분리와 리소스 수명 단계이며, 우선 CMesh로 정점 버퍼와 그리기 책임을 옮기는 작은 단계부터 설명해줘. 코드는 내가 작성하고 너는 한국어 가이드·질문·리뷰만 담당해. 완성 코드 제공이나 임의 수정, 빌드·실행은 하지 마. AntialiasedLineEnable=TRUE는 유지하기로 했어. 기존 구현을 먼저 확인하고 한 번에 한 단계씩 진행하자.
