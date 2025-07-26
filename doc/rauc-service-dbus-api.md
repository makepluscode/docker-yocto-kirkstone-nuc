# RAUC Installer D-Bus API

이 문서는 RAUC 데몬이 시스템 버스(systemd-DBus)로 노출하는
`de.pengutronix.rauc.Installer` 인터페이스를 간단히 정리한 것입니다.

| 항목 | 값 |
|------|----|
| 버스 이름  | `de.pengutronix.rauc.Installer` |
| 오브젝트 경로 | `/de/pengutronix/rauc/Installer` |
| XML 스펙 위치 | `/usr/share/dbus-1/interfaces/de.pengutronix.rauc.Installer.xml` |

---
## 1. 메서드(Method)

| 메서드 | IN (형식) | OUT (형식) | 설명 |
|---------|-----------|------------|------|
| **Install** | `source` (s) | – | 번들 파일(.raucb) 경로를 지정하여 설치 시작 |
| **InstallBundle** | `source`(s) `args`(a{sv}) | – | URL 지원 + 추가 옵션 전달 |
| **Info** | `bundle`(s) | `compatible`(s) `version`(s) | 번들 메타데이터 조회 |
| **InspectBundle** | `source`(s) `args`(a{sv}) | `info`(a{sv}) | 번들 세부 정보(variant map) |
| **Mark** | `state`(s) `slot_identifier`(s) | `slot_name`(s) `message`(s) | 슬롯 상태 변경<br/>state=`good\|bad\|active` |
| **GetSlotStatus** | – | `slot_status_array`(a(sa{sv})) | 모든 슬롯 상태 배열 |
| **GetArtifactStatus** | – | `artifacts`(aa{sv}) | artifact/repository 상태 |
| **GetPrimary** | – | `primary`(s) | 부트로더가 인식한 primary 슬롯 |

---
## 2. 프로퍼티(Property)

| 이름 | 형식 | 설명 |
|------|------|------|
| Operation | `s` | 현재 동작: `idle`, `installing`, … |
| LastError | `s` | 마지막 오류 메시지 |
| Progress | `(isi)` | (percent:int, message:string, depth:int) |
| Compatible | `s` | 시스템 compatible 문자열 |
| Variant | `s` | 시스템 variant |
| BootSlot | `s` | 현재 부팅한 슬롯 이름 |

---
## 3. 시그널(Signal)

| 시그널 | 파라미터 | 의미 |
|---------|----------|------|
| Completed | `result`(i) | 설치 완료 후 결과 코드(0=성공) |

---
## 4. 실전 사용 예시

### 4-1. 슬롯 상태 조회
```bash
busctl call --system \
    de.pengutronix.rauc.Installer \
    /de/pengutronix/rauc/Installer \
    de.pengutronix.rauc.Installer GetSlotStatus
```

### 4-2. 번들 설치
```bash
busctl call --system \
    de.pengutronix.rauc.Installer \
    /de/pengutronix/rauc/Installer \
    de.pengutronix.rauc.Installer Install \
    s "/data/update_1.0.raucb"
```

### 4-3. 진행률 모니터링
```bash
watch -n1 'busctl get-property --system \
  de.pengutronix.rauc.Installer \
  /de/pengutronix/rauc/Installer \
  de.pengutronix.rauc.Installer Progress'
```

---
## 5. XML 보기 (런타임)
```bash
busctl introspect --system \
  de.pengutronix.rauc.Installer \
  /de/pengutronix/rauc/Installer --xml | less
```

---
## 6. 참고
- RAUC 공식 문서: https://rauc.readthedocs.io
- DBus 인트로스펙션 스펙: https://dbus.freedesktop.org/doc/dbus-specification.html 