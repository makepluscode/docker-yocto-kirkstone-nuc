# RAUC Hawkbit C++ 업데이터 구현 가이드

## 개요

RAUC Hawkbit C++ 업데이터는 Eclipse Hawkbit와 RAUC(Robust Auto-Update Controller)를 통합한 완전한 OTA(Over-The-Air) 업데이트 클라이언트입니다. 이 시스템은 자동 폴링, 다운로드, 시스템 업데이트 설치를 제공합니다.

## 아키텍처

### 핵심 구성 요소

#### 1. HawkbitClient (`hawkbit_client.h/cpp`)
- **기능**: Hawkbit 서버와의 HTTP 통신
- **주요 역할**:
  - Hawkbit 서버 폴링
  - JSON 응답 파싱
  - 번들 다운로드 관리
  - 피드백 보고

#### 2. RaucClient (`rauc_client.h/cpp`)
- **기능**: RAUC 서비스와의 DBus 통신
- **주요 역할**:
  - 번들 설치 관리
  - 진행률 모니터링
  - 시그널 처리

#### 3. Main Application (`main.cpp`)
- **기능**: 메인 애플리케이션 로직
- **주요 역할**:
  - 메인 폴링 루프
  - 업데이트 오케스트레이션
  - 시그널 처리
  - DLT 로깅

#### 4. Configuration (`config.h`)
- **기능**: 중앙화된 설정 관리
- **주요 역할**:
  - 서버 URL 및 설정
  - 타이밍 파라미터
  - 네트워크 설정

## 구현된 기능

### ✅ 완료된 기능

#### 1. 자동 폴링 시스템
```cpp
// 설정 가능한 폴링 간격 (기본값: 30초)
const int POLL_INTERVAL_SECONDS = 30;
```
- Hawkbit 서버로부터 주기적으로 업데이트 확인
- HTTP 통신을 통한 서버 연결
- JSON 응답 파싱 및 업데이트 감지

#### 2. 다운로드 관리
```cpp
// 다운로드 타임아웃 설정 (5분)
const int DOWNLOAD_TIMEOUT_SECONDS = 300;
```
- 자동 번들 다운로드
- 진행률 추적
- 오류 처리 및 복구
- 설정 가능한 타임아웃

#### 3. RAUC 통합
- DBus를 통한 RAUC 서비스 통신
- 번들 설치 및 관리
- 진행률 모니터링
- 상태 추적

#### 4. 피드백 시스템
- **시작 피드백**: 업데이트 시작 알림
- **진행 피드백**: 설치 진행률 보고
- **완료 피드백**: 성공/실패 결과 보고
- **상세 오류 보고**: 오류 상황 상세 정보

#### 5. 포괄적인 로깅
```cpp
// DLT 로깅 컨텍스트
const std::string DLT_HAWK_CONTEXT = "HAWK";  // Hawkbit 클라이언트
const std::string DLT_RAUC_CONTEXT = "RAUC";  // RAUC 클라이언트
const std::string DLT_UPDATE_CONTEXT = "UPDT"; // 업데이트 프로세스
```
- DLT(Diagnostic Log and Trace) 통합
- 다중 로그 컨텍스트
- 설정 가능한 로그 레벨
- 디버그 정보 제공

#### 6. 오류 처리
- 네트워크 오류 복구
- 설치 실패 처리
- 타임아웃 관리
- 우아한 종료

#### 7. 설정 관리
- 중앙화된 설정 파일
- 쉬운 서버 URL 수정
- 설정 가능한 타임아웃 및 간격
- 디버그 설정

## 설정

### 서버 설정
```cpp
// Hawkbit 서버 설정
const std::string HAWKBIT_SERVER_URL = "http://192.168.1.101:8080";
const std::string HAWKBIT_TENANT = "DEFAULT";
const std::string HAWKBIT_CONTROLLER_ID = "nuc-device-001";
```

### 타이밍 설정
```cpp
// 타이밍 설정
const int POLL_INTERVAL_SECONDS = 30;           // 폴링 간격
const int DOWNLOAD_TIMEOUT_SECONDS = 300;       // 다운로드 타임아웃 (5분)
const int INSTALLATION_TIMEOUT_SECONDS = 600;   // 설치 타임아웃 (10분)
const int HTTP_TIMEOUT_SECONDS = 30;            // HTTP 타임아웃
```

### DLT 설정
```cpp
// DLT 로깅 설정
const std::string DLT_APP_NAME = "RHCP";
const std::string DLT_HAWK_CONTEXT = "HAWK";
const std::string DLT_RAUC_CONTEXT = "RAUC";
const std::string DLT_UPDATE_CONTEXT = "UPDT";
```

## 업데이트 프로세스 흐름

