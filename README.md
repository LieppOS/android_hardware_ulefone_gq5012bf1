# GQ5012BF1 vibrator HAL — C51B, deployment CLOSED

Owner: `LieppOS/android_hardware_ulefone_gq5012bf1`. Continues C51
`92f95b00c3d8a019b23f91279df1cbb39d242164`. Independently authored Apache-2.0
source (`LICENSE`); no proprietary userspace dependency or copied generated
Binder class. One source service, using the MediaTek native library, not a second
copy of its hardware backend.

## Exact source/build contract

| Item | Value |
|---|---|
| Soong module | `android.hardware.vibrator-service.gq5012bf1` |
| Static dependency | `libmtk_regulator_vibrator` in `hardware/mediatek/mt6878` |
| Shared dependencies | `android.hardware.vibrator-V2-ndk`, `libbinder_ndk`, `libbase` |
| Interface | frozen `android.hardware.vibrator.IVibrator` V2 |
| Instance | `android.hardware.vibrator.IVibrator/default` |
| Executable | `/vendor/bin/hw/android.hardware.vibrator-service.gq5012bf1` |
| Init name | `vendor.vibrator-default`, system UID/group, class hal |
| Init gate | `disabled` + `oneshot`; not lazy registration |
| Code gate | `deploymentAllowed()` is unconditionally false, before discovery, I/O or registration |
| Fragment | module-owned `vibrator/vibrator-gq5012bf1.xml`, V2 `IVibrator/default` only |
| SELinux executable/domain | `hal_vibrator_default_exec` / `hal_vibrator_default` |

Disabled alone would not stop explicit/interface starts. The code gate also
refuses those starts and prevents a crash-restart loop. There is no property or
command-line bypass. **Do not select/install the module or fragment while closed**:
framework clients may wait for a declared service which cannot register.

All 24 V2 methods are implemented against genuine generated types. Capabilities
are zero, effects empty; callbacks are rejected and never retained/invoked;
amplitude/effect/composition/always-on/frequency features remain explicitly
unsupported. Nonpositive duration and invalid amplitude are illegal arguments;
backend errno maps to a service-specific Binder exception. Host-tested production
request logic is in `VibratorPolicy.h`; the actual NDK transport is not mocked.
Startup requires timed-only discovery and accepted initial OFF before normal
`AServiceManager_addService`, then the main thread joins the Binder pool (max
additional threads 0). Actual Android registration/IPC is **not runtime-tested**.

## Authentic AIDL and build evidence

Source authority: `LineageOS/android_hardware_interfaces`, `lineage-24.0`, commit
`13fc23dae0dcd24fc2411d74f54a407999eaf555`. The interface Android.bp retains frozen
1/2/3/4 and NDK/vendor availability, SHA-256
`ce71a302b6d4f65e5b68b31162c0858a5149c92f8ed8aa34fa635f3cf984085a`.
Frozen V2 IVibrator SHA-256:
`51161792eba95aee28a92723a1430b235e57e75751c24db04d84f7d83cca318e`;
actual V2 interface hash `ea8742d6993e1a82917da38b9938e537aa7fcb54`.
The pinned target VTS source was consulted for cap-gated optional methods and
unsupported on-callback handling; VTS was **not run**.

C52 identifies the existing full platform at `/mnt/SamsungSSD/android/lineage-24.0`;
canonical `android/` currently contains integration owners, not the full platform.
Using that platform **read-only**, its real SDK37 aidl generated V2 headers and
source-defined Clang22 (`clang-r584948`) compiled **5/5 real sources to Android
arm64 objects**, with Bionic/libc++, vendor/VNDK macros and Werror. This proves
source signatures compile against the real frozen interface, **not** service
linking, the Soong vendor graph, VTS, registration or hardware behavior.

Reproduce from `tools/ulefone/gq5012bf1/hal-audit`:

```sh
PYTHONPATH=src PYTHONDONTWRITEBYTECODE=1 python3 -B \
  tests/android_adapter_probe.py /mnt/SamsungSSD/android/lineage-24.0
LIEPPOS_C51_SOURCE_TESTS=1 LIEPPOS_REAL_SOURCE_TESTS=1 \
  PYTHONPATH=src PYTHONDONTWRITEBYTECODE=1 python3 -B -m unittest discover -s tests -v
```

Recorded: **94/94 Python checks**; original backend **12/12**, new lifecycle/policy
**17/17**, both normal and ASan/UBSan; new suite also **17/17 TSan**. Host tests are
not Binder IPC or hardware proof. Temporary outputs are cleaned from the existing
ignored tooling `.tmp/`; generated platform headers are not committed.

