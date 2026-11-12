# Changelog

The changelog for the ITB fork of SRS.

<a name="v6-itb-changes"></a>

## SRS 6.0 ITB Changelog
* v6.0-itb.21, 2026-11-12, Codec: Validate `lengthSizeMinusOne` before keeping it, so an H.264 or H.265 sequence header with the value 2 followed by a frame no longer aborts the server, backported from upstream [#4741](https://github.com/ossrs/srs/pull/4741). Based on upstream v6.0.195.
* v6.0-itb.21, 2026-11-12, HEVC: Fix a heap overflow in the sub-layer `profile_tier_level` of a VPS or SPS, backported from upstream v7.0.206, and refuse a VPS, SPS or PPS shorter than the 2 bytes NAL unit header instead of aborting. The HEVC parser runs before the `hevc_supported` check, so any publisher reached both. Based on upstream v6.0.195.
* v6.0-itb.21, 2026-11-12, RTMP: Keep a fetched live source in the pool while its client may yield before it publishes, such as in a slow `on_publish` hook, and treat a new source as idle since it was created, so the cleanup timer no longer drops it and a reconnecting camera no longer gets a second source with two publishers and two DVR files, backported from upstream v7.0.127 and v7.0.151. Based on upstream v6.0.195.
* v6.0-itb.21, 2026-11-12, Coroutine: Retry the mutex lock on `EINTR` instead of aborting when the coroutine was interrupted, such as by an HTTP API kickoff, backported from upstream v7.0.213. Based on upstream v6.0.195.
* v6.0-itb.21, 2026-11-12, ST: Use the monotonic clock for the State Threads timers, so a wall clock step, such as by NTP, no longer fires the receive and publish timeouts early nor stalls the timers, backported from upstream v7.0.17. Based on upstream v6.0.195.
* v6.0-itb.21, 2026-11-12, Tests: Add unit coverage for the rejected `lengthSizeMinusOne`, the HEVC sub-layer and short parameter sets, the source cleanup with pending clients, the interrupted mutex lock and the monotonic ST clock. Based on upstream v6.0.195.
* v6.0-itb.21, 2026-11-12, Fork: Bump the fork display version to `6.0.195-itb.21`. Based on upstream v6.0.195.
* v6.0-itb.20, 2026-11-12, DVR: When the DVR is active for the stream, that is the vhost DVR is enabled and `dvr_apply` matches it, end the RTMP publish when the DVR fails to record, at the first file or mid-session such as on a full disk or a failed segment open, so the camera never stays connected without recording. With the DVR disabled or not applied, a DVR error is still ignored. Based on upstream v6.0.195.
* v6.0-itb.20, 2026-11-12, Hooks: Close the cycle of a DVR failure with `on_dvr` for the file already recorded and `on_unpublish` with `unpublish_reason` `dvr_error` and `unpublish_code` 3190 (`DvrFailed`), also when the first file fails after `on_publish` was accepted. Based on upstream v6.0.195.
* v6.0-itb.20, 2026-11-12, RTMP: Release the publish state when the publish fails after the source was marked as published, so the stream no longer answers busy until restart, backported from upstream [#4703](https://github.com/ossrs/srs/pull/4703). Based on upstream v6.0.195.
* v6.0-itb.20, 2026-11-12, DVR: Close and remove a file whose header can't be written, so it is never renamed nor reported to `on_dvr`, backported from upstream v7.0.211. Based on upstream v6.0.195.
* v6.0-itb.20, 2026-11-12, Tests: Add unit coverage for the active DVR check, the DVR error handling with the DVR active, inactive and disabled, and the removal of a file whose header failed. Based on upstream v6.0.195.
* v6.0-itb.20, 2026-11-12, Fork: Bump the fork display version to `6.0.195-itb.20`. Based on upstream v6.0.195.
* v6.0-itb.19, 2026-11-12, Upstream: Sync the fork base from SRS v6.0.191 to the v6.0-r3 release at v6.0.195. Based on upstream v6.0.195.
* v6.0-itb.19, 2026-11-12, Security: Limit AMF0 object and array nesting to 64 levels, so a deeply nested RTMP or RTMPS `connect` or metadata can no longer overflow the coroutine stack and crash the server before any check. Based on upstream v6.0.195.
* v6.0-itb.19, 2026-11-12, Security: Limit JSON nesting to 64 levels, so a deeply nested hook response or `/rtc/v1/*` request body, which the HTTP API auth does not cover, can no longer crash the server. Based on upstream v6.0.195.
* v6.0-itb.19, 2026-11-12, Security: Refuse `..` in the app or stream name for every protocol, even with the vhost security disabled, so a stream name can no longer write DVR files outside `dvr_path`. Based on upstream v6.0.195.
* v6.0-itb.19, 2026-11-12, RTC: Check the peer DTLS certificate against the SDP fingerprint, fix an abort from a STUN attribute without padding, and fix a crash and heap overflow from a malformed RTCP packet. Based on upstream v6.0.195.
* v6.0-itb.19, 2026-11-12, TS/GB28181: Fix a malformed TS adaptation field that skipped past the packet and a GB28181 TCP heap overflow. Based on upstream v6.0.195.
* v6.0-itb.19, 2026-11-12, SRT: Upgrade libsrt from 1.5.3 to 1.5.6 for CVE-2026-55868 and CVE-2026-55869. Based on upstream v6.0.195.
* v6.0-itb.19, 2026-11-12, Config: Add `SRS_RTMP_LISTEN` for the RTMP listen, keeping `SRS_LISTEN` as an alias. Based on upstream v6.0.195.
* v6.0-itb.19, 2026-11-12, Fork: Bump the fork display version to `6.0.195-itb.19`. Based on upstream v6.0.195.
* v6.0-itb.18, 2026-11-10, RTMPS: Log the ciphers the client offers, in its order, as `client_ciphers` in the `client identified` line, to review the server order in the camera tests. Based on upstream v6.0.191.
* v6.0-itb.18, 2026-11-10, RTMPS: Log the ciphers, groups and signature algorithms the client offers when the TLS handshake fails, as `client_ciphers`, `client_groups` and `client_sigalgs`. Based on upstream v6.0.191.
* v6.0-itb.18, 2026-11-10, TLS: Log the OpenSSL reason when the handshake fails, such as `reason=no shared cipher`, for RTMPS and HTTPS, and clear the OpenSSL error queue before the handshake so errors of other coroutines are not taken. Based on upstream v6.0.191.
* v6.0-itb.18, 2026-11-10, Tests: Add unit coverage for the offered ciphers, groups and signature algorithms in the logs and the handshake failure reasons. Based on upstream v6.0.191.
* v6.0-itb.18, 2026-11-10, Fork: Bump the fork display version to `6.0.191-itb.18`. Based on upstream v6.0.191.
* v6.0-itb.17, 2026-11-09, RTMPS: Restrict TLS to ECDHE with AEAD ciphers in the server order, `X25519:P-256:P-384` groups and RSA signatures without SHA-1, and disable renegotiation, session tickets and compression, with `ciphers`, `groups` and `sigalgs` directives and `SRS_RTMPS_CIPHERS`, `SRS_RTMPS_GROUPS` and `SRS_RTMPS_SIGALGS` to change the lists. Based on upstream v6.0.191.
* v6.0-itb.17, 2026-11-09, RTMPS: Check the TLS profile, key and cert at start, and refuse to start when a cipher lacks ECDHE or AEAD, a signature uses SHA-1 or MD5, or the key is not RSA of 2048 bits or more, logging the OpenSSL version and the final lists. Based on upstream v6.0.191.
* v6.0-itb.17, 2026-11-09, HTTPS: Use the same default TLS profile and key checks as RTMPS for the HTTPS API and server. Based on upstream v6.0.191.
* v6.0-itb.17, 2026-11-09, Tests: Add unit coverage for the TLS profile directives, the security floor, the key and cert checks, and RTMPS handshakes with weak and preferred ciphers. Based on upstream v6.0.191.
* v6.0-itb.17, 2026-11-09, Fork: Bump the fork display version to `6.0.191-itb.17`. Based on upstream v6.0.191.
* v6.0-itb.16, 2026-11-09, OpenSSL: Upgrade the bundled `openssl-1.1-fit` from 1.1.1l to 1.1.1w, the last public 1.1.1 release, which fixes the public 1.1.1 advisories published after 1.1.1l, such as CVE-2022-0778, CVE-2022-4304 and CVE-2023-0286. Build with `--ssl-local=on` so a prebuilt OpenSSL in the build image, like the 1.1.1j in `ossrs/srs:ubuntu20`, is not used instead. Based on upstream v6.0.191.
* v6.0-itb.16, 2026-11-09, Fork: Bump the fork display version to `6.0.191-itb.16`. Based on upstream v6.0.191.
* v6.0-itb.15, 2026-11-05, Metrics: Export `srs_rtmp_publishers` on `/metrics`, a gauge of the RTMP publishers by transport, labeled `ssl` for RTMPS and `plaintext` for RTMP like the transport in logs, counting only publishers that own a stream, so rejected publishes and connections waiting to republish are excluded. Based on upstream v6.0.191.
* v6.0-itb.15, 2026-11-05, Tests: Add unit coverage for the RTMP publisher counters by transport, including republish and unbalanced stops, and for the TLS detection of the RTMP transports. Based on upstream v6.0.191.
* v6.0-itb.15, 2026-11-05, Fork: Bump the fork display version to `6.0.191-itb.15`. Based on upstream v6.0.191.
* v6.0-itb.14, 2026-10-21, RTMPS: Backport the RTMPS server from upstream [#4443](https://github.com/ossrs/srs/pull/4443), with a dedicated `rtmps` listener, TLS transport and `enabled`, `listen`, `key` and `cert` directives, while plain RTMP is unchanged. Based on upstream v6.0.191.
* v6.0-itb.14, 2026-10-21, RTMPS: Default `rtmps.listen` to `1443` when it is missing or empty, and refuse an enabled RTMPS whose listen is not a valid `[ip:]port` endpoint. Based on upstream v6.0.191.
* v6.0-itb.14, 2026-10-21, TLS: Treat a peer `close_notify` as a graceful disconnect, backported from upstream [#4036](https://github.com/ossrs/srs/issues/4036), and fix `SrsSslConnection::read_fully` to consume the decrypted bytes. Based on upstream v6.0.191.
* v6.0-itb.14, 2026-10-21, TLS: Time out the RTMPS TLS handshake after 30 seconds, so idle clients can no longer hold connections and exhaust `max_connections`. Based on upstream v6.0.191.
* v6.0-itb.14, 2026-10-21, Logs: Append the transport to the RTMP `client identified` line, with the negotiated TLS version, the TLS versions the client offers, the cipher and the SNI for RTMPS, and report the offered TLS versions when the RTMPS handshake fails. Based on upstream v6.0.191.
* v6.0-itb.14, 2026-10-21, Tests: Add unit coverage for the RTMPS configuration and listen validation, the TLS handshake timeout, and the logged transport and TLS details against a real TLS client. Based on upstream v6.0.191.
* v6.0-itb.14, 2026-10-21, Fork: Bump the fork display version to `6.0.191-itb.14`. Based on upstream v6.0.191.
* v6.0-itb.13, 2026-09-15, DVR/Hooks: Rename the `on_publish` audio-control field from `audio_supported` to `audio_enabled`, keeping the polarity so DVR skips audio when `audio_enabled` is `false`. Based on upstream v6.0.191.
* v6.0-itb.13, 2026-09-15, Tests: Update unit and blackbox coverage for the renamed `audio_enabled` hook field. Based on upstream v6.0.191.
* v6.0-itb.13, 2026-09-15, Fork: Bump the fork display version to `6.0.191-itb.13`. Based on upstream v6.0.191.
* v6.0-itb.12, 2026-09-15, Logs: Rename the RTMP stall and jitter warnings to the `RTMP_MEDIA_INGEST_STALL` and `RTMP_JITTER_DETECTED` keys, drop the vhost, split the stream path into separate `app` and `stream` fields, and report the stall durations as integer `elapsed_ms`, `socket_wait_ms`, and `server_time_ms` fields for backend parsing. Based on upstream v6.0.191.
* v6.0-itb.12, 2026-09-15, Tests: Add unit coverage pinning the full `RTMP_MEDIA_INGEST_STALL` and `RTMP_JITTER_DETECTED` warning lines, covering millisecond truncation, the socket-wait and server-time split, and the clamp on negative server time. Based on upstream v6.0.191.
* v6.0-itb.12, 2026-09-15, Fork: Bump the fork display version to `6.0.191-itb.12`. Based on upstream v6.0.191.
* v6.0-itb.11, 2026-09-11, DVR/Hooks: Rename the `on_publish` audio-control field from `skip_dvr_audio` to `audio_supported`, inverting the polarity so DVR skips audio when `audio_supported` is `false`. Based on upstream v6.0.191.
* v6.0-itb.11, 2026-09-11, Tests: Update unit and blackbox coverage for the renamed `audio_supported` hook field and inverted DVR audio-skip condition. Based on upstream v6.0.191.
* v6.0-itb.11, 2026-09-11, Fork: Bump the fork display version to `6.0.191-itb.11`. Based on upstream v6.0.191.
* v6.0-itb.10, 2026-08-13, Upstream: Sync the fork base from SRS v6.0.186 to the v6.0-r1 release at v6.0.191. Based on upstream v6.0.191.
* v6.0-itb.10, 2026-08-13, WebRTC/TCP: Reject a duplicate TCP owner before replacing the active session and safely handle teardown without an owner. Based on upstream v6.0.191.
* v6.0-itb.10, 2026-08-13, SDP: Terminate `a=ssrc-group` lines with CRLF so the following SDP attribute is not concatenated. Based on upstream v6.0.191.
* v6.0-itb.10, 2026-08-13, DVR/MP4: Preserve zero-duration STTS entries for repeated-DTS H.264 samples so MP4 timing and duration remain correct. Based on upstream v6.0.191.
* v6.0-itb.10, 2026-08-13, RTC2RTMP: Suppress repeated byte-identical AVC sequence headers while preserving keyframes, changed headers, and headers after republishing. Based on upstream v6.0.191.
* v6.0-itb.10, 2026-08-13, Metrics: Exclude graceful RTMP, HTTP, and SRT disconnects from `srs_clients_errs_total` while retaining genuine client errors. Based on upstream v6.0.191.
* v6.0-itb.10, 2026-08-13, Tests: Add regression coverage for SDP line termination, repeated-DTS MP4 timing, RTC-to-RTMP AVC header deduplication, and graceful disconnect metrics. Based on upstream v6.0.191.
* v6.0-itb.10, 2026-08-13, Release: Adopt the v6.0-r1 release metadata and updated GitHub Actions and asset-upload workflow. Based on upstream v6.0.191.
* v6.0-itb.10, 2026-08-13, Fork: Bump the fork display version to `6.0.191-itb.10`. Based on upstream v6.0.191.
* v6.0-itb.9, 2026-08-10, Logs: Include the application, stream name, and processing scope (`consumer` or `dvr`) in `RTMP jitter detected` warnings. Based on upstream v6.0.186.
* v6.0-itb.9, 2026-08-10, Tests: Verify that RTMP jitter warnings carry the configured application, stream, and consumer scope while preserving zero-timestamp warning suppression. Based on upstream v6.0.186.
* v6.0-itb.9, 2026-08-10, Fork: Bump the fork display version to `6.0.186-itb.9`. Based on upstream v6.0.186.
* v6.0-itb.8, 2026-08-07, Jitter: Suppress false `RTMP jitter detected` warnings when a fresh DVR or consumer context transitions from a cached zero-timestamp sequence header to current media, while preserving timestamp correction. Based on upstream v6.0.186.
* v6.0-itb.8, 2026-08-07, Tests: Add unit coverage for zero-timestamp warning suppression and continued reporting of subsequent timestamp discontinuities. Based on upstream v6.0.186.
* v6.0-itb.8, 2026-08-07, Fork: Bump the fork display version to `6.0.186-itb.8`. Based on upstream v6.0.186.
* v6.0-itb.7, 2026-08-04, RTMP: Add a passive socket-read observer during publishing to attribute media ingest stalls between time spent waiting in input reads and time spent elsewhere in the server path. Based on upstream v6.0.186.
* v6.0-itb.7, 2026-08-04, Logs: Replace the RTMP publisher media-gap warning with `RTMP media ingest stall`, including elapsed, socket-wait and server time, socket read and byte counts, and the previous and resumed media types and timestamps. Based on upstream v6.0.186.
* v6.0-itb.7, 2026-08-04, Tests: Add unit coverage for fast-buffer read observation, observer removal, and RTMP protocol observer forwarding. Based on upstream v6.0.186.
* v6.0-itb.7, 2026-08-04, Fork: Bump the fork display version to `6.0.186-itb.7`. Based on upstream v6.0.186.
* v6.0-itb.6, 2026-08-03, Hooks: Include the RTMP unpublish reason, numeric error code, error name, and summary in the `on_unpublish` payload while preserving the existing SRT and WebRTC payloads. Based on upstream v6.0.186.
* v6.0-itb.6, 2026-08-03, RTMP: Distinguish HTTP API kickoff from generic coroutine interruption and classify timeout, idle kickoff, client disconnect, client unpublish, interruption, and publish-error teardown paths. Based on upstream v6.0.186.
* v6.0-itb.6, 2026-08-03, Logs: Include the classified RTMP unpublish reason in successful `on_unpublish` HTTP hook traces. Based on upstream v6.0.186.
* v6.0-itb.6, 2026-08-03, Fork: Bump the fork display version to `6.0.186-itb.6`. Based on upstream v6.0.186.
* v6.0-itb.5, 2026-07-29, Logs: Report RTMP publisher media arrival gaps greater than one second with the affected vhost, stream, publisher IP, gap duration, and packet type that resumed the stream. Based on upstream v6.0.186.
* v6.0-itb.5, 2026-07-29, Jitter: Emit a warning when RTMP timestamp jitter correction is applied, including the packet type, received timestamp, previous packet timestamp, and detected delta. Based on upstream v6.0.186.
* v6.0-itb.5, 2026-07-29, Fork: Bump the fork display version to `6.0.186-itb.5`. Based on upstream v6.0.186.
* v6.0-itb.4, 2026-07-21, DVR: Discard finalized DVR files at or below the configured minimum size before the temporary file is renamed, preventing undersized artifacts from triggering `on_dvr`. Based on upstream v6.0.186.
* v6.0-itb.4, 2026-07-21, Config: Add the `dvr_min_file_size` directive and `SRS_VHOST_DVR_DVR_MIN_FILE_SIZE` override, with a 267-byte threshold in `cloud-storage.conf`. Based on upstream v6.0.186.
* v6.0-itb.4, 2026-07-21, Tests: Add unit coverage for configuration parsing, environment overrides, inclusive DVR size filtering, file removal, and callback suppression. Based on upstream v6.0.186.
* v6.0-itb.4, 2026-07-21, Fork: Bump the fork display version to `6.0.186-itb.4`. Based on upstream v6.0.186.
* v6.0-itb.3, 2026-07-20, DVR/MP4: Finalize DVR sessions without media samples as valid trackless MP4 containers, allowing the temporary file to be renamed and `on_dvr` to run when publishing is rejected before the first accepted packet. Based on upstream v6.0.186.
* v6.0-itb.3, 2026-07-20, HEVC: Finalize the existing AVC track and complete the DVR hook flow when a publisher changes codec to unsupported HEVC on the same RTMP connection. Based on upstream v6.0.186.
* v6.0-itb.3, 2026-07-20, Tests: Add MP4 unit and black-box coverage for immediate HEVC rejection, AVC-to-HEVC codec changes, finalized DVR artifacts, and removal of `.mp4.tmp` files. Based on upstream v6.0.186.
* v6.0-itb.3, 2026-07-20, Benchmark: Add an opt-in in-memory DVR muxer benchmark to compare FLV and MP4 processing, close cost, writes, seeks, and retained MP4 sample metadata. Based on upstream v6.0.186.
* v6.0-itb.3, 2026-07-20, Fork: Bump the fork display version to `6.0.186-itb.3`. Based on upstream v6.0.186.
* v6.0-itb.2, 2026-07-02, Coroutine: Retry `srs_thread_join` after `EINTR` during coroutine shutdown instead of asserting, covering cleanup paths where the caller was already interrupted, such as an RTMP client kickoff. Based on upstream v6.0.186.
* v6.0-itb.2, 2026-07-02, Logs: Report the coroutine identifiers, thread pointer, return value, and saved errno when `srs_thread_join` is interrupted or fails. Based on upstream v6.0.186.
* v6.0-itb.2, 2026-07-02, Tests: Add unit coverage for stopping a child coroutine from an already interrupted caller. Based on upstream v6.0.186.
* v6.0-itb.1, 2026-06-05, Config: Restore parsing compatibility for the legacy DVR `dvr_app_name_to_ignore_audio` directive without wiring it into DVR behavior. Based on upstream v6.0.186.
* v6.0-itb.1, 2026-06-05, Hooks: Parse `on_publish` JSON response `data.skip_dvr_audio` and `data.hevc_supported` stream controls while preserving the legacy success response body. Based on upstream v6.0.186.
* v6.0-itb.1, 2026-06-05, RTMP: Propagate publish-time stream controls from the RTMP request into the live source before publisher start. Based on upstream v6.0.186.
* v6.0-itb.1, 2026-06-05, DVR: Skip DVR audio packets when `skip_dvr_audio` is enabled for the published stream. Based on upstream v6.0.186.
* v6.0-itb.1, 2026-06-05, HEVC: Reject HEVC video packets by default unless the publish hook enables `hevc_supported` for the stream. Based on upstream v6.0.186.
* v6.0-itb.1, 2026-06-05, Jitter: Increase RTMP jitter tolerance from 250ms to 500ms and use 67ms as the fallback frame interval for ITB camera streams. Based on upstream v6.0.186.
* v6.0-itb.1, 2026-06-05, Logs: Include stream names in RTMP stream-service errors and HTTP API client kickoff logs. Based on upstream v6.0.186.
* v6.0-itb.1, 2026-06-05, Config: Add `cloud-storage.conf` sample with DVR, exporter, HTTP API, and cloud-storage HTTP hooks enabled. Based on upstream v6.0.186.
* v6.0-itb.1, 2026-06-05, Tests: Add unit and black-box coverage for publish-hook stream controls, DVR audio suppression, and HEVC publish authorization. Based on upstream v6.0.186.
* v6.0-itb.1, 2026-06-05, Docs: Add repo-local AI memory and SRS codebase navigation documents for the fork. Based on upstream v6.0.186.
* v6.0-itb.1, 2026-06-05, Docs: Add SRS user documentation, pages, and blog content under `trunk/3rdparty/srs-docs`. Based on upstream v6.0.186.
* v6.0-itb.1, 2026-06-05, Fork: Add separate ITB changelog and expose fork display version as `6.0.186-itb.1`. Based on upstream v6.0.186.
