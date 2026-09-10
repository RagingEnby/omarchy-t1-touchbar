# Touch ID on the T1: current state and roadmap

## September 2026 update

[Standard Agents T1Bridge](https://github.com/standardagents/t1bridge) now ships
signed Arch/Omarchy packages for T1 Touch ID. Standard fprintd enrollment and
verification have succeeded on the local MacBookPro13,2. The driver requires
this Mac's preserved Apple fingerprint calibration data; installation alone
does not enroll a fingerprint or configure PAM. Keep the official libfprint
and fprintd variants together, retain password authentication, and test every
PAM consumer. System suspend/resume remains unsupported upstream.

The custom Omarchy layouts can use that hardware stack through the
[T1Bridge renderer adapter](T1BRIDGE-CUSTOM-BAR.md). Do not combine the old
firmware-mode startup rules with T1Bridge's configuration selector.

The August notes below are retained as historical research, not current
installation guidance.

## August 2026 research

Touch ID is materially harder than lighting the Touch Bar. The bar uses a
recoverable firmware boot plus HID/display interfaces. Touch ID is an
authentication system whose sensor is paired to the T1 Secure Enclave; biometric
templates and matching are designed to remain inside that secure boundary.

As of 2026-08-28 there is no public, reproducible T1 `libfprint` driver that can
be installed on Omarchy. The public T1 Linux status documentation still lists
Touch ID as unsupported.

A recent private proof-of-concept claims enrollment and matching on Omarchy, but
the author says the work started from macOS and preserved machine-specific T1
recovery/fingerprint calibration. No implementation or protocol has been
published yet. That is evidence that the hardware path may be possible, not a
driver that users can safely install.

## Why the existing activation is not enough

The successful EmbeddedOS recovery preserves the T1 and its device-specific FDR
state. That is a useful prerequisite, but it does not expose a standard USB
fingerprint reader to Linux and it does not provide:

- a documented command transport for enrollment and matching;
- secure session establishment with the T1 biometric service;
- sensor calibration handling;
- a `libfprint` match-on-chip driver;
- cancellation, timeout, retry, lockout, and power-state behavior;
- `fprintd` and PAM integration;
- a threat model and security review.

PAM must never be made to accept a fake success just to demonstrate a UI. The
password path must remain available and biometric failure must fail closed.

## Responsible implementation plan

A real project would be divided into these independently reviewable stages:

1. **Preservation and inventory**
   Back up same-Mac EmbeddedOS/FDR data and document the existing internal USB,
   HID, and service topology without printing biometric or device identity data.

2. **Protocol capture and clean-room description**
   Observe enrollment/match transactions on the owner's hardware, identify the
   T1 service and framing, and publish a protocol description that excludes keys,
   templates, calibration payloads, and Apple code.

3. **Read-only transport prototype**
   Establish a session and query capability/status with no enrollment, matching,
   template mutation, or PAM changes.

4. **Match-on-chip library**
   Implement enrollment and verification while keeping templates and matching
   inside the T1/Secure Enclave. Add cancellation, timeouts, lockout, and error
   mapping.

5. **libfprint/fprintd integration**
   Add a model-specific driver with tests and explicit same-device calibration
   handling.

6. **Authentication integration**
   Add optional PAM use only after code review. Keep password fallback, recovery
   documentation, and a one-command way to disable the biometric module.

7. **Security review**
   Test spoof resistance assumptions, replay resistance, privilege boundaries,
   log redaction, suspend/resume, and failure behavior.

This is likely a weeks-to-months reverse-engineering effort, not another evening
of DKMS work. The fastest responsible path is to collaborate with the private
proof-of-concept author if code becomes public, then adapt and review it for
Omarchy rather than starting with PAM.

## What this repository will accept

Useful contributions include:

- public protocol notes with no Apple or user secrets;
- sanitized USB/service topology;
- read-only transport probes;
- a match-on-chip `libfprint` design;
- tests and a threat model;
- links to newly published upstream work.

This repository will not accept biometric template dumps, calibration blobs,
machine tickets, insecure PAM bypasses, or another Mac's provisioning data.

## References

- [Apple Platform Security guide](https://help.apple.com/pdf/security/en_US/apple-platform-security-guide.pdf)
- [Public T1 Linux Touch ID status](https://github.com/elephantum/ublue-t1/blob/master/issues/touch-id-not-working.md)
- [Private Omarchy proof-of-concept discussion](https://www.reddit.com/r/linux_on_mac/comments/1vyyxuz/i_got_touchid_t1_201617_working_on_omarchy_linux/)
- [libfprint](https://gitlab.freedesktop.org/libfprint/libfprint)
- [fprintd](https://gitlab.freedesktop.org/libfprint/fprintd)
