# GQ5012BF1 vibrator HAL — C51C, deployment CLOSED

Owner: `LieppOS/android_hardware_ulefone_gq5012bf1`. Continues C51B
`88aac5431cdb2e2599411658d6e71dc233ab2bb6`. Apache-2.0 independent source;
no copied/generated Binder class or proprietary userspace backend.

## Exact current contract

| Item | Value |
|---|---|
| Soong module | `android.hardware.vibrator-service.gq5012bf1` |
| Backend | `libmtk_regulator_vibrator`, canonical MediaTek hardware owner |
| Shared dependencies | `android.hardware.vibrator-V2-ndk`, `libbinder_ndk`, `libbase` |
| Interface/instance | frozen V2 `android.hardware.vibrator.IVibrator/default` |
| Executable | `/vendor/bin/hw/android.hardware.vibrator-service.gq5012bf1` |
| Init name | `vendor.vibrator-default`, system UID/group, class hal |
| Source gates | init disabled+oneshot AND deploymentAllowed() unconditionally false before vibrator I/O/registration |
| Fragment | module-owned vibrator-gq5012bf1.xml; IVibrator V2 only |
| Executable type/domain | `hal_vibrator_default_exec` / `hal_vibrator_default` |
| Production backend | `discoverDriverTimedRegulatorLed`, driver timeout ABI1; no legacy/one-shot fallback |

C51C kernel implementation `af3ecd85d28614c2ad25c6448cd68634d62c6a33` adds stable
LED attributes `timeout_abi=1`, `max_duration_ms=5000`, atomic `duration_ms` and
`last_error`. A duration request is kernel-timed independently of HAL process
lifetime. Maximum is queried, not duplicated as a board guess. Oversized values
are illegal arguments before I/O. The 5000ms ceiling is provisional software
protection policy, not a proven actuator/thermal rating.

Startup still requires exact-driver discovery and accepted initial OFF before
ordinary AServiceManager registration. Capabilities remain zero; effects empty;
unsupported haptics/callbacks return explicit errors and callbacks are not retained.
The normal main thread joins the Binder pool with zero additional threads.
Controller serializes all actual requests. Genuine V2 signatures compile against
real SDK37-generated NDK headers; physical behavior and Binder IPC are not proven.

The kernel uses a CLOCK_BOOTTIME hrtimer plus mutex-serialized OFF work, drains
old LED-core work before rearm/suspend, latches/retries faults, and orders producer
teardown before state release. Persistent PMIC faults, another rail consumer,
kernel hangs and forced removal cannot be claimed physically safe by software.
Exact implementation/lifetime limits: MediaTek `vibrator/SAFETY.md` and kernel
`Documentation/ABI/testing/sysfs-class-led-regulator-vibrator`.

## Exact C54 integration handoff — no changes applied

**Keep stock selected and this package/fragment unselected.** Disabled init alone
cannot prevent explicit/interface starts; the code gate also refuses registration.
Do not flip it based only on host tests or object compilation.

1. Review/install the exact compatible C51C kernel module under separate owner
   approval; do not assume the running kernel has timeout ABI1. Existing kernel
   config is byte-identical C50: REGULATOR_VIBRATOR=m, ONESHOT unset, CFI and
   MODVERSIONS enabled. Do not apply the superseded generic-trigger config patch.
2. Establish the safe physical duration policy, dedicated controllable supply,
   fault/retry/suspend/remove behavior and real process-death tests separately.
   Returning a remove error does not veto Linux unbind; persistent failed OFF is
   a physical deployment blocker, not a successful stop.
3. Obtain sanitized actual LED/real-device path, labels and ownership. The kernel
   creates static attribute groups at LED registration, so there is no per-shot
   trigger-attribute recreation. Give system UID/hal_vibrator_default only
   directory/driver-link and max_brightness/timeout_abi/max_duration_ms/duration_ms/
   last_error reads, plus **duration_ms write only**. No trigger/invert/delay/shot
   or brightness write is required by production. No guessed genfs/ueventd rule,
   broad sysfs grant, root UID or DAC_OVERRIDE shortcut is provided.
