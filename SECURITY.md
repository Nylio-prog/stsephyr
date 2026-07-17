# Security policy

Report suspected vulnerabilities privately to the repository maintainers rather
than opening a public issue. Include the affected revision, hardware profile,
reproduction steps, and potential impact.

Frame logging is disabled by default. Do not add logs containing host keys,
session keys, provisioning payloads, passwords, or other secret material.

The basic sample is intentionally non-destructive. Changes that provision keys,
modify access conditions, change the I2C address, or lock lifecycle state require
an explicit warning and must never run automatically in CI or on application
startup.
