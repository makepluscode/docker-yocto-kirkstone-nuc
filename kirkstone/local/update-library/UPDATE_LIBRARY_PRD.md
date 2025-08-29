# Update Library PRD (Product Requirements Document)

## 1. 제품 개요

### 1.1 목적
현재 update-service는 RAUC D-Bus 통신을 통해 업데이트 기능을 제공하고 있습니다. 이 프로젝트는 RAUC의 핵심 기능을 직접 라이브러리로 포팅하여 D-Bus 의존성을 제거하고, update-service에서 직접 호출할 수 있는 update-library를 구현하는 것을 목표로 합니다.

### 1.2 배경
- **문제**: 현재 update-service는 RAUC D-Bus 서비스에 의존
- **해결책**: RAUC 기능을 직접 포팅한 라이브러리 제공
- **이점**: D-Bus 의존성 제거, 성능 향상, 직접 제어 가능

### 1.3 범위
- **포함**: Bundle 설치, 슬롯 상태 조회 기능
- **제외**: RAUC의 고급 기능 (casync, 암호화 등)

## 2. 기술 요구사항

### 2.1 핵심 기능

#### 2.1.1 Bundle 설치 (Install)
- **기능**: RAUC 번들(.raucb) 파일 설치
- **입력**: 번들 파일 경로
- **출력**: 설치 성공/실패, 진행률, 상태 메시지
- **요구사항**:
  - 번들 서명 검증
  - 호환성 검사
  - 슬롯 선택 및 설치
  - 실시간 진행률 업데이트
  - 오류 처리 및 롤백

#### 2.1.2 슬롯 상태 조회 (Status)
- **기능**: 시스템의 모든 슬롯 상태 정보 제공
- **출력**: 슬롯별 상태 정보 (활성/비활성, 부트 슬롯, 버전 등)
- **요구사항**:
  - 부트 슬롯 식별
  - 슬롯별 메타데이터 수집
  - 활성/비활성 상태 확인

### 2.2 기술 스택

#### 2.2.1 언어 및 라이브러리
- **언어**: C (RAUC 포팅), C++ (인터페이스 래퍼)
- **필수 라이브러리**:
  - GLib 2.0: 기본 데이터 구조 및 유틸리티
  - GIO: 파일 I/O 및 스트림 처리
  - OpenSSL: 암호화 및 서명 검증
  - DLT (선택): 로깅 (기존 시스템과의 호환성)

#### 2.2.2 빌드 시스템
- **CMake**: 크로스 플랫폼 빌드 관리
- **Static Library**: 배포 및 의존성 관리 단순화

### 2.3 아키텍처

#### 2.3.1 계층 구조
```
┌─────────────────────────────────────┐
│          update-service             │
│      (기존 D-Bus 브로커 서비스)      │
├─────────────────────────────────────┤
│          UpdateClient               │
│        (C++ 인터페이스)             │
├─────────────────────────────────────┤
│          RaucEngine                 │
│       (RAUC 기능 래퍼)              │
├─────────────────────────────────────┤
│      RAUC Core Functions            │
│    (포팅된 C 함수들)                │
│  • bundle.c  • install.c           │
│  • slot.c    • manifest.c          │
│  • checksum.c • utils.c             │
└─────────────────────────────────────┘
```

#### 2.3.2 데이터 흐름
1. **update-service** → **UpdateClient::install()** 호출
2. **UpdateClient** → **RaucEngine::installBundle()** 위임
3. **RaucEngine** → RAUC 포팅 함수들 호출 (check_bundle, do_install_bundle)
4. **진행률/완료** → 콜백을 통해 update-service로 전달

## 3. 인터페이스 설계

### 3.1 주요 클래스

#### 3.1.1 UpdateClient (Public API)
```cpp
class UpdateClient {
public:
    bool initialize(const std::string& config_file_path = "");
    bool install(const std::string& bundle_path);
    std::vector<SlotInfo> getSlotStatus();
    std::string getBootSlot();
    std::string getCompatible();
    bool getBundleInfo(const std::string& bundle_path, std::string& compatible, std::string& version);

    void setCompletedCallback(CompletedCallback callback);
    void setProgressCallback(ProgressCallback callback);
    void setErrorCallback(ErrorCallback callback);

    bool isInitialized() const;
    bool isInstalling() const;
};
```

#### 3.1.2 데이터 타입
```cpp
struct ProgressInfo {
    int percentage;       // 0-100
    std::string message;
    int nesting_depth;
};

struct SlotInfo {
    std::string slot_name;
    std::map<std::string, std::string> properties;
};

enum class InstallResult {
    SUCCESS = 0,
    FAILURE = 1,
    CANCELLED = 2
};
```

