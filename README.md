# GQ5012BF1 vibrator HAL — C51D, deployment CLOSED

Owner: `LieppOS/android_hardware_ulefone_gq5012bf1`. Continues C51C
`72617cde4e647ac908539952fcd7c4f821d94d04`. Apache-2.0 independent source;
no copied/generated Binder class or proprietary userspace backend.

## Exact current contract

| Item | Value |
|---|---|
| Soong module | `android.hardware.vibrator-service.gq5012bf1` (`cc_binary`, `vendor: true`, namespace `hardware/ulefone/gq5012bf1` importing `hardware/mediatek/mt6878`) |
| Backend | `libmtk_regulator_vibrator` (static), canonical MediaTek hardware owner |
| Shared dependencies | `android.hardware.vibrator-V2-ndk`, `libbinder_ndk`, `libbase` |
| Interface/instance | frozen V2 (hash `ea8742d6993e1a82917da38b9938e537aa7fcb54`) `android.hardware.vibrator.IVibrator/default` ONLY |
| Executable | `/vendor/bin/hw/android.hardware.vibrator-service.gq5012bf1` |
| Init | `vendor.vibrator-default`, class hal, user/group system, `interface aidl ...IVibrator/default`; currently disabled+oneshot |
| Source gates | init disabled+oneshot AND `deploymentAllowed()` unconditionally false before any vibrator I/O or registration |
| Fragment | module-owned `vibrator-gq5012bf1.xml`: IVibrator V2 only, no IVibratorManager |
| Executable type/domain | `hal_vibrator_default_exec` / `hal_vibrator_default` (platform-defined) |
| Service context | platform `service_contexts`: IVibrator/default -> `hal_vibrator_service` (no new entry) |
| Production node | `openDriverTimedRegulatorLed("/sys/devices/platform/regulator-vibrator", "vibrator")`, timeout ABI1, no fallback |

Startup: code gate -> canonical device open (driver link must be
`regulator_vibrator`, `leds/vibrator` not a symlink) -> ABI1 metadata ->
accepted initial OFF -> `AServiceManager_addService` -> main thread joins the
Binder pool with zero extra threads. Capabilities 0; effects empty;
unsupported operations return EX_UNSUPPORTED_OPERATION; a non-null `on()`
callback is refused, never retained. Controller serializes all requests.

## C51D findings

**IVibratorManager (actual stock evidence).** The stock vendor fragment
`vibrator-mtk-default.xml` declares BOTH `IVibrator/default` and
`IVibratorManager/default` V2. One binary,
`android.hardware.vibrator-service.mediatek` (SHA256
`ab4fc6285875b3f6ce70d1a7f15b10465d149fbb34a6c3bd8caa8d2d95b6573a`, MediaTek
`libvibrator/aidl/default` Vibrator.cpp + VibratorManager.cpp), serves both, via
`/sys/class/leds/vibrator` activate/duration/state (AOSP init.rc selects the
LED transient trigger) or legacy timed_output. Live stock dumpsys: framework uses
the Manager path, `vibratorIds=[1]`, Manager `capabilitiesFlags=11111111` (all
eight SYNC/PREPARE/MIXED_TRIGGER/TRIGGER_CALLBACK bits) for one DC-driven ERM,
per-vibrator capabilities 0, five prebaked effects.

- The stock Manager returns its own in-process vibrator; it cannot front this
  service. Both rc files define `vendor.vibrator-default`. **Coexistence is
  impossible; replacement is all-or-nothing** (binary, rc, fragment).
- Android 17 framework (Java `VintfHalVibratorManager` and native
  `VibratorManagerHalController`) falls back to `LegacyHalVibratorManager` /
  `LegacyManagerHalWrapper` when IVibratorManager/default is NOT declared:
  manager capabilities NONE, one vibrator (id 0) backed by IVibrator/default.
  The compatibility matrices (202604/202704, `1-4`) list both interfaces; V2 is
  in range. **A Manager implementation is not required.**
- A truthful single-actuator Manager could only report capabilities 0 and one
  ID while every sync call is unsupported, identical to the framework fallback.
  None is implemented; no multi-vibrator/sync support is fabricated.
- Visible differences vs stock after replacement: vibrator id 1 -> 0; prebaked
  effects use framework fallback waveforms through `on()`; **framework `on()`
  requests above the kernel max (5000ms) get EX_ILLEGAL_ARGUMENT and the
  framework cancels that vibration** (single one-shots >5s, all-on repeating
  loops longer than 5s). Shorter steps and repeating loops <=5s
  (`REPEATING_EFFECT_ON_DURATION=5000`) work. Keeping the ceiling is required
  until hardware validation; a later policy decision may allow longer requests.
- Retaining the complete stock service is the correct state for C55.

**Defect fixed: LED-class enumeration.** C51C production discovery opened every
`/sys/class/leds` entry and failed closed on the first unreadable one. Enforcing
policy does not let `hal_vibrator_default` open backlight/RGB LED directories
(`sysfs_leds` and vendor types), so the service could never start without broad
LED access. C51D opens the canonical device directly (O_PATH). Evidence: live DT
`/regulator-vibrator` label `vibrator`, live sysfs
`/sys/devices/platform/regulator-vibrator/{driver,leds/vibrator}`, stock genfscon.

