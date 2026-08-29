# Exact x619 T1 activation protocol

This document records the behavior that changed a T1 from persistent recovery
`05ac:1281` to stable production iBridge `05ac:8600`.

## Architecture

```text
Touch Bar panel and digitizer
            |
            v
    Apple T1 / iBridge
       Apple secure boot
       EmbeddedOS + SEP
            |
            v
     USB 05ac:8600 config 1
            |
            v
 apple_ibridge -> virtual HID -> apple_touchbar -> Linux input
```

The Linux HID driver cannot make a recovery-mode T1 pass Apple secure boot. The
firmware and driver problems must be solved in that order.

## Guarded phase 11

The successful restored session used:

```text
ApBootstrapOnly = false
PersonalizedDuringPreflight = true
FlashNOR = true
ShouldRestoreSystemImage = false
CreateFilesystemPartitions = false
UpdateBaseband = false
RestoreBootArgs = rd=md0 -restore IOUSBDeviceController-configuration=standardMuxOnly
```

It omitted `SystemImage`, `SystemImageType`, `RootToInstall`, and phase-14
keys. The observed data flow was limited to T1 firmware/provisioning requests,
including `NORData`, `RootTicket`, `FDRTrustData`, `FDRMemoryCommit`, and
`FUDData`.

When no valid FDR store exists for this same physical T1:

- Pass A completes phase 11 and atomically saves the device-generated binary
  FDR memory store.
- After the commit wait and T1 reset, Pass B supplies that store in the root
  ticket response, completes phase 11 again, and requires the replayed store to
  be byte-identical.

## One preflight, two consumers

During Pass B, idevicerestore asks Apple for one device-specific TSS response.
That same response is used for:

- components consumed by the phase-11 provisioning restore;
- four personalized components saved for phase 14;
- the AP IMG4 ticket saved for phase 14.

The saved `combined.memboot` is a bare concatenation in manifest order:

```text
OSRamdisk || KernelCache || DeviceTree || SEP
```

Each member is already a personalized IMG4 object. There is no `2GMI` wrapper,
alignment padding, or rewritten size header.

## Commit boundary and reset

After `Status: Restore Finished`:

1. wait at least ten seconds so T1 provisioning/FDR state has committed;
2. stop the transient private usbmuxd;
3. load `acpi_call`;
4. invoke only `\_SB.PCI0.XHC1.RHUB.ASOC.FRST`;
5. require return value `0x0`;
6. wait for `05ac:1281`.

`FRST` resets the T1 without restarting the Intel host. `SOCW(1)` is not an
alternative; it can freeze the whole machine.

## Exact phase 14

Phase 14 is deliberately executed before ordinary TSS acquisition and before
the normal iBEC path:

1. reuse the recovery handle already opened during device discovery;
2. send `setenv auto-boot false`;
3. send `saveenv`;
4. upload the saved AP ticket and execute `ticket`;
5. upload the exact saved `combined.memboot`;
6. set `boot-args` to exactly `rd=md0`;
7. issue blind `memboot` using USB `bRequest=1`.

The initial local implementation opened a second recovery handle even though
device discovery still owned the first one. On Linux that path did not actually
deliver phase 14. Reusing the live handle made the transaction observable and
successful.

This phase-14 operation is not the same as a later EFI memboot helper. The EFI
helper normally uses `bRequest=0` on an already-staged file. Apple's bootstrap
phase uses `bRequest=1` and the explicit ticket/boot-argument sequence above.

## Success criteria

A zero process exit code only means the USB transaction was dispatched. It does
not prove the T1 accepted and booted the image.

Require all of:

- production USB ID `05ac:8600` remains present for at least 30 seconds;
- recovery ID `05ac:1281` does not return;
- USB configuration 1 is active;
- physical `05AC:8600` HIDs bind to `apple-ibridge-hid`;
- virtual `1D6B:0301` HIDs bind to `apple-touchbar`;
- a Linux Touch Bar input device is registered;
- the strip lights and Esc works physically.

Only then may that exact `combined.memboot`, same-Mac FDR data, and matching
`version.plist` be staged on EFI.

## Failed approaches

The following were tested or ruled out and should not be reintroduced:

- donor FDR data or donor personalized firmware;
- a fresh AP ticket requested after the T1 reset;
- mixing image and ticket from different preflight responses;
- a `2GMI` wrapper for the Image4 x619 image;
- restore-mode components used as the permanent normal-boot image;
- preloading or replacing iBEC during phase 14;
- normal `bRequest=0` command semantics for phase-14 memboot;
- omitting `auto-boot=false` and `saveenv`;
- phase 11 with `ApBootstrapOnly=true`;
- simultaneous system and private usbmuxd instances;
- staging a file before its exact bytes booted live;
- calling `SOCW(1)`.

Incorrect images commonly disconnected and returned to recovery within seconds.

## Source controls

The guarded branch recognizes only these T1-specific controls:

```text
IDEVICERESTORE_T1_EMBEDDEDOS
IDEVICERESTORE_T1_FDR_INPUT
IDEVICERESTORE_T1_FDR_OUTPUT
IDEVICERESTORE_RESTORE_BOOT_ARGS
IDEVICERESTORE_T1_PREFLIGHT_MEMBOOT_SAVE
IDEVICERESTORE_T1_PREFLIGHT_TICKET_SAVE
IDEVICERESTORE_T1_PHASE14
IDEVICERESTORE_MEMBOOT_FILE
IDEVICERESTORE_T1_APTICKET_FILE
```

The implementation validates the exact product/hardware model and boot-argument
strings, bounds artifact sizes, refuses group/world-readable ticket and image
files, writes private outputs atomically at mode `0600`, and suppresses T1
ECIDs, serials, and nonces from normal logs.

## References

- [Independent Linux-only verification](https://gist.github.com/tigercosmos/ecbfe1fc20b7303c1808d3ab74af1f5b)
- [idevicerestore upstream](https://github.com/libimobiledevice/idevicerestore)
- [libirecovery upstream](https://github.com/libimobiledevice/libirecovery)
- [usbmuxd upstream](https://github.com/libimobiledevice/usbmuxd)
- [Background on offline Touch Bar activation](https://onemoreadmin.wordpress.com/2016/11/30/the-untouchables-pt-2-offline-touchbar-activation-with-a-purged-disk/)