### 3.2 콜백 시스템
- **ProgressCallback**: 설치 진행률 업데이트
- **CompletedCallback**: 설치 완료 알림
- **ErrorCallback**: 오류 발생 알림

## 4. 구현 범위

### 4.1 포팅할 RAUC 함수들

#### 4.1.1 Bundle 관련
- `check_bundle()`: 번들 검증 및 서명 확인
- `mount_bundle()` / `umount_bundle()`: 번들 마운트/언마운트
- `load_manifest_from_bundle()`: 매니페스트 로드

#### 4.1.2 Install 관련
- `do_install_bundle()`: 메인 설치 함수
- `determine_slot_states()`: 슬롯 상태 파악
- `determine_boot_states()`: 부트 상태 파악
- `determine_target_install_group()`: 설치 대상 슬롯 선택

#### 4.1.3 Slot 관련
- `r_slot_get_booted()`: 부트 슬롯 식별
- `r_slot_slotstate_to_str()`: 상태 문자열 변환

#### 4.1.4 유틸리티
- 설정 파일 파싱
- 체크섬 계산 및 검증
- 마운트 포인트 관리
- 오류 처리

### 4.2 제외할 기능들
- **D-Bus 관련 코드**: service.c, D-Bus 인터페이스
- **고급 기능**: casync, 암호화 번들, NBD
- **CLI 인터페이스**: main.c, 명령줄 파싱
- **네트워크 기능**: HTTP 다운로드 등

## 5. 성능 및 제약사항

### 5.1 성능 요구사항
- **설치 시간**: 기존 RAUC D-Bus 대비 동등하거나 향상
- **메모리 사용량**: 100MB 이하 (대용량 번들 처리 시)
- **응답성**: 진행률 업데이트 1초 이내

### 5.2 제약사항
- **플랫폼**: Linux x86_64 (Intel NUC 타겟)
- **파일시스템**: ext4, GRUB 부트로더 환경
- **권한**: root 권한 필요 (마운트, 파티션 조작)

## 6. 테스트 전략

### 6.1 단위 테스트
- 각 포팅된 함수의 정확성 검증
- 번들 검증 로직 테스트
- 슬롯 상태 파싱 테스트

### 6.2 통합 테스트
- update-service와의 연동 테스트
- 실제 번들 설치 시나리오 테스트
- 오류 상황 처리 테스트

### 6.3 성능 테스트
- 대용량 번들 설치 성능 측정
- 메모리 사용량 프로파일링

## 7. 배포 및 호환성

### 7.1 배포 방식
- **정적 라이브러리**: libupdate-library.a
- **헤더 파일**: include/update-library/
- **CMake 설정**: FindUpdateLibrary.cmake

### 7.2 호환성
- **기존 시스템**: 기존 RAUC 설정 파일 호환
- **번들 포맷**: 표준 RAUC 번들 형식 지원
- **로깅**: DLT 시스템과 연동

## 8. 위험 요소 및 완화 방안

### 8.1 주요 위험
- **복잡성**: RAUC 코드 포팅의 복잡성
- **호환성**: 기존 RAUC 동작과의 차이
- **디버깅**: D-Bus 없이 문제 진단의 어려움

### 8.2 완화 방안
- **단계적 구현**: 핵심 기능부터 순차 구현
- **철저한 테스트**: 기존 RAUC와 동작 비교 테스트
- **로깅 강화**: 상세한 디버그 로그 제공

## 9. 일정 및 마일스톤

### Phase 1: 기반 구조 (1-2일)
- [ ] 프로젝트 구조 설정
- [ ] CMake 빌드 시스템 구성
- [ ] 기본 헤더 파일 포팅

### Phase 2: 핵심 기능 포팅 (3-5일)
- [ ] Bundle 검증 기능 포팅
- [ ] Install 엔진 포팅
- [ ] Slot 관리 기능 포팅

### Phase 3: 통합 및 테스트 (2-3일)
- [ ] UpdateClient 인터페이스 구현
- [ ] update-service 연동
- [ ] 테스트 및 검증

### Phase 4: 최적화 및 문서화 (1-2일)
- [ ] 성능 최적화
- [ ] 문서 작성
- [ ] 배포 준비

## 10. 성공 기준

### 10.1 기능적 성공 기준
- [ ] RAUC 번들 설치 정상 동작
- [ ] 슬롯 상태 조회 정확성
- [ ] 기존 update-service API 호환성 유지

### 10.2 비기능적 성공 기준
- [ ] D-Bus 의존성 완전 제거
- [ ] 설치 성능 기존 대비 동등 이상
- [ ] 메모리 사용량 목표 달성

### 10.3 품질 기준
- [ ] 단위 테스트 커버리지 80% 이상
- [ ] 메모리 누수 없음
- [ ] 안정적인 오류 처리
