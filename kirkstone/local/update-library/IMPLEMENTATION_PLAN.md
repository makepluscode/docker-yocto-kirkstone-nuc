# Update Library 구현 계획서

## 1. 구현 전략

### 1.1 포팅 접근법
RAUC의 방대한 코드베이스에서 필요한 부분만 선별적으로 포팅하여 경량화된 라이브러리를 구현합니다.

**선택적 포팅 기준:**
- ✅ **포함**: Bundle 검증, Install 엔진, Slot 관리, 설정 파싱
- ❌ **제외**: D-Bus, CLI, 네트워크, casync, 고급 암호화

### 1.2 아키텍처 설계

```
update-library/
├── include/
│   ├── update_client.h         # 메인 공개 API
│   ├── rauc_engine.h          # RAUC 엔진 래퍼
│   ├── update_types.h         # 공통 데이터 타입
│   └── rauc/                  # 포팅된 RAUC 헤더들
│       ├── bundle.h           # 번들 검증 및 처리
│       ├── install.h          # 설치 엔진
│       ├── slot.h             # 슬롯 관리
│       ├── manifest.h         # 매니페스트 파싱
│       ├── checksum.h         # 체크섬 계산
│       ├── config_file.h      # 설정 파일 파싱
│       └── utils.h            # 유틸리티 함수
├── src/
│   ├── update_client.cpp      # C++ 인터페이스 구현
│   ├── rauc_engine.cpp        # 엔진 래퍼 구현
│   └── rauc/                  # 포팅된 RAUC C 코드
│       ├── bundle.c           # 번들 검증 로직
│       ├── install.c          # 설치 엔진 로직
│       ├── slot.c             # 슬롯 상태 관리
│       ├── manifest.c         # 매니페스트 처리
│       ├── checksum.c         # 체크섬 계산
│       ├── config_file.c      # 설정 파일 파서
│       ├── utils.c            # 유틸리티 함수
│       ├── context.c          # 전역 컨텍스트 관리
│       ├── mount.c            # 마운트/언마운트
│       ├── mark.c             # 슬롯 마킹
│       ├── signature.c        # 서명 검증
│       └── bootchooser.c      # 부트로더 연동
└── test/
    └── test_main.cpp          # 기본 테스트
```

## 2. 단계별 구현 계획

### Phase 1: 기반 구조 설정 (완료)
- [x] 프로젝트 디렉토리 구조 생성
- [x] CMake 빌드 시스템 구성
- [x] 기본 인터페이스 헤더 정의
- [x] 데이터 타입 정의

### Phase 2: RAUC 핵심 헤더 포팅
**목표:** RAUC의 핵심 헤더 파일들을 라이브러리에 맞게 포팅

#### 2.1 우선순위 1 - 기본 구조체 및 유틸리티
```bash
# 포팅 대상 헤더들
include/rauc/utils.h         # 기본 유틸리티
include/rauc/context.h       # 전역 컨텍스트
include/rauc/config_file.h   # 설정 파일 파싱
include/rauc/checksum.h      # 체크섬 계산
```

#### 2.2 우선순위 2 - Bundle 및 Manifest
```bash
include/rauc/manifest.h      # 매니페스트 구조
include/rauc/bundle.h        # 번들 검증
include/rauc/signature.h     # 서명 검증
```

#### 2.3 우선순위 3 - Slot 및 Install
```bash
include/rauc/slot.h          # 슬롯 관리
include/rauc/install.h       # 설치 엔진
include/rauc/mount.h         # 마운트 관리
include/rauc/mark.h          # 슬롯 마킹
include/rauc/bootchooser.h   # 부트로더 연동
```

### Phase 3: C 코드 포팅 및 수정
**목표:** RAUC C 소스를 라이브러리 환경에 맞게 포팅

#### 3.1 포팅 전략
1. **의존성 제거**: D-Bus, 네트워크, CLI 관련 코드 제거
2. **API 단순화**: 복잡한 옵션들을 기본값으로 설정
3. **콜백 추가**: 진행률 및 상태 업데이트를 위한 콜백 시스템 추가
4. **오류 처리**: GError를 표준 C++ 예외나 bool 반환값으로 변환

#### 3.2 수정이 필요한 주요 함수들
```c
// bundle.c - 번들 검증
gboolean check_bundle_simplified(const gchar *bundlename, RaucBundle **bundle);
gboolean mount_bundle_simplified(RaucBundle *bundle);

// install.c - 설치 엔진
gboolean do_install_bundle_simplified(const gchar *bundle_path,
                                     progress_callback_t progress_cb,
                                     void *user_data);

// slot.c - 슬롯 관리
GHashTable* get_system_slots(void);
RaucSlot* get_booted_slot(void);

// config_file.c - 설정 로드
gboolean load_config_file_simplified(const gchar *filename);
```

### Phase 4: C++ 래퍼 구현
**목표:** 포팅된 C 함수들을 C++ 인터페이스로 래핑