Actual targeted request (isolated owned temporary OUT_DIR/GOCACHE/TMPDIR,
GOMAXPROCS=2, TARGET_PRODUCT=lineage_gq5012bf1, TARGET_RELEASE=cp2a,
TARGET_BUILD_VARIANT=userdebug):

```sh
bash build/soong/soong_ui.bash --make-mode -j2 android.hardware.vibrator-service.gq5012bf1
```

It reached product evaluation/Soong bootstrap and exited **1**:
`failed to load @config//main.star: open main.star: no such file or directory`.
The two hardware owner paths are also absent from that platform source tree.
**DeepSeek C53 owns checkout repair and single-root source wiring**. No repair,
repo sync, duplicate checkout, external source/output modification or shared
build-output reuse was performed by C51B.

## Exact device/vendor handoff for DeepSeek — NOT applied

**Now: keep stock selected and C51B unselected. Do not open either gate.**

After build, kernel/lifetime, policy and hardware gates are separately satisfied:

1. Wire the exact canonical hardware source commits into the single approved
   build root. Do not build an old donor copy. Add
   `PRODUCT_SOONG_NAMESPACES += hardware/ulefone/gq5012bf1` (its namespace imports
   MediaTek). Only then select
   `PRODUCT_PACKAGES += android.hardware.vibrator-service.gq5012bf1`.
2. C52 reports three stock installation entries: binary at proprietary-files line
   348, rc at 734, fragment at 980. Mark exactly these **EXTRACT_ONLY** in the
   extraction manifest and regenerate vendor configuration at product-definition
   time: `vendor/bin/hw/android.hardware.vibrator-service.mediatek`,
   `vendor/etc/init/vibrator-mtk-default.rc`,
   `vendor/etc/vintf/manifest/vibrator-mtk-default.xml`. C52 proved BoardConfig
   cannot mutate the late readonly PRODUCT_PACKAGES/COPY_FILES lists. Do not use
   a misleading late filter/flag or remove unrelated libraries by substring.
3. **Additional compatibility gate:** C52 reports the real stock fragment declares
   BOTH `IVibrator/default` and `IVibratorManager/default` V2. This source implements
   ONLY IVibrator. Do not silently drop Manager or invent its actuator IDs. Owner
   must obtain/validate the Manager contract, implement it in the proper owner or
   establish an explicitly compatible fallback/selection before replacing the
   whole stock fragment. Do not retain a second stock IVibrator server.
4. Install the new module-owned rc/fragment exactly once; do not also copy it or
   append it to DEVICE_MANIFEST_FILE. Validate the actual selected matrices/SKU/
   APEX merge with target tools. C52's stock-only compatibility result does not
   validate this replacement.
5. Include `hardware/ulefone/gq5012bf1/vibrator/sepolicy` in
   `BOARD_VENDOR_SEPOLICY_DIRS`; it contains only the executable file-context
   mapping. The target default vibrator domain is existing platform policy
   (C51 reference: system_sepolicy commit d61fd80832f558d52de026097d30d695634e8b0f).
   **No guessed node genfs/ueventd policy is supplied.** Obtain sanitized actual
   LED/real-device path, labels and ownership. Grant only this service's domain/
   system UID directory/link reads, max_brightness read, trigger read/write,
   brightness OFF write, invert/delay_on/delay_off/shot writes. No wildcard
   world-writable LED permissions, root UID, permissive domain or broad sysfs allow.
6. Kernel owner: review `hardware/mediatek/mt6878/vibrator/SAFETY.md` and unapplied
   `oneshot-config-proposal.patch`: require verified ONESHOT in the effective
   running kernel, and fix/review producer/workqueue/regulator cleanup and suspend/
   unbind races. A kernel off-attempt is not a guarantee against regulator failure.
7. Build only the selected module, link its real vendor dependencies, compile
   policy and test registration/IPC/VINTF. Only then arrange separately authorized
   real hardware/process-death tests on an identified safe image set. After all
   acceptance gates, review the explicit source code/init gate change. There is
   deliberately no one-variable production opt-in now.

Source availability, object compilation, full linking, compatibility, kernel
process-death behavior and hardware validation are separate properties. No claim
that the whole vibrator/kernel/PMIC/firmware stack is proven open or safe.

Research-ready C51B findings remain at
`tools/ulefone/gq5012bf1/hal-audit/docs/RESEARCH-HANDOFF.md`, **not yet merged into
the C53-owned research worktree**. C52 reports the earlier C51 summary was merged;
that does not merge or validate these new C51B changes. No push or phone operation.
