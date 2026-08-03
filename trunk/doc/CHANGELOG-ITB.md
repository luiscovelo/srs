# Changelog

The changelog for the ITB fork of SRS.

<a name="v6-itb-changes"></a>

## SRS 6.0 ITB Changelog
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
