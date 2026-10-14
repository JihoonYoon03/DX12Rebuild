# 게임 제작용 DX12 프레임워크 개발 계획

작성/갱신: 2026-10-14. 기간 제약에 따라 최초 14단계 계획을 아래 7단계로 축소했다. 기존 DDS 출력 학습 이후의 새 개발 방향이다. 구체적 API 이름은 설계 제안이며 각 단계에서 현재 코드와 과제 요구에 맞춰 확정한다.

## 목표와 진행 방식

과제 코드는 GPU 리소스 생성 세부 사항을 반복 작성하지 않고, 에셋 로딩 → Mesh/Material 지정 → Object 배치 → Scene 갱신/렌더링으로 게임을 구성할 수 있게 한다. 카메라와 입력도 객체의 게임 동작과 분리한다.

- 사용자가 구현하고 AI는 한국어 가이드·질문·리뷰를 담당한다. 이번 요청은 개발 계획 수립이며 소스 구현 권한이 아니다.
- 한 번에 한 단계만 진행한다. 표의 7단계는 기능 단위이며 구현량에 따라 리뷰/보완은 여러 차례 필요할 수 있다. 14단계의 전체 작업을 이름만 묶는 대신 별도 장치 클래스, 고급 캐시/수명 관리, 여러 포맷 지원 등의 범위를 줄였다.
- 매 단계 기존 DDS 출력이 유지되는지 사용자가 실행한다. AI는 요청 없이 빌드·실행·긴 테스트·커밋·푸시를 하지 않는다.
- 먼저 동기식 로딩, 단일 DIRECT 큐, 단일 Allocator, 프레임별 GPU 대기를 유지한다. 성능 개선은 사용 가능한 API가 갖춰진 뒤 별도 진행한다.
- ComPtr로 COM 자원을, unique_ptr로 단독 소유를, shared_ptr로 실제 공유 에셋을 관리한다. 생성자에서 실패 가능한 GPU 초기화나 가상 함수 호출을 수행하지 않는다.
- 기존 클래스·함수의 유효한 구현은 옮겨 사용한다. 기능 없는 래퍼와 거대한 상속 계층을 늘리지 않는다.
- pch에 공통 선언을 두는 기존 선택은 유지할 수 있다. 파일 위치보다 단일 생성/업로드/제출의 책임과 입력·출력 계약을 먼저 정리한다.

## 확인한 기준과 Review에서 가져올 것

- Rebuild: CMesh/CShader 분리, DEFAULT 정점 업로드, 월드 행렬 root CBV, UV, DDS CPU 로딩, 텍스처 업로드, SRV 생성·바인딩·HLSL Sample까지 구현되어 있다. 사용자가 DDS 정상 출력을 보고했다.
- 문서 작성 전 GameFramework.cpp와 Shaders.hlsl에 미커밋 사용자 변경이 있었다. 이를 보존한다.
- 현재 DDS SRV는 CreateShaderResourceView의 pDesc=nullptr로 생성한다. 현재 지원 텍스처에는 기본 뷰가 사용되고 있으며, Texture API에서는 지원할 차원/형식/밉 계약을 명시한다.
- Review의 실제 체크아웃은 DescriptorTable이다. 로컬·원격 조회에 DescriptorHeap 브랜치는 없었으며 브랜치를 변경하지 않았다. 이후 사용자가 다른 참고 ref를 지정하면 다시 확인한다.
- Review GameObject/Material의 Mesh·Shader 연결, Scene의 Update/Render, Camera의 View/Projection 개념을 참고한다.
- Review의 Scene은 Shader 목록을, CObjectsShader는 Object 목록을 소유하는 부분이 있다. Rebuild에서는 Scene이 Object를 소유하며 Shader는 GPU 프로그램/PSO 준비에 집중한다.
- Camera가 Player와 GPU 버퍼까지 묶는 Review의 방식은 그대로 복제하지 않는다. Camera는 시점 계산, Controller는 입력/추적, Renderer는 GPU 전달을 맡는다.

## 목표 클래스와 API의 책임

