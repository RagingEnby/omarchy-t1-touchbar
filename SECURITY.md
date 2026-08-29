# Security and private-data policy

## Never publish T1 identity material

Apple T1 activation produces device-bound data. Do not include any of the
following in a Git commit, issue, discussion, paste, screenshot, or support
bundle:

- FDR data or decoded FDR property lists;
- AP/SEP tickets, SHSH blobs, TSS requests, or TSS responses;
- personalized IMG4 components or `combined.memboot`;
- raw idevicerestore, usbmuxd, iBoot, restored, or recovery logs;
- serial numbers, ECIDs, unique chip IDs, AP nonces, or SEP nonces;
- Apple firmware packages or extracted firmware files.

The scripts keep this data below `/var/lib/t1-touchbar` with root-only
permissions. Preserve that boundary. Never use artifacts from another Mac.

## Safe issue information

A useful report can contain:

- Mac model, such as `MacBookPro13,2`;
- T1 USB state `05ac:1281` or `05ac:8600`;
- Linux and Omarchy versions;
- firmware product/build version;
- component names and byte sizes, but not component contents or hashes of
  private identity artifacts;
- restore status names and request names;
- whether private files compare equal;
- kernel module names, USB configuration number, HID IDs, and bound drivers;
- output from `./scripts/status.sh`.

Do not run tools with debug logging just to produce a public issue. If a raw log
is essential, redact it locally and review every line before sharing.

## Vulnerability reports

For a vulnerability in the guarded restore implementation, use GitHub's private
security-advisory reporting for this repository. Describe the issue without
attaching T1 identity data. If private reporting is unavailable, open a minimal
issue requesting a private contact channel.

## Authentication safety

Touch ID work must remain match-on-chip and fail closed. Do not submit PAM rules
that accept a synthetic success, export biometric templates, or bypass password
fallback. A Touch ID implementation requires a public threat model and review
before it is recommended for authentication.
