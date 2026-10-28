# GQ5012BF1 Android hardware source — C51

Canonical owner: `LieppOS/android_hardware_ulefone_gq5012bf1`.

One new service: `android.hardware.vibrator-service.gq5012bf1`, implementing the actual frozen **AIDL android.hardware.vibrator.IVibrator V2/default** interface. The source is independently authored, Apache-2.0 (`LICENSE`), and delegates real device access/timing to the single native backend in `hardware/mediatek/mt6878`. No camera/radio/security/biometric replacements are introduced.

## Authoritative interface and existing source review

- Target upstream: `LineageOS/android_hardware_interfaces`, branch `lineage-24.0`, commit **13fc23dae0dcd24fc2411d74f54a407999eaf555**.
- `vibrator/aidl/Android.bp` retains frozen versions 1, 2, 3 and 4 and the NDK backend. V2 build-definition SHA-256: `ce71a302b6d4f65e5b68b31162c0858a5149c92f8ed8aa34fa635f3cf984085a` (whole interface Android.bp).
- Frozen `vibrator/aidl/aidl_api/android.hardware.vibrator/2/android/hardware/vibrator/IVibrator.aidl`: SHA-256 **51161792eba95aee28a92723a1430b235e57e75751c24db04d84f7d83cca318e**. Its authentic Apache-2.0 snapshot is a test fixture in the existing HAL audit, not a local interface redefinition.
- The upstream example source is available, but models timing/callbacks and example capabilities; it is not a verified regulator-vibrator hardware backend. It was not imported or used to advertise fictitious effects/frequency/amplitude.
- Existing device extraction metadata references `android.hardware.vibrator-V2-ndk.so`, the MTK binary, `vibrator-mtk-default.rc/xml`, and stock init name `vendor.vibrator-default`. Those bodies are still unavailable. **V2 here is a real selected target API, not a recovered stock XML version or platform-compatibility claim.**

All 24 frozen V2 methods are declared/defined. Timed on/off performs actual LED-class attribute I/O using exact-driver discovery. Capabilities are zero; effects are empty; callbacks, amplitude/external control, compositions, always-on, resonance/frequency and braking are unsupported with explicit exceptions. Nonpositive duration and invalid amplitude inputs are rejected. Backend errno is returned as a Binder service-specific error; startup refuses to register without a successful initial off. The asynchronous kernel write contract is documented in the backend README.

## Independent validation states

| Property | Result |
|---|---|
| SOURCE_IMPLEMENTED | YES, timed on/off; optional unverified features explicitly unsupported |
| UNIT_TESTED | Native backend: 12/12 cases in normal and ASan/UBSan builds; source/init/VINTF/frozen method-set checks pass |
| SOONG_BUILD_PASSED | NO — not executed; canonical platform build dependencies absent |
| VINTF_VALIDATED | NO — XML consistency checked, not target libvintf compatibility |
| PLATFORM_INTEGRATED | NO — device/vendor activation intentionally untouched |
| HARDWARE_VERIFIED | NO |

The Binder adapter itself was **not compiled** using counterfeit generated headers. Missing dependencies are `build/soong`/envsetup, `hardware/interfaces`, generated `android.hardware.vibrator-V2-ndk` headers/library, `libbinder_ndk`, `libbase`, the target sysroot/toolchain build graph, target matrices and compiled SELinux policy. An actual native `-fsyntax-only` dependency probe of `vibrator/Vibrator.cpp` exited 1 at the missing authentic `aidl/android/hardware/vibrator/BnVibrator.h`; no replacement header was created. Native backend compilation is not an Android service build. No Android build failure was masked by changing the interface or disabling policy.

## Exact owner integration requirements (NOT applied)

1. Device owner: add `hardware/ulefone/gq5012bf1` to `PRODUCT_SOONG_NAMESPACES` (its namespace imports `hardware/mediatek/mt6878`) and select exactly `PRODUCT_PACKAGES += android.hardware.vibrator-service.gq5012bf1`.
2. Vendor/device owners: exclude the existing extracted `vendor/bin/hw/android.hardware.vibrator-service.mediatek`, `vendor/etc/init/vibrator-mtk-default.rc`, and `vendor/etc/vintf/manifest/vibrator-mtk-default.xml` from installation when selecting this replacement. The new init deliberately uses the **documented** `vendor.vibrator-default` name; installing both is a conflict. Do not delete unrelated vibrator libraries or fragments merely by substring.
3. Verify the real stock fragment and Android 17 matrix requirements before accepting V2/default. Install the new module-owned fragment **once**, not also through DEVICE_MANIFEST_FILE or copy rules. No current device manifest was edited.
4. Device policy owner: include `hardware/ulefone/gq5012bf1/vibrator/sepolicy` in `BOARD_VENDOR_SEPOLICY_DIRS`. Its one literal executable mapping uses target LineageOS 24's existing `hal_vibrator_default_exec` / `hal_vibrator_default` transition (`LineageOS/android_system_sepolicy` commit `d61fd80832f558d52de026097d30d695634e8b0f`, `vendor/hal_vibrator_default.te`). No root service, broad sysfs allow, permissive domain or SELinux override is added.
5. **Still required before activation:** sanitized actual LED-class/real-device sysfs path, existing labels and ueventd ownership for this driver. Grant only the service's system UID/domain the necessary directory/link reads and exact brightness write, preserving max_brightness read-only. Do not use guessed `/sys/class/leds/vibrator` labels or wildcard world-writable LED rules. No speculative genfs/ueventd rule is supplied because the actual DT label/real path is absent from authorized evidence.
6. Resolve the backend's documented **process-crash hardware timeout risk**. The kernel ABI has no timed-off guarantee; startup/destructor/worker safety is not a kernel fail-safe. Owner-approved platform and later crash/restart/suspend testing are prerequisites, not completed work.
7. In the real Android 17 checkout, build only `m android.hardware.vibrator-service.gq5012bf1`, compile the selected policy and run real assembled-metadata `checkvintf`/VTS as appropriate. No full ROM build/repo sync is needed or performed here.

The kernel ABI is verified by source and historical driver logs, but the whole vibrator stack and its kernel/regulator/firmware dependencies are not declared globally open-source or hardware-ready. No proprietary binary, firmware, key or calibration payload is included.

## Reproduce offline tests

From the existing `tools/ulefone/gq5012bf1/hal-audit` directory:

```sh
LIEPPOS_C51_SOURCE_TESTS=1 LIEPPOS_REAL_SOURCE_TESTS=1 \
PYTHONPATH=src PYTHONDONTWRITEBYTECODE=1 \
python3 -B -m unittest discover -s tests -v
```

`CXX` can select a trusted native Linux C++17 compiler; the recorded test uses GCC 16.2.1. Tests create/remove compiler outputs and synthetic sysfs files only in the tooling project's ignored `.tmp/`, never the real `/sys`. The actual Android interface remains a platform dependency, not a vendored fake header.

Research handoff is maintained only at `tools/ulefone/gq5012bf1/hal-audit/docs/RESEARCH-HANDOFF.md`, **not yet integrated** into the concurrent research checkout. No push or phone operation is authorized/performed.