| 구성 | 책임 | 게임 코드가 사용하는 개념/API 예시 |
|---|---|---|
| GameFramework | 창·메시지·시간·게임 전체 수명, 활성 Scene 연결 | Initialize, Tick, ChangeScene |
| Framework의 GPU 실행 부분 | 기존 Device/Queue/SwapChain/Fence, 프레임 시작·종료·Resize 흐름 유지 | 현재 프레임/업로드 제출 API 활용 |
| UploadContext 또는 작은 업로드 보관 기능 | 초기 복사를 기록하고 완료까지 중간 버퍼 보관. 제출·대기는 Framework 담당 | 업로드 기록, 중간 자원 보관/완료 후 해제 |
| Buffer/ResourceFactory | 단일 버퍼 생성과 초기 업로드의 분명한 계약 | CreateUploadBuffer, CreateDefaultBuffer |
| DescriptorAllocator | 고정 크기 shader-visible SRV 힙과 슬롯 할당 | Allocate, CPU/GPU 핸들 조회 |
| Texture / TextureLoader | DEFAULT 이미지 리소스와 SRV 슬롯, DDS 파일 로딩 | LoadDDS(path, uploadBatch) |
| MeshData / Mesh / MeshLoader | CPU 정점·인덱스 데이터, GPU 버퍼·뷰, 과제 포맷 하나 파싱 | LoadMeshData(path), CreateMesh(data), Draw |
| Shader/Pipeline | 셰이더 바이트코드, 입력 형식, PSO와 공통 바인딩 규칙 | Create(desc), Bind |
| Material | 사용할 Shader/Pipeline, Texture, 색상 등 재질 데이터 | SetShader, SetBaseColorTexture, SetTint |
| Transform / GameObject | 위치·회전·크기, Mesh와 Material 조합, 객체 상태 | SetPosition, SetMesh, SetMaterial |
| Scene | Object 소유, 활성 Camera, 게임 업데이트와 객체 순회 | AddObject, Update, Render |
| Camera / CameraController | View/Projection 계산과 별도의 입력/추적 동작 | LookAt, SetPerspective, Update |
| Renderer | Scene/Camera 정보로 상수·Material·Mesh 바인딩 및 Draw 기록 | Render(scene, camera) |
| AssetManager | 경로 기준 에셋 로딩·공유·캐시와 에셋 루트 경로 | LoadTexture, LoadMesh, LoadShader |

API 이름은 완성 코드 답안이 아니다. 별도 GraphicsDevice 추출은 이번 7단계의 필수가 아니다. Framework의 안정된 DX12 코드를 유지하고 작은 Renderer/RenderContext로 그리기 책임만 구분한다. UploadContext도 필요한 데이터와 함수부터 시작하며 클래스 이름마다 반드시 파일을 만들지는 않는다.

## 소유권과 프레임 규칙

- Framework는 기존 GPU 객체와 Renderer/활성 Scene을 소유한다. 같은 수명의 작은 유틸리티는 값 멤버도 가능하다.
- Scene은 vector<unique_ptr<GameObject>>로 Object를 소유한다. Transform은 Object의 값 멤버다. 외부 Object 참조는 비소유이며 제거 이후 유효하지 않다는 규칙을 문서화한다. 안정적인 Object ID는 필요 단계에서 도입한다.
- Object는 shared_ptr<Mesh>와 shared_ptr<Material>을 가진다. 여러 Object가 동일 Mesh를 사용해도 위치는 각각 다르다.
- Material은 shared_ptr<Shader/Pipeline>과 shared_ptr<Texture>를 가진다. 공유 Material 수정은 이를 사용하는 모든 Object에 영향을 준다. 독립 색상 변경은 별도 Material 또는 Object별 오버라이드로 구분한다.
- 자식이 부모 Scene/Renderer를 shared_ptr로 되잡지 않는다. 게임 코드에는 ID3D12Device/CommandList 등 COM 소유권을 전달할 필요가 없게 한다. 내부 호출은 비소유 참조/포인터를 기본으로 한다.
- shared_ptr 참조 카운트와 GPU 사용 완료는 별개다. 리소스/디스크립터 슬롯을 실제로 해제하거나 재사용하는 시점은 Fence 완료 조건을 따라야 한다.
- Object/Frame/Material 상수 버퍼와 descriptor heap의 수명을 GPU 사용 완료까지 보장한다. 4단계는 객체별 단순 ConstantBuffer부터 연결하고 5단계의 Renderer가 그리기 바인딩을 담당한다.
- 초기에는 씬 생성·에셋 로딩·교체·삭제를 GPU 완료가 확인된 프레임 경계에서 수행한다. 런타임 비동기 로딩과 지연 해제는 나중 과제다.

