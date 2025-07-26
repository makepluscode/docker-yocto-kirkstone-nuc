# Qt Graphics Pipeline Guide

Intel NUC에서 Qt 애플리케이션의 그래픽 렌더링 파이프라인에 대한 상세 가이드입니다.

## 📋 목차

1. [그래픽 파이프라인 개요](#그래픽-파이프라인-개요)
2. [각 레이어 상세 설명](#각-레이어-상세-설명)
3. [플랫폼별 특징](#플랫폼별-특징)
4. [문제 진단 가이드](#문제-진단-가이드)
5. [성능 최적화](#성능-최적화)

## 🎨 그래픽 파이프라인 개요

Qt에서 GPU까지의 완전한 그래픽 파이프라인 흐름:

```
┌─────────────────────────────────┐
│     Qt Application Layer        │  ← QML/QtQuick Application
├─────────────────────────────────┤
│     Qt Scene Graph              │  ← OpenGL/Vulkan Rendering
├─────────────────────────────────┤
│  Qt Platform Abstraction (QPA) │  ← Cross-platform Layer
├─────────────────────────────────┤
│   Qt Platform Plugin Layer     │  ← eglfs/linuxfb/xcb/wayland
├─────────────────────────────────┤
│    Graphics API Layer          │  ← OpenGL ES/EGL/Mesa
├─────────────────────────────────┤
│   Kernel Graphics Stack        │  ← DRM/KMS/Graphics Driver
├─────────────────────────────────┤
│      Hardware Layer            │  ← GPU/Display Controller
└─────────────────────────────────┘
```

## 🔧 각 레이어 상세 설명

### 1. Qt Application Layer

**구성 요소:**
- QML Engine
- QtQuick Components
- Qt Scene Graph

**역할:**
- UI 요소를 Scene Graph 노드로 변환
- 애니메이션 및 상태 관리
- 이벤트 처리

**출력:**
- OpenGL/Vulkan 렌더링 명령
- 기하학적 데이터 (vertices, textures)

### 2. Qt Platform Abstraction (QPA)

**역할:**
- 플랫폼 독립적 인터페이스 제공
- 창 관리 추상화
- 입력 이벤트 통합

**주요 클래스:**
- `QPlatformIntegration`
- `QPlatformWindow`
- `QPlatformScreen`

### 3. Qt Platform Plugin Layer

#### EGLFS (Embedded Linux FrameBuffer)
```
Qt App → EGL → OpenGL ES → Mesa → DRM/KMS → GPU
```

**특징:**
- 직접 GPU 하드웨어 가속
- Fullscreen 전용
- 최고 성능
- 복잡한 설정

**설정 파일:**
```json
{
  "device": "/dev/dri/card0",
  "hwcursor": false,
  "pbuffers": true,
  "outputs": [
    {
      "name": "HDMI-A-1",
      "mode": "1920x1080@60"
    }
  ]
}
```

#### LinuxFB (Linux FrameBuffer)
```
Qt App → Linux FB → Kernel FB Driver → Display
```

**특징:**
- 소프트웨어 렌더링
- 설정 간단
- 안정성 높음
- 성능 제한적

#### XCB (X11)
```
Qt App → X11 → GPU Driver → Display
```

**특징:**
- 윈도우 시스템 지원
- 다중 애플리케이션
- 높은 메모리 사용량

#### Wayland
```
Qt App → Wayland → Compositor → GPU
```

**특징:**
- 현대적 디스플레이 서버
- 보안성 우수
- 낮은 지연시간

### 4. Graphics API Layer

#### Mesa 3D Graphics Library
- 오픈소스 OpenGL/Vulkan 구현
- 하드웨어 추상화 레이어
- 다양한 GPU 드라이버 지원

#### EGL (Embedded-System Graphics Library)
- OpenGL ES와 네이티브 플랫폼 연결
- 컨텍스트 관리
- 표면(Surface) 관리

### 5. Kernel Graphics Stack

#### DRM (Direct Rendering Manager)
- GPU 리소스 관리
- 메모리 관리
- 보안 및 동기화

#### KMS (Kernel Mode Setting)
- 디스플레이 모드 설정
- 해상도/주사율 제어
- 다중 디스플레이 지원

#### Graphics Driver (Intel i915)
- 하드웨어 특화 명령 변환
- GPU 메모리 관리
- 전력 관리

### 6. Hardware Layer

#### GPU Hardware
- 3D 렌더링 파이프라인
- 텍스처 샘플링
- 셰이더 실행

#### Display Controller
- 픽셀 데이터 출력
- 디스플레이 타이밍 제어
- 색상 공간 변환

## 🔄 데이터 흐름 예시

### EGLFS 경로 (하드웨어 가속)
```
QML Rectangle {
    color: "blue"
    width: 100
    height: 100
}
     ↓
Qt Scene Graph (Blue Rectangle Node)
     ↓
OpenGL glDrawElements() 명령
     ↓
Mesa → Intel i915 드라이버
     ↓
GPU 셰이더 실행
     ↓
프레임버퍼 → 디스플레이
```

### LinuxFB 경로 (소프트웨어 렌더링)
```
QML Rectangle
     ↓
Qt Scene Graph
     ↓
소프트웨어 래스터라이저
     ↓
CPU 픽셀 연산
     ↓
/dev/fb0 프레임버퍼
     ↓
디스플레이
```

## 🔍 플랫폼별 특징 비교

| 플랫폼 | 성능 | 안정성 | 설정 복잡도 | 메모리 사용량 | 사용 사례 |
|--------|------|--------|-------------|---------------|-----------|
| **EGLFS** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐ | 임베디드, 키오스크 |
| **LinuxFB** | ⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐ | ⭐⭐⭐ | 간단한 UI, 디버깅 |
| **XCB** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐⭐ | 데스크톱 환경 |
| **Wayland** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ | 현대적 임베디드 |

## 🚨 문제 진단 가이드

### 1. Qt Platform Plugin 문제

**증상:**
```
qt.qpa.plugin: Could not find the Qt platform plugin "eglfs"
Available platform plugins are: minimal, offscreen, vnc, xcb.
```

**원인:**
- qtbase PACKAGECONFIG에서 플랫폼 지원 누락

**해결:**
```bitbake
PACKAGECONFIG:pn-qtbase = "eglfs linuxfb kms gbm"
```

### 2. DRM/KMS 접근 문제

**증상:**
```
qt.qpa.eglfs.kms: Could not open DRM device
```

**확인:**
```bash
ls -la /dev/dri/
# 권한 확인
# Intel 드라이버 로드 확인
dmesg | grep i915
```

**해결:**
- 사용자를 video 그룹에 추가
- DRM 모듈 로드 확인

### 3. EGL 초기화 실패

**증상:**
```
qt.qpa.eglfs: Failed to initialize EGL display
```

**확인:**
```bash
# Mesa 설치 확인
find /usr -name "*libEGL*"
# GPU 정보 확인
lspci | grep VGA
```

### 4. 프레임버퍼 문제

**증상:**
```
qt.qpa.linuxfb: Failed to open framebuffer
```

**확인:**
```bash
ls -la /dev/fb*
cat /proc/fb
```

## ⚡ 성능 최적화

### EGLFS 최적화

#### 1. KMS 설정 최적화
```json
{
  "device": "/dev/dri/card0",
  "hwcursor": true,
  "pbuffers": true,
  "outputs": [
    {
      "name": "HDMI-A-1",
      "mode": "1920x1080@60",
      "format": "argb8888",
      "physicalWidth": 400,
      "physicalHeight": 300
    }
  ]
}
```

#### 2. 환경 변수 최적화
```bash
export QT_QPA_EGLFS_INTEGRATION=eglfs_kms
export QT_QPA_EGLFS_KMS_CONFIG=/etc/qt5/eglfs_kms_config.json
export QT_QPA_EGLFS_ALWAYS_SET_MODE=1
```

#### 3. Qt Scene Graph 최적화
```bash
export QSG_RENDER_LOOP=basic
export QSG_RHI_BACKEND=opengl
```

### LinuxFB 최적화

#### 1. 프레임버퍼 설정
```bash
# 색 깊이 설정
fbset -depth 32
# 해상도 설정
fbset -xres 1920 -yres 1080
```

#### 2. Qt 렌더링 최적화
```bash
export QT_QPA_FB_FORCE_FULLSCREEN=1
export QT_QPA_FB_DISABLE_INPUT=0
```

## 🛠️ 디버깅 도구

### Qt 디버깅
```bash
# 모든 Qt 로그 활성화
export QT_LOGGING_RULES="*=true"

# 특정 카테고리만
export QT_LOGGING_RULES="qt.qpa.*=true"
export QT_LOGGING_RULES="qt.qpa.eglfs.*=true"
```

### 시스템 디버깅
```bash
# DRM 정보
cat /sys/kernel/debug/dri/0/i915_display_info

# GPU 사용률
intel_gpu_top

# 프레임버퍼 정보
fbset -i
```

## 📚 참고 자료

- [Qt Platform Abstraction Documentation](https://doc.qt.io/qt-5/qpa.html)
- [Intel Graphics Driver Documentation](https://01.org/linuxgraphics/documentation)
- [Mesa 3D Documentation](https://docs.mesa3d.org/)
- [DRM/KMS Documentation](https://www.kernel.org/doc/html/latest/gpu/drm-kms.html)

---

*마지막 업데이트: 2025-07-26* 