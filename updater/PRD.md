# 🚀 RAUC 업데이트 툴 (Qt6 QML + Python) – 제품 요구사항서

> 이 문서는 Yocto 기반 타깃 디바이스(예: Intel NUC)에서 RAUC를 통해 안전하게 소프트웨어 업데이트를 수행하기 위한 호스트 애플리케이션의 요구사항을 정의한다. 도구는 점진적 개발 방식으로 구현되며, 최종적으로 Qt 6의 QML UI와 Python 백엔드를 사용하여 데스크톱 환경에서 동작하는 Proof-of-Concept(PoC) 단계의 툴을 제공한다.

## 📋 목차

- [1. 목적 및 목표](#1-목적-및-목표)
- [2. 배경](#2-배경)
- [3. 개발 단계 계획](#3-개발-단계-계획)
- [4. 필수 기능 정의](#4-필수-기능-정의)
- [5. 기술 요구사항](#5-기술-요구사항)
- [6. 비기능 요구사항](#6-비기능-요구사항)
- [7. 구현 고려사항](#7-구현-고려사항)
- [8. 테스트 전략](#8-테스트-전략)
- [9. 배포 및 패키징](#9-배포-및-패키징)
- [10. 향후 발전 방향](#10-향후-발전-방향)

---

## 1. 목적 및 목표

### 🎯 목적
**호스트 PC에서 타깃 장치로 서명된 .raucb 번들을 전송하고 설치 과정을 시각적으로 모니터링할 수 있는 GUI 업데이트 툴을 개발한다.** 실제 제품 도입에 앞서 PoC 단계에서 RAUC 통합을 검증하는 것이 목표다.

### 📊 범위
**프로그램은 하나 이상의 타깃과 연결하여 업데이트를 수행할 수 있지만, 초기 버전에서는 단일 타깃을 우선 지원한다.** 보안 및 인증 기능은 개발용 키/인증서를 사용한 간단한 구현으로 제한한다.

---

## 2. 배경

**RAUC**는 임베디드 리눅스 장치용 신뢰성 높은 업데이트 프레임워크로, CLI와 D-Bus API를 제공한다. 설치 중 Progress 속성을 통해 진행률 정보를 제공하며([rauc.readthedocs.io](https://rauc.readthedocs.io)), rauc status 명령은 현재 시스템과 슬롯 상태를 다양한 형식으로 반환한다([rauc.readthedocs.io](https://rauc.readthedocs.io)). 

**Qt6 QML을 사용하면 모던한 GUI를 신속하게 구현할 수 있고, Python 백엔드는 SSH 전송과 D-Bus 통신을 손쉽게 처리할 수 있다.**

---

## 3. 개발 단계 계획

### 🔧 Step 1: Headless 업데이터 (기본 CLI 버전)

> **목표**: 명령줄 인터페이스를 통한 기본 RAUC 업데이트 기능 구현

#### ✨ 핵심 기능
- ✅ 대상 장치 연결 (SSH)
- ✅ RAUC 번들 파일 전송 (SCP)
- ✅ 원격 설치 실행 및 진행률 모니터링
- ✅ 기본 오류 처리 및 로깅
- ✅ 모듈화된 코드 구조로 GUI 확장 준비

#### 🛠️ 기술 스택
- **Python** 3.9+
- **uv** 패키지 매니저 사용
- **pyproject.toml** 기반 프로젝트 설정
- **paramiko** (SSH/SCP)
- **asyncio** (비동기 처리)

### 🖥️ Step 2: GUI 업데이터 (Qt6 QML 기본 버전)

> **목표**: Headless 업데이터의 기능을 Qt6 QML GUI로 확장

#### ✨ 핵심 기능
- ✅ Step 1의 모든 기능을 GUI로 제공
- ✅ 연결 상태 시각화
- ✅ 파일 선택 다이얼로그
- ✅ 전송 및 설치 진행률 표시
- ✅ 실시간 로그 출력

#### 🛠️ 기술 스택 추가
- **PySide6** (Qt6 Python 바인딩)
- **QML** (UI 구성)
- **Qt Quick Controls 2**

### 🚀 Step 3: GUI 업데이터 고도화 (권장 기능 포함)

> **목표**: 사용자 편의성과 고급 기능을 포함한 완전한 버전

#### ✨ 추가 기능
- ✅ 시스템 상태 조회 및 표시
- ✅ Good-mark 처리 지원
- ✅ 다중 장치 관리
- ✅ 번들 정보 표시
- ✅ 국제화 지원 (한/영)
- ✅ 고급 오류 처리 및 복구

---

## 4. 필수 기능 정의

### 🔧 4.1 Step 1: Headless 업데이터 필수 기능

#### 🌐 4.1.1 대상 장치 연결
| 기능 | 설명 |
|------|------|
| **SSH 연결** | IP 주소와 인증 정보로 타깃 장치 연결 |
| **연결 확인** | ping 및 SSH 프로토콜을 통한 연결 검증 |
| **연결 상태 모니터링** | 실시간 연결 상태 추적 |

#### 📦 4.1.2 RAUC 번들 전송
| 기능 | 설명 |
|------|------|
| **파일 전송** | SCP를 통한 .raucb 파일 업로드 |
| **대상 경로** | /tmp/<bundle>.raucb로 기본 설정 |
| **전송 진행률** | 바이트 단위 진행률 추적 |
| **전송 오류 처리** | 재시도 로직 포함 |

#### ⚡ 4.1.3 업데이트 실행
| 기능 | 설명 |
|------|------|
| **원격 설치** | SSH를 통한 `rauc install` 명령 실행 |
| **진행률 모니터링** | RAUC CLI 출력 파싱을 통한 진행률 추적 |
| **완료 알림** | 설치 성공/실패 상태 감지 |

#### ⚠️ 4.1.4 기본 오류 처리
| 기능 | 설명 |
|------|------|
| **네트워크 장애** | 연결 끊김 감지 및 처리 |
| **RAUC 오류** | 설치 오류 메시지 수집 및 출력 |
| **로깅** | 모든 작업 로그를 파일로 저장 |

### 🖥️ 4.2 Step 2: GUI 업데이터 추가 기능

#### 🎨 4.2.1 사용자 인터페이스
| 기능 | 설명 |
|------|------|
| **연결 설정 UI** | IP, 사용자명, 인증 방식 입력 |
| **파일 선택** | 네이티브 파일 다이얼로그 통합 |
| **진행률 표시** | 시각적 진행률 바 및 상태 메시지 |
| **로그 출력** | 실시간 로그 표시 창 |

#### 📊 4.2.2 상태 관리
| 기능 | 설명 |
|------|------|
| **연결 상태 표시** | 실시간 연결 상태 시각화 |
| **작업 상태** | 대기/전송/설치/완료 상태 표시 |
| **오류 알림** | 사용자 친화적 오류 메시지 다이얼로그 |

### 🚀 4.3 Step 3: 고도화 기능

#### 🔍 4.3.1 시스템 상태 확인
- **RAUC 상태 조회**: 슬롯 상태 및 버전 정보 표시
- **시스템 정보**: 타깃 디바이스 기본 정보 수집

#### ✅ 4.3.2 Good-mark 처리 지원
- **슬롯 마킹**: 설치 후 good 마크 설정 기능
- **롤백 방지**: 자동 롤백 방지 로직

#### 🌐 4.3.3 다중 장치 관리
- **장치 목록**: 여러 타깃 IP 관리
- **장치별 상태**: 각 장치의 개별 상태 추적

#### 💫 4.3.4 사용자 환경 향상
- **국제화**: 한국어/영어 UI 지원
- **번들 정보**: rauc info를 통한 번들 상세 정보 표시
- **테마 지원**: 라이트/다크 테마 선택

---

## 5. 기술 요구사항

### 🐍 5.1 Python 환경

| 요구사항 | 상세 |
|----------|------|
| **Python 버전** | 3.9 이상 |
| **패키지 매니저** | uv 필수 사용 |
| **프로젝트 설정** | pyproject.toml 기반 설정 |
| **가상환경** | uv를 통한 의존성 관리 |

### 📁 5.2 프로젝트 구조

```
📦 rauc-updater/
├── 📄 pyproject.toml          # 프로젝트 설정 및 의존성
├── 🔒 uv.lock                 # 잠금 파일
├── 📂 src/
│   └── 📂 rauc_updater/
│       ├── 📄 __init__.py
│       ├── 📂 core/           # Step 1: 핵심 로직
│       ├── 📂 gui/            # Step 2: GUI 컴포넌트
│       └── 📂 advanced/       # Step 3: 고급 기능
├── 📂 tests/
├── 📂 docs/
└── 📂 resources/              # QML 파일 및 리소스
```

### 📦 5.3 의존성 관리

#### Step 1 의존성
```toml
[project]
dependencies = [
    "paramiko>=3.0.0",    # SSH/SCP 통신
    "asyncio-ssh>=2.13.0", # 비동기 SSH
    "click>=8.0.0",       # CLI 프레임워크
    "rich>=13.0.0",       # 풍부한 CLI 출력
    "pydantic>=2.0.0"     # 데이터 검증
]
```

#### Step 2 추가 의존성
```toml
[project]
dependencies = [
    # Step 1 의존성 +
    "PySide6>=6.5.0",         # Qt6 Python 바인딩
    "PySide6-Addons>=6.5.0"   # Qt6 추가 모듈
]
```

#### Step 3 추가 의존성
```toml
[project]
dependencies = [
    # Step 1, 2 의존성 +
    "babel>=2.12.0",      # 국제화
    "watchdog>=3.0.0",    # 파일 시스템 모니터링
    "keyring>=24.0.0"     # 인증 정보 저장
]
```

---

## 6. 비기능 요구사항

### 💻 6.1 플랫폼 지원

| 플랫폼 | 버전 |
|--------|------|
| **운영체제** | Linux (Ubuntu 20.04+), Windows 10+ |
| **Python** | 3.9 이상 |
| **Qt** | 6.5 이상 |

### ⚡ 6.2 성능 요구사항

- ✅ **파일 전송**: 별도 스레드에서 비동기 처리
- ✅ **UI 응답성**: GUI는 항상 반응 가능한 상태 유지
- ✅ **메모리 사용량**: 최대 100MB 이하

### 🔐 6.3 보안 요구사항

- ✅ **SSH 인증**: 암호 및 키 기반 인증 지원
- ✅ **번들 검증**: 선택적 서명 검증 기능
- ✅ **민감 정보**: 메모리에서 인증 정보 안전한 처리

### 🔧 6.4 확장성 요구사항

- ✅ **모듈화**: 각 단계별 독립적 모듈 구조
- ✅ **플러그인**: 향후 플러그인 아키텍처 지원 준비
- ✅ **설정 관리**: JSON/YAML 기반 외부 설정 파일

---

## 7. 구현 고려사항

### 📦 7.1 uv 사용 가이드라인

#### 프로젝트 초기화
```bash
# 새 프로젝트 생성
uv init rauc-updater
cd rauc-updater

# 의존성 추가 (Step별로)
uv add paramiko asyncio-ssh click rich pydantic

# 개발 의존성 추가
uv add --dev pytest black isort mypy
```

#### pyproject.toml 설정 예시
```toml
[project]
name = "rauc-updater"
version = "0.1.0"
description = "RAUC Update Tool with Qt6 QML GUI"
authors = [
    {name = "Your Name", email = "your.email@example.com"}
]
readme = "README.md"
license = {text = "MIT"}
requires-python = ">=3.9"
keywords = ["rauc", "ota", "embedded", "update"]
classifiers = [
    "Development Status :: 3 - Alpha",
    "Intended Audience :: Developers",
    "License :: OSI Approved :: MIT License",
    "Programming Language :: Python :: 3.9",
    "Programming Language :: Python :: 3.10",
    "Programming Language :: Python :: 3.11",
]

dependencies = [
    "paramiko>=3.0.0",
    "click>=8.0.0",
    "rich>=13.0.0",
    "pydantic>=2.0.0",
]

[project.optional-dependencies]
gui = [
    "PySide6>=6.5.0",
    "PySide6-Addons>=6.5.0",
]
advanced = [
    "babel>=2.12.0",
    "watchdog>=3.0.0",
    "keyring>=24.0.0",
]
dev = [
    "pytest>=7.0.0",
    "black>=23.0.0",
    "isort>=5.0.0",
    "mypy>=1.0.0",
    "pre-commit>=3.0.0",
]

[project.scripts]
rauc-updater = "rauc_updater.cli:main"
rauc-updater-gui = "rauc_updater.gui:main"

[build-system]
requires = ["hatchling"]
build-backend = "hatchling.build"

[tool.black]
line-length = 88
target-version = ['py39']

[tool.isort]
profile = "black"
line_length = 88

[tool.mypy]
python_version = "3.9"
strict = true
```

### 🎯 7.2 단계별 구현 전략

#### 🔧 Step 1 구현 방법
| 컴포넌트 | 구현 방안 |
|----------|-----------|
| **SSH 연결** | paramiko 라이브러리를 사용하여 SSH 클라이언트 구현 |
| **파일 전송** | SCP 프로토콜로 번들 파일 전송, 진행률 콜백 구현 |
| **RAUC 실행** | SSH 채널을 통해 `rauc install` 명령 실행 |
| **진행률 파싱** | RAUC CLI 출력을 정규식으로 파싱하여 진행률 추출 |
| **로깅** | rich 라이브러리를 사용한 컬러풀한 CLI 출력 |

#### 🖥️ Step 2 확장 방법
| 컴포넌트 | 구현 방안 |
|----------|-----------|
| **GUI 아키텍처** | QML과 Python 간 시그널/슬롯 통신 구조 |
| **파일 다이얼로그** | Qt.FileDialog를 QML에서 호출 |
| **진행률 바** | QML ProgressBar와 Python 백엔드 연동 |
| **비동기 처리** | QThread 또는 asyncio와 QML 통합 |
| **상태 관리** | QML의 State 및 Transition 활용 |

#### 🚀 Step 3 고도화 방법
| 컴포넌트 | 구현 방안 |
|----------|-----------|
| **D-Bus 통신** | dbus-next를 통한 RAUC D-Bus API 활용 |
| **다중 장치** | 장치별 독립적 상태 관리 및 UI 표시 |
| **국제화** | Qt Linguist 도구를 활용한 다국어 지원 |
| **테마 시스템** | QML의 Material 또는 Universal 스타일 |
| **설정 관리** | QSettings 또는 JSON 파일 기반 설정 저장 |

---

## 8. 테스트 전략

### 🧪 8.1 단위 테스트
- **pytest**를 사용한 핵심 로직 테스트
- **mock**을 통한 SSH 연결 및 파일 전송 테스트
- **asyncio** 기반 비동기 함수 테스트

### 🔄 8.2 통합 테스트
- 실제 RAUC 타깃과의 연결 테스트
- 전체 업데이트 플로우 엔드투엔드 테스트
- GUI 자동화 테스트 (Qt Test Framework)

### 👥 8.3 사용자 테스트
- PoC 환경에서의 실제 사용 시나리오 테스트
- 다양한 네트워크 환경에서의 안정성 테스트
- 에러 상황에서의 복구 테스트

---

## 9. 배포 및 패키징

### 🛠️ 9.1 개발 배포
```bash
# 개발 모드 설치
uv sync --dev

# Step별 실행
uv run rauc-updater --help              # Step 1
uv run rauc-updater-gui                 # Step 2+
```

### 📦 9.2 프로덕션 배포
```bash
# 빌드
uv build

# 설치
uv tool install rauc-updater

# 또는 pipx 호환
pipx install rauc-updater
```

### 📱 9.3 바이너리 패키징
- **PyInstaller** 또는 **cx_Freeze**를 통한 실행 파일 생성
- **Linux**: AppImage 형태로 배포
- **Windows**: MSI 인스톨러 또는 포터블 실행 파일

---

## 10. 향후 발전 방향

### 🌟 10.1 추가 기능
- **OTA 서버 통합**: RAUC Hawkbit 브릿지 연동
- **보안 강화**: 키링 관리 및 TLS 전송
- **자동 롤백**: 설치 실패 시 자동 복구
- **배치 업데이트**: 다중 장치 동시 업데이트

### 🏗️ 10.2 아키텍처 개선
- **플러그인 시스템**: 확장 가능한 모듈 구조
- **웹 인터페이스**: 브라우저 기반 관리 UI
- **REST API**: 외부 시스템과의 통합
- **컨테이너화**: Docker 기반 배포 지원

### 💫 10.3 사용성 개선
- **자동 검색**: 네트워크 내 RAUC 장치 자동 발견
- **원격 진단**: 장치 상태 원격 모니터링
- **업데이트 스케줄링**: 예약된 업데이트 실행
- **알림 시스템**: 이메일/슬랙 등 외부 알림 연동

---

## 📚 참고 자료

- [RAUC Documentation](https://rauc.readthedocs.io)
- [PySide6 Documentation](https://doc.qt.io/qtforpython/)
- [uv Documentation](https://docs.astral.sh/uv/)
- [Qt6 QML Documentation](https://doc.qt.io/qt-6/qml-tutorial.html)

---

> **💡 참고**: 이 PRD는 점진적 개발을 통해 안정적이고 확장 가능한 RAUC 업데이트 툴 개발을 목표로 합니다. 각 단계별 구현을 통해 리스크를 최소화하고 사용자 피드백을 빠르게 반영할 수 있습니다.