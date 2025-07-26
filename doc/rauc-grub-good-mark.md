# RAUC + GRUB Good-Mark Flow

이 문서는 Intel NUC (x86-64 UEFI) 이미지를 기준으로 **RAUC** 와 **GRUB** 가 협력하여 ‘성공 부팅(good-mark)’ 를 기록·관리하는 과정을 설명합니다.

## 개요
RAUC 는 *A/B* 두 개의 루트파티션(`rootfs.0`, `rootfs.1`) 중 하나를 선택하여 부팅한 뒤, 부팅이 정상 완료되면 현재 슬롯을 *good* 으로 표시합니다. 이때 부트로더로 **GRUB** 를 사용하며, 슬롯 상태와 부트 순서는 ESP 가 아닌 **grubenv 파티션**(예: `/dev/sda1`)의 환경 변수로 관리합니다.

필수 구성 요소
- ESP(`/boot/EFI/BOOT/grub.cfg`) : 슬롯 로직과 `rauc.slot=` 커맨드라인을 포함한 GRUB config
- grubenv(`/grubenv/grubenv`) : `ORDER`, `A_OK`, `B_OK`, `A_TRY`, `B_TRY` 등 변수 저장소
- systemd 서비스 `rauc-mark-good.service` : 부팅 성공 시점에 `rauc mark-good` 실행

## 부팅 및 good-mark 시퀀스
아래 시퀀스 다이어그램은 정상 업데이트 후 최초 부팅 과정을 예시로 보여 줍니다.

```mermaid
autonumber
sequenceDiagram
    participant FW as UEFI/BIOS
    participant GRUB as GRUB EFI Loader
    participant ENV as grubenv (hd0,1)
    participant CFG as grub.cfg (ESP)
    participant KERN as Linux Kernel
    participant SYSTD as systemd
    participant RMG as rauc-mark-good.service
    participant RAUC

    FW->>GRUB: Load BOOTX64.EFI
    GRUB->>ENV: load_env grubenv
    GRUB->>CFG: Parse slot logic
    GRUB->>GRUB: Choose slot **A/B**
    GRUB->>KERN: Boot kernel + `rauc.slot=<X>`
    KERN->>SYSTD: Userspace hand-off
    SYSTD->>RMG: Start (condition: `rauc.slot` present)
    RMG->>RAUC: `rauc mark-good`
    RAUC->>ENV: `fw_setenv <X>_OK=1`, `<X>_TRY=0`
    RAUC-->>RMG: Success
    RMG-->>SYSTD: Exit 0
    note over RAUC,ENV: grubenv updated → 다음 부팅 시 <X> 슬롯이 *good* 으로 인식
```

## 상세 단계
1. **UEFI 펌웨어**가 ESP 의 `BOOTX64.EFI` 을 로드하면 **GRUB** 가 시작됩니다.
2. GRUB 는 `(hd0,1)/grubenv` 파일로부터 부트 변수(ORDER, A_OK…)를 읽습니다.
3. `grub.cfg` 안의 스크립트가
   - `ORDER` 순서대로 슬롯을 훑으며
   - `*_OK == 1` 이면서 `*_TRY == 0` 인 첫 슬롯을 기본 부팅 항목으로 선택합니다.
4. 선택된 메뉴 항목은 커널에 `rauc.slot=<A|B>` 커맨드라인을 넘겨 부팅합니다.
5. Linux userspace 로 넘어오면 **systemd** 가 `ConditionKernelCommandLine=|rauc.slot` 조건을 만족시켜 `rauc-mark-good.service` 를 실행합니다.
6. 서비스는 `rauc mark-good` 명령을 호출해 RAUC 데몬(DBus)에게 “현재 슬롯 부팅 성공” 메시지를 보냅니다.
7. RAUC 는 `fw_setenv`(libubootenv) 로 grubenv 안에 `<SLOT>_OK=1` 와 `<SLOT>_TRY=0` 를 기록하여 슬롯을 *good* 으로 확정합니다.
8. 이후 재부팅 시 GRUB 는 같은 슬롯을 *good* 으로 인식하고, 업데이트 실패 시에는 TRY 플래그를 보고 롤백 슬롯을 선택합니다.

## 문제 해결 체크리스트
- `/proc/cmdline` 에 `rauc.slot=` 파라미터가 없으면 **rauc-mark-good** 가 실행되지 않습니다.
- `grub-editenv /grubenv/grubenv list` 명령으로 `ORDER` 등 변수가 존재하는지 확인하세요.
- `rauc status` 가 *bad* 로 남아 있다면, grubenv 에서 `*_OK` 변수가 설정되지 않았거나 쓰기 실패한 것입니다.

## 참고 링크
- RAUC Documentation → Bootloader Integration → GRUB
- Yocto meta-rauc layer `classes/bundle.bbclass`
- U-Boot fw_setenv / libubootenv 도움말 