4. The source executable mapping remains in `vibrator/sepolicy/file_contexts`;
   owner may include `hardware/ulefone/gq5012bf1/vibrator/sepolicy` in
   BOARD_VENDOR_SEPOLICY_DIRS after policy review. Compile enforcing policy and
   actual access against the real target; existing stock policy checks do not
   validate this replacement.
5. C52 reports the stock fragment declares BOTH IVibrator/default and
   IVibratorManager/default V2. This source implements ONLY IVibrator. Resolve
   Manager/actuator-ID/compatible-selection requirements before replacing the
   stock fragment. Do not invent IDs or leave two IVibrator servers installed.
6. Only after those gates: wire exact canonical source commits into the single
   approved Android build root, add PRODUCT_SOONG_NAMESPACES +=
   hardware/ulefone/gq5012bf1 (imports MediaTek) and PRODUCT_PACKAGES +=
   android.hardware.vibrator-service.gq5012bf1. Do not build a stale donor copy.
7. Exclude exactly the old stock binary, rc and fragment by marking their
   extraction entries EXTRACT_ONLY and regenerating vendor configuration:
   vendor/bin/hw/android.hardware.vibrator-service.mediatek;
   vendor/etc/init/vibrator-mtk-default.rc;
   vendor/etc/vintf/manifest/vibrator-mtk-default.xml. C52 proved late BoardConfig
   cannot mutate finalized PRODUCT_PACKAGES/COPY_FILES. No substring-wide removal.
8. Install the module-owned rc/fragment once, not via duplicate copy/manifest
   inclusion. Check actual assembled VINTF/SKU/APEX metadata, registration and
   IPC. Review an explicit code/init gate change only after all acceptance gates.

## Tests and build boundaries

HAL: **96/96 Python**, **34/34 native and ASan/UBSan**, **22/22 TSan**. Original
C51/C51B cases retained, five focused driver-ABI cases added. Host fake attributes
exercise actual request policy/controller, not counterfeit Binder classes.
Kernel shared policy: **12/12** each mode plus four static source-order checks;
these are not Linux timer/regulator runtime tests.

C51C real targeted kernel-module build/modpost/link PASS with Android Clang17
r487747c and unchanged C50 config. Scoped offline ABI checks: 32 import CRCs
match core, 13 common CRCs/six common callback tags match, no exports, same
vermagic. New .ko deliberately differs; no Image/full cohort or phone build.

C53 corrected the earlier Soong diagnosis: absolute OUT_DIR caused the
main.star error; **OUT_DIR=out works**, not a missing source to fabricate.
DeepSeek C54 is concurrently using the platform/vendor/policy environment, so
C51C does NOT run a concurrent shared-output Soong build or modify that tree.
Owner's targeted command, once source is wired and shared output is available:

```sh
export OUT_DIR=out
source build/envsetup.sh
lunch lineage_gq5012bf1-cp2a-userdebug
m -j2 android.hardware.vibrator-service.gq5012bf1
```

Independent read-only platform signature/object probe, from hal-audit:

```sh
PYTHONPATH=src PYTHONDONTWRITEBYTECODE=1 python3 -B tests/android_adapter_probe.py \
  /mnt/SamsungSSD/android/lineage-24.0
```

Recorded C51C result: **6/6 ELF64 AArch64 objects PASS**, not linked or Soong-built.
This uses real SDK37 aidl, frozen V2 SHA256
`51161792eba95aee28a92723a1430b235e57e75751c24db04d84f7d83cca318e`, hash
`ea8742d6993e1a82917da38b9938e537aa7fcb54`, and real Android Clang/Bionic/vendor
headers. Objects are not a linked Soong service or runtime proof. All temporary
outputs are removed. Source contracts, complete kernel lifetime evidence and
current commit receipts are in the existing **unmerged C51C** research handoff:
`tools/ulefone/gq5012bf1/hal-audit/docs/RESEARCH-HANDOFF.md`.

No C54 device/vendor/manifest/research edits, Android activation, security
weakening, push or phone operation. The entire hardware stack is not certified
open or physically safe by this source work.
