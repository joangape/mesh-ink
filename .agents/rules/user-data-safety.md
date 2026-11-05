---
trigger: glob
globs: "src/message_store.*, src/backup_*, src/device_settings_snapshot.*, src/channel_key.h, src/board/t5_storage_backend.*, tests/test_backup_*.cpp, tools/check_backup_integration.py, tools/check_utf8_data_integrity.py"
description: "Backward compatibility and safety for message journals, NVS settings, SD backup/restore."
---

# User data safety

- On-device journals, NVS keys and the `.BAK` archive format are user data. Changes must stay **backward compatible**: old data must still read, and existing backups must still restore. If you need a format change, version it explicitly and add a migration plus host tests.
- Never add code that silently deletes, truncates or reformats user data, SPIFFS or SD contents. Orphan cleanup is limited to exact verified scratch names (`/rstNN.*`, `/r2sNN.*`, `/restore.txn.tmp`).
- Restore is a transaction: validate archive, manifest and CRCs first; stage; commit; roll back fully on failure; keep crash recovery working for legacy and current stage prefixes.
- Never log keys, private messages or payloads.
- Keep UTF-8 intact through journals and backups (`check_utf8_data_integrity.py`).
- Add or extend `tests/test_backup_*.cpp` for any format or pathname logic change. Also run `check_backup_integration.py`.