프레임 흐름은 입력 수집 → Scene/Controller Update(dt) → 기존 프레임 시작 → Scene 객체 순회/Renderer의 카메라·객체·Material 준비와 Mesh Draw → 기존 프레임 종료다. Scene을 한 번 렌더링한다는 것은 한 번의 상위 API 호출로 모든 Object를 처리한다는 뜻이다. 여러 Object를 자동으로 한 GPU Draw로 합치는 것은 아니며 배치/인스턴싱은 후속 최적화다.

## 개발 진도: 7단계

| 단계 | 구현/학습 범위 | 사용자 실행·검토 완료 기준 |
|---|---|---|
| 1. 리소스 생성 API와 Texture | 기존 버퍼 헬퍼의 필수 입력/실패 계약 정리. DDS 생성/업로드/SRV를 TextureLoader로 추출. 고정 용량 공용 SRV 슬롯, 완료까지 중간 자원 보관. 기존 Framework의 제출/대기 유지 | 파일 경로로 Texture를 생성하고 텍스처 2개를 한 힙에서 선택 가능. 잘못된 경로/필수 입력은 안전하게 실패 |
| 2. Mesh와 파일 로딩 | CPU MeshData와 GPU Mesh를 분리. 인덱스 버퍼/DrawIndexed, 정점 형식 일치. 과제 포맷 하나를 CPU 데이터로 파싱하며 인덱스 범위 검사 | 파일에서 읽은 Mesh를 기존 셰이더/텍스처로 표시 |
| 3. Shader와 Material | 기존 CShader를 유지하며 필요한 파일/진입점/입력 형식만 설정화. 공통 바인딩 규칙. Material이 Shader, Texture, tint를 보관. 불투명 기본 재질만 지원 | 동일 Mesh의 Shader/Texture/색상을 Material로 선택 |
| 4. Transform과 GameObject | 위치/회전/크기와 World 계산, Mesh/Material 지정. 객체별 독립 ConstantBuffer로 먼저 구현. 행렬 전치 경계 통일 | Mesh를 공유하는 객체 두 개가 독립 변환/재질로 표시 |
| 5. Scene과 렌더링 흐름 | Scene이 Object 소유/Update/Render를 담당. 작은 Renderer/RenderContext가 Object→Material→Mesh 바인딩. 목록 단순 순회, 기존 프레임 실행 유지 | 객체를 Scene에 추가하는 것만으로 화면에 포함 |
| 6. Camera와 기본 조작 | View/Projection, Frame 상수, Resize aspect. 기본 입력 상태와 dt 기반 자유 이동/마우스 회전 Controller | 원근 공간에서 WASD/마우스로 카메라 조작, 화면 비율 유지 |
| 7. 에셋 공유·통합·필수 정리 | 에셋 루트 경로와 단순 Mesh/Texture/Shader 캐시. 씬 교체/객체 삭제는 GPU 완료 프레임 경계에서 수행. 초기화/종료 실패, Map/HANDLE, Present 및 최종 Live Objects 순서 정리 | 파일 에셋과 공유 Material로 작은 과제 씬 제작, 로딩 실패/씬 교체/종료 확인 |

다음은 1단계의 Texture/공용 SRV API 구성부터 시작한다. 자원 생성 실패·업로드 대기 실패처럼 수명에 직접 영향을 주는 오류는 각 단계에서 바로 다룬다. 나머지 초기화/종료 정리는 7단계에서 통합하며 Device 재생성 복구는 포함하지 않는다. 7단계는 7번의 대화나 정해진 기간을 보장하는 수치가 아니다.

## 중요한 렌더링 설계 결정