### 1. 초기화 단계
```cpp
// DLT 등록
DLT_REGISTER_APP(DLT_APP_NAME.c_str(), "RAUC Hawkbit C++ Client");
DLT_REGISTER_CONTEXT(hawkbitContext, DLT_HAWK_CONTEXT.c_str(), "Hawkbit client context");
DLT_REGISTER_CONTEXT(raucContext, DLT_RAUC_CONTEXT.c_str(), "RAUC client context");
DLT_REGISTER_CONTEXT(updateContext, DLT_UPDATE_CONTEXT.c_str(), "Update process context");

// Hawkbit 클라이언트 초기화
HawkbitClient hawkbitClient(HAWKBIT_SERVER_URL, HAWKBIT_TENANT, HAWKBIT_CONTROLLER_ID);

// RAUC 클라이언트 연결
RaucClient raucClient;
raucClient.connect();
```

### 2. 폴링 루프
```cpp
while (running) {
    // Hawkbit 서버 폴링
    if (hawkbitClient.pollForUpdates()) {
        // JSON 응답 파싱
        UpdateInfo update_info;
        if (hawkbitClient.parseUpdateResponse(response, update_info)) {
            // 업데이트 실행
            performUpdate(hawkbitClient, raucClient, update_info);
        }
    }
    
    // 다음 폴링까지 대기
    std::this_thread::sleep_for(std::chrono::seconds(POLL_INTERVAL_SECONDS));
}
```

### 3. 업데이트 실행 (업데이트 발견 시)
```cpp
bool performUpdate(HawkbitClient& hawkbitClient, RaucClient& raucClient, const UpdateInfo& update_info) {
    // 1단계: 시작 피드백 전송
    hawkbitClient.sendStartedFeedback(update_info.execution_id);
    
    // 2단계: 번들 다운로드
    std::string bundle_path = BUNDLE_DOWNLOAD_PATH;
    hawkbitClient.downloadBundle(update_info.download_url, bundle_path);
    
    // 3단계: 다운로드 완료 피드백
    hawkbitClient.sendProgressFeedback(update_info.execution_id, 50, "Bundle downloaded successfully");
    
    // 4단계: RAUC를 통한 번들 설치
    raucClient.installBundle(bundle_path);
    
    // 5단계: 설치 진행률 모니터링
    while (update_in_progress && timeout_counter < MAX_TIMEOUT) {
        raucClient.getStatus(status);
        if (status == "idle") {
            hawkbitClient.sendFinishedFeedback(update_info.execution_id, true, "Installation completed successfully");
            break;
        }
    }
    
    // 6단계: 정리
    remove(bundle_path.c_str());
    return true;
}
```

### 4. 정리
- 임시 파일 제거
- 업데이트 상태 초기화
- 폴링 계속

## DLT 로깅

### 로그 컨텍스트
- **HAWK**: Hawkbit 클라이언트 작업
- **RAUC**: RAUC 클라이언트 작업
- **UPDT**: 업데이트 프로세스 작업

### 로그 레벨
- **ERROR**: 오류 조건
- **WARN**: 경고 조건
- **INFO**: 일반 정보
- **DEBUG**: 상세 디버깅 정보

### 디버깅 스크립트
`dlt-receive.sh` 스크립트는 다음 기능을 제공합니다:
- 실시간 로그 모니터링
- 컨텍스트 필터링
- 로그 레벨 제어
- 로그 파일 저장
- 색상 코딩된 출력

## 빌드 통합

### Yocto 통합
- **레시피**: `kirkstone/meta-apps/recipes-apps/rauc-hawkbit-cpp/rauc-hawkbit-cpp_1.0.bb`
- **외부 소스 설정**: 개발 중인 코드 사용
- **의존성**: DLT, DBus, libcurl, json-c, RAUC

### 의존성
- **빌드**: dlt-daemon, cmake-native, pkgconfig-native, dbus, curl, json-c, rauc
- **런타임**: rauc, dbus, curl, json-c

## 서비스 통합

### Systemd 서비스
```ini
[Unit]
Description=RAUC Hawkbit C++ Client
After=network.target rauc.service
Wants=network.target rauc.service

[Service]
Type=simple
ExecStart=/usr/local/bin/rauc-hawkbit-cpp
Restart=always
RestartSec=5
User=root

[Install]
WantedBy=multi-user.target
```

### 서비스 관리
```bash
# 서비스 시작
systemctl start rauc-hawkbit-cpp

# 상태 확인
systemctl status rauc-hawkbit-cpp

# 로그 보기
journalctl -u rauc-hawkbit-cpp -f

# 서비스 중지
systemctl stop rauc-hawkbit-cpp
```

## 디버깅 및 모니터링