#### 4.1 RaucEngine 클래스 구현
```cpp
class RaucEngine {
private:
    // RAUC 전역 상태 관리
    bool initialized_;
    std::string config_path_;
    std::string last_error_;

    // 콜백 저장
    ProgressCallback progress_callback_;
    CompletedCallback completed_callback_;

public:
    bool initialize(const std::string& config_path);
    bool installBundle(const std::string& bundle_path);
    std::vector<SlotInfo> getSlotStatus();
    std::string getBootSlot();
    // ... 기타 메서드들
};
```

#### 4.2 UpdateClient 클래스 구현
```cpp
class UpdateClient {
private:
    std::unique_ptr<RaucEngine> engine_;

public:
    // 단순화된 공개 API
    bool initialize(const std::string& config_path = "");
    bool install(const std::string& bundle_path);
    std::vector<SlotInfo> getSlotStatus();
    // ... 콜백 설정 메서드들
};
```

### Phase 5: 통합 및 테스트
**목표:** update-service와의 연동 및 기능 검증

#### 5.1 update-service 수정
```cpp
// update_service.cpp에서
#include <update-library/update_client.h>

class UpdateService {
private:
    std::unique_ptr<UpdateLibrary::UpdateClient> update_client_;

public:
    bool initialize() {
        update_client_ = std::make_unique<UpdateLibrary::UpdateClient>();
        return update_client_->initialize();
    }

    DBusMessage* handleInstall(DBusMessage* message) {
        // D-Bus 파라미터 파싱
        const char* bundle_path = ...;

        // 라이브러리 호출
        bool success = update_client_->install(bundle_path);

        // D-Bus 응답 생성
        return create_dbus_reply(success);
    }
};
```

#### 5.2 테스트 시나리오
1. **기본 설치 테스트**: 유효한 번들로 정상 설치
2. **상태 조회 테스트**: 설치 전후 슬롯 상태 비교
3. **오류 처리 테스트**: 잘못된 번들, 권한 부족 등
4. **성능 테스트**: 기존 RAUC D-Bus 대비 성능 측정

## 3. 구체적 구현 작업

### 3.1 즉시 시작할 작업들

#### A. utils.h/utils.c 포팅
- 파일 I/O 유틸리티
- 문자열 처리 함수
- 디렉토리 관리 함수

#### B. context.h/context.c 포팅
- 전역 RAUC 컨텍스트 관리
- 설정 저장소
- 초기화/정리 함수

#### C. config_file.h/config_file.c 포팅
- system.conf 파싱
- 슬롯 정의 로드
- 설정 검증

### 3.2 데이터 흐름 설계

```
[update-service D-Bus 요청]
    ↓
[UpdateClient::install()]
    ↓
[RaucEngine::installBundle()]
    ↓
[check_bundle_simplified()] → 번들 검증
    ↓
[do_install_bundle_simplified()] → 실제 설치
    ↓ (진행률 콜백)
[progress_callback()] → update-service → D-Bus 신호
    ↓
[completed_callback()] → update-service → D-Bus 신호
```

### 3.3 오류 처리 전략

#### 기존 RAUC 방식 (GError)
```c
gboolean check_bundle(const gchar *bundlename, RaucBundle **bundle,
                      CheckBundleParams params, RaucBundleAccessArgs *access_args,
                      GError **error);
```

#### 라이브러리 방식 (단순화)
```cpp
class RaucEngine {
    std::string last_error_;
public:
    bool checkBundle(const std::string& bundle_path, RaucBundle** bundle) {
        GError* error = nullptr;
        gboolean result = check_bundle_simplified(bundle_path.c_str(), bundle, &error);
        if (error) {
            last_error_ = error->message;
            g_error_free(error);
        }
        return result == TRUE;
    }

    std::string getLastError() const { return last_error_; }
};
```

## 4. 예상 문제점 및 해결방안

### 4.1 의존성 문제
**문제:** RAUC 코드의 복잡한 의존성 체인
**해결:** 단계적 포팅으로 의존성을 점진적으로 해결

### 4.2 메모리 관리
**문제:** C와 C++ 간 메모리 관리 충돌
**해결:** RAII 패턴과 스마트 포인터 적극 활용

### 4.3 스레드 안전성
**문제:** RAUC의 설치 과정은 비동기로 실행됨
**해결:** 적절한 뮤텍스와 콜백 시스템으로 동기화

## 5. 검증 방법

### 5.1 단위 테스트
- 각 포팅된 함수별 개별 테스트
- Mock 데이터를 이용한 번들 검증 테스트
- 슬롯 상태 파싱 정확성 검증

### 5.2 통합 테스트
```bash
# 테스트 시나리오
1. 라이브러리 초기화 테스트
2. 기존 RAUC 번들로 설치 테스트
3. 설치 후 슬롯 상태 검증
4. update-service 연동 테스트
5. 성능 비교 테스트 (D-Bus vs Library)
```

### 5.3 호환성 검증
- 기존 시스템 설정 파일 호환성
- 기존 RAUC 번들 포맷 호환성
- update-service API 호환성

## 6. 다음 단계

구현 순서:
1. ✅ **기반 구조** (완료)
2. 🔄 **RAUC 헤더 포팅** (진행 중)
3. ⏳ **C 코드 포팅**
4. ⏳ **C++ 래퍼 구현**
5. ⏳ **통합 및 테스트**

각 단계는 독립적으로 테스트 가능하도록 설계하여 점진적 검증이 가능합니다.