- 모든 텍스처를 공용 SRV 힙의 슬롯으로 관리한다. Texture마다 힙을 만들거나 Material마다 힙을 바꾸지 않는다. 힙 고정 용량은 명시하고 런타임 자동 확장/핸들 이동은 처음에는 하지 않는다.
- 바인딩 레이아웃은 Renderer 측에서 통일한다. 기존 Object CBV b0/루트0, Texture t0/루트1을 유지하면서 Camera/Frame b1과 Material b2를 필요한 단계에 추가하는 방향이다. 실제 루트 인덱스는 이름 있는 상수로 관리하며 HLSL 레지스터 번호와 구분한다. 변경 시 해당 레이아웃으로 생성한 모든 PSO도 맞춘다.
- 첫 Material은 기본 색상 Texture 1개와 색상 tint를 지원한다. 정적 샘플러 1개를 유지하고 재질별 필터/주소 모드는 필요 시 PSO/루트 레이아웃 또는 sampler heap 설계를 별도로 추가한다.
- GameObject와 Camera는 CPU의 게임 데이터가 중심이다. Object별 작은 ConstantBuffer로 먼저 정확성을 확보하고 Renderer가 바인딩한다. 공용 상수 버퍼 풀·프레임별 할당 구조는 나중에 도입한다.
- 여러 Object의 월드 행렬을 하나의 같은 버퍼 주소에 덮어쓰며 Draw하는 방식은 사용하지 않는다. 각각의 Draw가 GPU에서 읽을 독립 구간이 필요하다. 현재 프레임 대기는 이전 프레임과의 충돌만 막는다.
- 같은 Material/PSO를 기준으로 Draw 순서를 묶는 것은 7단계 이후의 최적화다. 초기에는 Scene 목록을 단순 순회한다. 투명 렌더링의 깊이 정렬은 별도 규칙이다.
- Mesh 파서는 과제 요구 포맷을 우선한다. 아직 형식이 정해지지 않았으므로 OBJ/glTF/Blender 바이너리 전체 지원을 확정하지 않는다. 단순 과제 포맷은 직접 파싱하고 복잡한 표준 포맷은 라이브러리 사용 여부를 해당 단계에서 판단한다.
- 행렬은 일관된 좌표계와 단위를 문서화한다. 초기에는 DirectXMath LH 및 행 벡터 규칙을 사용하고 HLSL 전달 시 전치 경계를 한 곳으로 모으는 방향이다. 파일 포맷의 좌표계 변환은 CPU 가져오기 단계에서 수행한다.

## 먼저 미루는 기능

기간 단축을 위해 별도 GraphicsDevice 추출, 일반화된 RenderItem 큐, Draw 정렬, 공용 상수 버퍼 풀, SRV 힙 자동 확장/슬롯별 지연 회수, 다중 Mesh 포맷과 여러 Material의 Submesh, AssetManager의 자동 제거 정책을 미룬다. 공용 SRV 힙은 증가형 할당과 명시적 용량 제한을 사용하며 반복 씬 로딩의 슬롯 사용량도 확인한다. 필요 시 GPU 대기와 기존 에셋 수명을 보장한 전체 재생성 방식부터 판단한다.

ECS, 범용 컴포넌트/리플렉션 시스템, 에디터, 직렬화 엔진, 비동기 에셋 스트리밍, bindless, render graph, 프레임 병렬화, 다중 큐, 스키닝/스켈레탈 애니메이션, PBR는 현재 기본 프레임워크 완료 조건에 포함하지 않는다. 충돌·조명·투명도·UI·Player 행동 등 과제에 꼭 필요한 기능은 실제 요구에 맞춰 별도 소단계로 추가한다. 기간이 촉박해도 GPU 완료 전 해제나 여러 객체의 같은 상수 버퍼 덮어쓰기를 허용하지 않는다.

## 기술 근거와 검증 원칙

- shader-visible CBV/SRV/UAV 힙은 동시에 하나만 바인딩하며 슬롯 변경·재사용에도 GPU 동기화가 필요하다: [Microsoft Descriptor Heaps Overview](https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptor-heaps-overview).
- 명령 리스트는 전달받은 GPU 객체의 COM 참조를 유지하지 않으므로 스마트 포인터만으로 사용 중 자원 수명이 자동 보장되지는 않는다: [Microsoft command list recording](https://learn.microsoft.com/en-us/windows/win32/direct3d12/recording-command-lists-and-bundles).
- PSO 설정과 Draw마다 바꾸는 바인딩 데이터는 구분한다: [Microsoft pipeline state](https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12).
- 클래스 분리·소유권 배치는 위 API 제약과 현재 학습 목표를 바탕으로 한 이 프로젝트의 설계 제안이다. Microsoft가 요구하는 클래스 구조는 아니다.

노트북 새 대화에서는 LEARNING_PROGRESS.md의 최신 절과 이 계획을 함께 읽고, 완료된 부분을 되풀이하지 않으면서 한 소과제씩 이어간다.
