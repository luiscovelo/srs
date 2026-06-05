# Changelog

The changelog for the ITB fork of SRS.

<a name="v6-itb-changes"></a>

## SRS 6.0 ITB Changelog
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