**Binder review.** 24 frozen V2 methods, overrides compile against genuine
generated headers. Without CAP_ON_CALLBACK the framework calls `on(ms, null)` and
schedules completion itself, so no callback is owed. After a failed `on()`, the
framework's cancel step calls `off()`, which clears the HAL fault latch. Static
review (NOT a VTS run) of Android 17 VtsHalVibratorTargetTest for capability 0:
UNSUPPORTED for callbacks, effects, amplitude, external control, compose, PWLE,
always-on and frequency matches source; V3/V4-only methods reach this V2 server
as STATUS_UNKNOWN_TRANSACTION, which those checks accept.

**Kernel ABI agreement** (kernel `af3ecd85d28614c2ad25c6448cd68634d62c6a33`, not
modified): `timeout_abi` "1\n"; `max_duration_ms` "%u\n" 5000; `duration_ms` store
`kstrtouint` base 10, 1..max arms, 0 cancels/acknowledges, >max -EINVAL without
replacing; read = remaining ms rounded up (<= max). HAL writes "N\n"/"0\n" only,
validates 1..max before I/O. Store detaches any trigger (incl. AOSP-selected
transient) and drains the old LED-core OFF work before arming. No kernel defect
found in C51D.

## Exact C55/device-owner handoff — nothing applied

**Keep the stock vibrator selected and this package unselected.** Not for C55.

1. Kernel: install the exact C51C `regulator-vibrator.ko` only under separate
   approval; the stock vendor HAL does not use timeout ABI1. With the C51C module,
   legacy brightness requests are also capped at 5000ms.
2. DAC (missing today): device `ueventd.rc`
   `/sys/devices/platform/regulator-vibrator/leds/vibrator duration_ms 0664 system system`.
   Metadata attributes are 0444 and need no change.
3. SELinux: no new allow rule. Required: stock genfscon
   `/devices/platform/regulator-vibrator/leds/vibrator` -> `sysfs_vibrator` (present
   in stock vendor policy), platform `hal_vibrator` sysfs_vibrator file rw + dir
   search, platform domain sysfs dir search/lnk_file read. Add the executable
   mapping from `vibrator/sepolicy/file_contexts` (BOARD_VENDOR_SEPOLICY_DIRS +=
   `hardware/ulefone/gq5012bf1/vibrator/sepolicy`). The prebuilt stock vendor
   policy does not label this binary, so vendor policy must be rebuilt.
   Enforcing only; verify zero vibrator denials on hardware.
4. Exclude exactly the three stock entries (EXTRACT_ONLY + regenerate):
   `vendor/bin/hw/android.hardware.vibrator-service.mediatek`,
   `vendor/etc/init/vibrator-mtk-default.rc`,
   `vendor/etc/vintf/manifest/vibrator-mtk-default.xml`. No substring removal.
   Stock `vibrator.default.so`, `libvibratorutils.so` and stock
   `android.hardware.vibrator-V2-ndk.so` are already EXTRACT_ONLY; the module's
   own V2 NDK library comes from Soong.
5. Wire these exact repositories once into the single build root as
   `hardware/mediatek/mt6878` and `hardware/ulefone/gq5012bf1`; add
   `PRODUCT_SOONG_NAMESPACES += hardware/ulefone/gq5012bf1 hardware/mediatek/mt6878`
   and `PRODUCT_PACKAGES += android.hardware.vibrator-service.gq5012bf1` only
   with the exclusion above. Module rc/fragment install once.
6. Check assembled VINTF (`checkvintf`), that IVibratorManager/default is no longer
   declared, registration, IPC and VTS `VtsHalVibratorTargetTest`.
7. Hardware gates, separately authorized: duration rating, regulator fault/
   suspend/remove/process-death behavior. Only then review an explicit code/init
   gate change (`deploymentAllowed()`, `disabled`, `oneshot`).

## Tests and build boundaries

HAL native **36/36** (12 + 24) normal and ASan/UBSan, **24/24 TSan**; Python
**100/100** including four C51D contract checks. Host fakes exercise production
policy/controller/open code, not Binder IPC. Kernel shared policy 12/12 per mode.
Genuine SDK37 AIDL + Clang22/Bionic: **6/6 ARM64 objects**; NOT linked.

**Soong link: DEFERRED.** DeepSeek C55 owns the active shared platform build
(`m -j32 systemimage systemextimage productimage`); the platform tree does not
contain these repositories. Pending exclusive-window procedure (relative OUT_DIR,
reversible, no product change):

```sh
cd /mnt/SamsungSSD/android/lineage-24.0
mkdir hardware/mediatek/mt6878 hardware/ulefone/gq5012bf1   # must not pre-exist
git -C ~/LieppOS/android/hardware/mediatek/mt6878 archive <MTK_SHA> | tar -x -C hardware/mediatek/mt6878
git -C ~/LieppOS/android/hardware/ulefone/gq5012bf1 archive <BOARD_SHA> | tar -x -C hardware/ulefone/gq5012bf1
export OUT_DIR=out; source build/envsetup.sh; lunch lineage_gq5012bf1-cp2a-userdebug
m -j8 out/soong/.intermediates/hardware/ulefone/gq5012bf1/android.hardware.vibrator-service.gq5012bf1/android_vendor_arm64_armv8-a/android.hardware.vibrator-service.gq5012bf1
rm -r hardware/mediatek/mt6878 hardware/ulefone/gq5012bf1; rmdir hardware/mediatek hardware/ulefone
```

The namespace is not exported to Make, so the intermediate path is the target.
Adding/removing Android.bp files triggers Soong re-analysis of the shared output.

No C55 device/vendor/manifest/research edits, Android activation, security
weakening, push or phone operation. Physical safety is not certified.