### DLT 로그 모니터링
```bash
# 모든 로그 실시간 추적
./dlt-receive.sh -f

# 컨텍스트별 필터링
./dlt-receive.sh -c HAWK -f

# 디버그 레벨 로깅
./dlt-receive.sh -l DEBUG -f

# 로그 파일 저장
./dlt-receive.sh -s hawkbit.log
```

### 수동 테스트
```bash
# Hawkbit 연결 테스트
curl http://192.168.1.101:8080/DEFAULT/controller/v1/nuc-device-001

# RAUC 상태 확인
rauc status

# DLT 데몬 확인
systemctl status dlt-daemon
```

## 현재 상태

### ✅ 완료된 작업
- 완전한 C++ 구현
- DLT 로깅 통합
- 설정 관리
- 오류 처리
- 서비스 통합
- 디버깅 도구

### 🔧 업데이트된 설정
- **서버 URL**: `http://192.168.1.101:8080`
- **폴링 간격**: 30초
- **타임아웃 설정**: 다운로드 5분, 설치 10분
- **디버그 설정**: 활성화

### 📋 테스트 준비 완료
- 애플리케이션 배포 준비 완료
- 올바른 서버로 설정 완료
- 디버깅을 위한 DLT 로깅 활성화
- 서비스 자동 시작 설정 완료

## 다음 단계

### 1. 배포 및 테스트
- 타겟 디바이스에 빌드 및 배포
- Hawkbit 서버 연결성 확인
- 엔드투엔드 업데이트 프로세스 테스트

### 2. 로그 모니터링
- DLT 로깅을 통한 디버깅
- 업데이트 프로세스 모니터링
- 피드백 보고 확인

### 3. 프로덕션 설정
- SSL 인증 활성화
- 프로덕션용 타임아웃 조정
- 적절한 오류 처리 구성

### 4. 향후 개선사항
- 환경 변수 설정 지원
- 설정 파일 지원
- 고급 오류 복구
- 다중 테넌트 지원

## 문제 해결

### 일반적인 문제

#### 1. 네트워크 연결 실패
```bash
# 네트워크 연결 확인
ping 192.168.1.101

# 포트 연결 확인
telnet 192.168.1.101 8080
```

#### 2. RAUC 서비스 문제
```bash
# RAUC 서비스 상태 확인
systemctl status rauc

# RAUC 로그 확인
journalctl -u rauc -f
```

#### 3. DLT 로깅 문제
```bash
# DLT 데몬 상태 확인
systemctl status dlt-daemon

# DLT 로그 확인
dlt-receive -a RHCP -c HAWK -c RAUC -c UPDT -v
```

### 로그 분석

#### Hawkbit 연결 로그
```
[HAWK] Polling for updates from: http://192.168.1.101:8080/DEFAULT/controller/v1/nuc-device-001
[HAWK] Poll response HTTP code: 200
[HAWK] Poll successful, response length: 1234
```

#### RAUC 설치 로그
```
[RAUC] Installing bundle: /tmp/hawkbit_update.raucb
[RAUC] Progress signal received: 50%
[RAUC] Installation completed successfully
```

#### 업데이트 프로세스 로그
```
[UPDT] Starting update process
[UPDT] Execution ID: 12345
[UPDT] Version: 1.2.3
[UPDT] Bundle downloaded successfully
[UPDT] Installation completed successfully
```

## 성능 최적화

### 메모리 사용량
- JSON 파싱 시 메모리 효율적 처리
- 임시 파일 자동 정리
- 적절한 타임아웃 설정

### 네트워크 효율성
- HTTP 연결 재사용
- 압축 전송 지원
- 적절한 폴링 간격 설정

### CPU 사용량
- 비동기 I/O 처리
- 효율적인 폴링 루프
- 최적화된 JSON 파싱

## 보안 고려사항

### 현재 설정
- SSL 인증 비활성화 (개발용)
- HTTP 기본 인증 미사용
- 로컬 네트워크 사용

### 프로덕션 권장사항
- HTTPS 사용
- SSL 인증서 검증
- 적절한 인증 메커니즘
- 방화벽 설정

## 결론

RAUC Hawkbit C++ 업데이터는 완전한 OTA 업데이트 솔루션을 제공합니다. 이 구현은 다음과 같은 특징을 가집니다:

- **완전한 자동화**: 수동 개입 없이 업데이트 처리
- **견고한 오류 처리**: 다양한 실패 상황에 대한 복구
- **포괄적인 로깅**: 상세한 디버깅 정보 제공
- **유연한 설정**: 다양한 환경에 맞는 설정 가능
- **확장 가능한 아키텍처**: 향후 기능 추가 용이

이 시스템을 통해 안정적이고 효율적인 OTA 업데이트 환경을 구축할 수 있습니다. 