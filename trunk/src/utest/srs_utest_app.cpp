//
// Copyright (c) 2013-2025 The SRS Authors
//
// SPDX-License-Identifier: MIT
//
#include <srs_utest_app.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <st.h>

using namespace std;

#include <srs_kernel_error.hpp>
#include <srs_app_fragment.hpp>
#include <srs_app_security.hpp>
#include <srs_app_config.hpp>
#include <srs_app_statistic.hpp>
#include <srs_app_dvr.hpp>
#include <srs_app_http_hooks.hpp>
#include <srs_app_recv_thread.hpp>
#include <srs_app_source.hpp>
#include <srs_kernel_file.hpp>
#include <srs_kernel_flv.hpp>
#include <srs_kernel_log.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_utest_config.hpp>

#include <srs_app_st.hpp>
#include <srs_protocol_conn.hpp>
#include <srs_app_conn.hpp>
#include <srs_protocol_st.hpp>
#include <srs_protocol_rtmp_stack.hpp>

class MockCountingLog : public ISrsLog
{
public:
    int warns;
    string last_warn;
public:
    MockCountingLog() {
        warns = 0;
    }
    virtual ~MockCountingLog() {
    }
public:
    virtual srs_error_t initialize() {
        return srs_success;
    }
    virtual void reopen() {
    }
    virtual void log(SrsLogLevel level, const char* /*tag*/, const SrsContextId& /*context_id*/, const char* fmt, va_list args) {
        if (level == SrsLogLevelWarn) {
            warns++;

            char message[1024];
            vsnprintf(message, sizeof(message), fmt, args);
            last_warn = message;
        }
    }
};

class MockLogGuard
{
private:
    ISrsLog* previous_;
public:
    MockLogGuard(ISrsLog* replacement) {
        previous_ = _srs_log;
        _srs_log = replacement;
    }
    virtual ~MockLogGuard() {
        _srs_log = previous_;
    }
};

VOID TEST(AppRtmpJitterTest, SuppressZeroLastPacketWarning)
{
    srs_error_t err = srs_success;
    MockCountingLog log;
    MockLogGuard guard(&log);
    SrsRtmpJitter jitter("live", "camera01", "consumer");

    SrsMessageHeader header;
    header.initialize_video(0, 0, 1);
    SrsSharedPtrMessage sequence_header;
    HELPER_EXPECT_SUCCESS(sequence_header.create(&header, NULL, 0));
    HELPER_EXPECT_SUCCESS(jitter.correct(&sequence_header, SrsRtmpJitterAlgorithmFULL));

    header.initialize_video(0, 62937513, 1);
    SrsSharedPtrMessage current_packet;
    HELPER_EXPECT_SUCCESS(current_packet.create(&header, NULL, 0));
    HELPER_EXPECT_SUCCESS(jitter.correct(&current_packet, SrsRtmpJitterAlgorithmFULL));
    EXPECT_EQ(0, log.warns);
    EXPECT_EQ(67, current_packet.timestamp);

    header.initialize_video(0, 62938513, 1);
    SrsSharedPtrMessage real_jitter;
    HELPER_EXPECT_SUCCESS(real_jitter.create(&header, NULL, 0));
    HELPER_EXPECT_SUCCESS(jitter.correct(&real_jitter, SrsRtmpJitterAlgorithmFULL));
    EXPECT_EQ(1, log.warns);
    EXPECT_NE(string::npos, log.last_warn.find("app=live"));
    EXPECT_NE(string::npos, log.last_warn.find("stream=camera01"));
    EXPECT_NE(string::npos, log.last_warn.find("scope=consumer"));
    EXPECT_EQ(134, real_jitter.timestamp);
}

VOID TEST(AppRtmpJitterTest, DetectedWarningFormat)
{
    srs_error_t err = srs_success;
    MockCountingLog log;
    MockLogGuard guard(&log);
    SrsRtmpJitter jitter("live", "camera01", "dvr");

    SrsMessageHeader header;
    header.initialize_video(0, 1000, 1);
    SrsSharedPtrMessage first;
    HELPER_EXPECT_SUCCESS(first.create(&header, NULL, 0));
    HELPER_EXPECT_SUCCESS(jitter.correct(&first, SrsRtmpJitterAlgorithmFULL));
    EXPECT_EQ(0, log.warns);

    header.initialize_video(0, 9000, 1);
    SrsSharedPtrMessage jittered;
    HELPER_EXPECT_SUCCESS(jittered.create(&header, NULL, 0));
    HELPER_EXPECT_SUCCESS(jitter.correct(&jittered, SrsRtmpJitterAlgorithmFULL));

    EXPECT_EQ(1, log.warns);
    EXPECT_STREQ("RTMP_JITTER_DETECTED: app=live, stream=camera01, scope=dvr, type=video, "
        "timestamp=9000ms, last_pkt_time=1000ms, delta=8000ms", log.last_warn.c_str());
}

VOID TEST(AppRtmpIngestStallTest, WarningFormat)
{
    MockCountingLog log;
    MockLogGuard guard(&log);

    SrsRequest req;
    req.vhost = "__defaultVhost__";
    req.app = "l";
    req.stream = "5776e2ba915046fc8f1e";
    req.ip = "177.53.246.143";

    // Sub-millisecond precision is truncated, not rounded, so 1048123us becomes 1048ms.
    srs_rtmp_warn_media_ingest_stall(&req, 1048123, 1048123, 1, 296,
        RTMP_MSG_AudioMessage, 727290, RTMP_MSG_AudioMessage, 728410);

    EXPECT_EQ(1, log.warns);
    EXPECT_STREQ("RTMP_MEDIA_INGEST_STALL: app=l, stream=5776e2ba915046fc8f1e, ip=177.53.246.143, "
        "elapsed_ms=1048, socket_wait_ms=1048, server_time_ms=0, socket_reads=1, socket_bytes=296, "
        "previous_ms=audio@727290, resumed_ms=audio@728410", log.last_warn.c_str());
}

VOID TEST(AppRtmpIngestStallTest, SplitSocketWaitAndServerTime)
{
    MockCountingLog log;
    MockLogGuard guard(&log);

    SrsRequest req;
    req.app = "live";
    req.stream = "livestream";
    req.ip = "10.0.0.1";

    srs_rtmp_warn_media_ingest_stall(&req, 2500 * 1000, 400 * 1000, 7, 65536,
        RTMP_MSG_VideoMessage, 12345, RTMP_MSG_AudioMessage, 14845);

    EXPECT_EQ(1, log.warns);
    EXPECT_STREQ("RTMP_MEDIA_INGEST_STALL: app=live, stream=livestream, ip=10.0.0.1, "
        "elapsed_ms=2500, socket_wait_ms=400, server_time_ms=2100, socket_reads=7, socket_bytes=65536, "
        "previous_ms=video@12345, resumed_ms=audio@14845", log.last_warn.c_str());
}

VOID TEST(AppRtmpIngestStallTest, ClampNegativeServerTime)
{
    MockCountingLog log;
    MockLogGuard guard(&log);

    SrsRequest req;
    req.app = "live";
    req.stream = "livestream";
    req.ip = "10.0.0.1";

    // The socket wait may exceed the media interval by a few microseconds, never report it as negative.
    srs_rtmp_warn_media_ingest_stall(&req, 1200 * 1000, 1300 * 1000, 2, 1024,
        RTMP_MSG_AudioMessage, 100, 0, 1300);

    EXPECT_EQ(1, log.warns);
    EXPECT_STREQ("RTMP_MEDIA_INGEST_STALL: app=live, stream=livestream, ip=10.0.0.1, "
        "elapsed_ms=1200, socket_wait_ms=1300, server_time_ms=0, socket_reads=2, socket_bytes=1024, "
        "previous_ms=audio@100, resumed_ms=other@1300", log.last_warn.c_str());
}

class MockIDResource : public ISrsResource
{
public:
    int id;
    MockIDResource(int v) {
        id = v;
    }
    virtual ~MockIDResource() {
    }
    virtual const SrsContextId& get_id() {
        return _srs_context->get_id();
    }
    virtual std::string desc() {
        return "";
    }
};

class MockStatisticExpire : public ISrsExpire
{
public:
    virtual void expire() {
    }
};

VOID TEST(StatisticTest, GracefulDisconnectsDoNotIncrementErrors)
{
    srs_error_t err = srs_success;

    SrsStatistic stat;
    SrsRequest req;
    req.vhost = "test.vhost";
    req.app = "live";
    req.stream = "livestream";
    MockStatisticExpire conn;

    int graceful_codes[] = {
        ERROR_SOCKET_READ,
        ERROR_SOCKET_READ_FULLY,
        ERROR_SOCKET_WRITE,
        ERROR_SRT_IO,
        ERROR_HTTP_STREAM_EOF,
    };
    int nb_graceful_codes = sizeof(graceful_codes) / sizeof(int);

    for (int i = 0; i < nb_graceful_codes; i++) {
        stringstream ss;
        ss << "client-" << i;
        string client_id = ss.str();
        HELPER_EXPECT_SUCCESS(stat.on_client(client_id, &req, &conn, SrsRtmpConnPlay));

        srs_error_t graceful_close = srs_error_new(graceful_codes[i], "gracefully closed");
        EXPECT_TRUE(srs_is_client_gracefully_close(graceful_close) || srs_is_server_gracefully_close(graceful_close));
        stat.on_disconnect(client_id, graceful_close);
        srs_freep(graceful_close);
    }

    HELPER_EXPECT_SUCCESS(stat.on_client("failed-client", &req, &conn, SrsRtmpConnPlay));
    srs_error_t failure = srs_error_new(ERROR_RTMP_HANDSHAKE, "genuine failure");
    stat.on_disconnect("failed-client", failure);
    srs_freep(failure);

    int64_t send_bytes = 0;
    int64_t recv_bytes = 0;
    int64_t nstreams = 0;
    int64_t nclients = 0;
    int64_t total_nclients = 0;
    int64_t nerrs = 0;
    HELPER_EXPECT_SUCCESS(stat.dumps_metrics(send_bytes, recv_bytes, nstreams, nclients, total_nclients, nerrs));

    EXPECT_EQ(0, nclients);
    EXPECT_EQ(nb_graceful_codes + 1, total_nclients);
    EXPECT_EQ(1, nerrs);
}

VOID TEST(StatisticTest, RtmpPublishersByTransport)
{
    SrsStatistic stat;
    int64_t tls = -1, plaintext = -1;

    stat.dumps_rtmp_publishers(tls, plaintext);
    EXPECT_EQ(0, tls);
    EXPECT_EQ(0, plaintext);

    // Two RTMPS publishers and one RTMP publisher.
    stat.on_rtmp_publish_start(true);
    stat.on_rtmp_publish_start(true);
    stat.on_rtmp_publish_start(false);
    stat.dumps_rtmp_publishers(tls, plaintext);
    EXPECT_EQ(2, tls);
    EXPECT_EQ(1, plaintext);

    // A republish on the same connection stops then starts again, so it never double counts.
    for (int i = 0; i < 3; i++) {
        stat.on_rtmp_publish_stop(true);
        stat.on_rtmp_publish_start(true);
    }
    stat.dumps_rtmp_publishers(tls, plaintext);
    EXPECT_EQ(2, tls);
    EXPECT_EQ(1, plaintext);

    stat.on_rtmp_publish_stop(true);
    stat.on_rtmp_publish_stop(false);
    stat.dumps_rtmp_publishers(tls, plaintext);
    EXPECT_EQ(1, tls);
    EXPECT_EQ(0, plaintext);

    // An unbalanced stop must not make the gauge negative.
    stat.on_rtmp_publish_stop(false);
    stat.dumps_rtmp_publishers(tls, plaintext);
    EXPECT_EQ(1, tls);
    EXPECT_EQ(0, plaintext);
}

VOID TEST(AppResourceManagerTest, FindByFastID)
{
    srs_error_t err = srs_success;

    if (true) {
        SrsResourceManager m("test");
        HELPER_EXPECT_SUCCESS(m.start());

        m.add_with_fast_id(101, new MockIDResource(1));
        m.add_with_fast_id(102, new MockIDResource(2));
        m.add_with_fast_id(103, new MockIDResource(3));
        EXPECT_EQ(1, ((MockIDResource*)m.find_by_fast_id(101))->id);
        EXPECT_EQ(2, ((MockIDResource*)m.find_by_fast_id(102))->id);
        EXPECT_EQ(3, ((MockIDResource*)m.find_by_fast_id(103))->id);
    }

    if (true) {
        SrsResourceManager m("test");
        HELPER_EXPECT_SUCCESS(m.start());

        MockIDResource* r1 = new MockIDResource(1);
        MockIDResource* r2 = new MockIDResource(2);
        MockIDResource* r3 = new MockIDResource(3);
        m.add_with_fast_id(101, r1);
        m.add_with_fast_id(102, r2);
        m.add_with_fast_id(103, r3);
        EXPECT_EQ(1, ((MockIDResource*)m.find_by_fast_id(101))->id);
        EXPECT_EQ(2, ((MockIDResource*)m.find_by_fast_id(102))->id);
        EXPECT_EQ(3, ((MockIDResource*)m.find_by_fast_id(103))->id);

        m.remove(r2); srs_usleep(0);
        EXPECT_TRUE(m.find_by_fast_id(102) == NULL);
    }

    if (true) {
        SrsResourceManager m("test");
        HELPER_EXPECT_SUCCESS(m.start());

        MockIDResource* r1 = new MockIDResource(1);
        MockIDResource* r2 = new MockIDResource(2);
        MockIDResource* r3 = new MockIDResource(3);
        m.add_with_fast_id(1, r1);
        m.add_with_fast_id(100001, r2);
        m.add_with_fast_id(1000001, r3);
        EXPECT_EQ(1, ((MockIDResource*)m.find_by_fast_id(1))->id);
        EXPECT_EQ(2, ((MockIDResource*)m.find_by_fast_id(100001))->id);
        EXPECT_EQ(3, ((MockIDResource*)m.find_by_fast_id(1000001))->id);

        m.remove(r2); srs_usleep(0);
        EXPECT_TRUE(m.find_by_fast_id(100001) == NULL);

        m.remove(r3); srs_usleep(0);
        EXPECT_TRUE(m.find_by_fast_id(1000001) == NULL);

        m.remove(r1); srs_usleep(0);
        EXPECT_TRUE(m.find_by_fast_id(1) == NULL);
    }

    if (true) {
        SrsResourceManager m("test");
        HELPER_EXPECT_SUCCESS(m.start());

        m.add_with_fast_id(101, new MockIDResource(1));
        m.add_with_fast_id(10101, new MockIDResource(2));
        m.add_with_fast_id(1010101, new MockIDResource(3));
        m.add_with_fast_id(101010101, new MockIDResource(4));
        m.add_with_fast_id(10101010101LL, new MockIDResource(5));
        m.add_with_fast_id(1010101010101LL, new MockIDResource(6));
        m.add_with_fast_id(101010101010101LL, new MockIDResource(7));
        m.add_with_fast_id(10101010101010101LL, new MockIDResource(8));
        m.add_with_fast_id(1010101010101010101ULL, new MockIDResource(9));
        m.add_with_fast_id(11010101010101010101ULL, new MockIDResource(10));
        EXPECT_EQ(1, ((MockIDResource*)m.find_by_fast_id(101))->id);
        EXPECT_EQ(2, ((MockIDResource*)m.find_by_fast_id(10101))->id);
        EXPECT_EQ(3, ((MockIDResource*)m.find_by_fast_id(1010101))->id);
        EXPECT_EQ(4, ((MockIDResource*)m.find_by_fast_id(101010101))->id);
        EXPECT_EQ(5, ((MockIDResource*)m.find_by_fast_id(10101010101LL))->id);
        EXPECT_EQ(6, ((MockIDResource*)m.find_by_fast_id(1010101010101LL))->id);
        EXPECT_EQ(7, ((MockIDResource*)m.find_by_fast_id(101010101010101LL))->id);
        EXPECT_EQ(8, ((MockIDResource*)m.find_by_fast_id(10101010101010101LL))->id);
        EXPECT_EQ(9, ((MockIDResource*)m.find_by_fast_id(1010101010101010101ULL))->id);
        EXPECT_EQ(10, ((MockIDResource*)m.find_by_fast_id(11010101010101010101ULL))->id);
    }

    if (true) {
        SrsResourceManager m("test");
        HELPER_EXPECT_SUCCESS(m.start());

        m.add_with_fast_id(101, new MockIDResource(1));
        m.add_with_fast_id(101, new MockIDResource(4));
        EXPECT_EQ(1, ((MockIDResource*)m.find_by_fast_id(101))->id);
    }
}

VOID TEST(AppCoroutineTest, Dummy)
{
    SrsDummyCoroutine dc;

    if (true) {
        SrsContextId v = dc.cid();
        EXPECT_TRUE(v.empty());

        srs_error_t err = dc.pull();
        EXPECT_TRUE(err != srs_success);
        EXPECT_TRUE(ERROR_THREAD_DUMMY == srs_error_code(err));
        srs_freep(err);

        err = dc.start();
        EXPECT_TRUE(err != srs_success);
        EXPECT_TRUE(ERROR_THREAD_DUMMY == srs_error_code(err));
        srs_freep(err);
    }

    if (true) {
        dc.stop();

        SrsContextId v = dc.cid();
        EXPECT_TRUE(v.empty());

        srs_error_t err = dc.pull();
        EXPECT_TRUE(err != srs_success);
        EXPECT_TRUE(ERROR_THREAD_DUMMY == srs_error_code(err));
        srs_freep(err);

        err = dc.start();
        EXPECT_TRUE(err != srs_success);
        EXPECT_TRUE(ERROR_THREAD_DUMMY == srs_error_code(err));
        srs_freep(err);
    }

    if (true) {
        dc.interrupt();

        SrsContextId v = dc.cid();
        EXPECT_TRUE(v.empty());

        srs_error_t err = dc.pull();
        EXPECT_TRUE(err != srs_success);
        EXPECT_TRUE(ERROR_THREAD_DUMMY == srs_error_code(err));
        srs_freep(err);

        err = dc.start();
        EXPECT_TRUE(err != srs_success);
        EXPECT_TRUE(ERROR_THREAD_DUMMY == srs_error_code(err));
        srs_freep(err);
    }
}

VOID TEST(AppHttpHooksTest, OnPublishResponseFlags)
{
    if (true) {
        SrsRequest req;
        SrsHttpHooks::parse_on_publish_response("{\"code\":0,\"data\":{\"audio_enabled\":true,\"hevc_supported\":true}}", &req);
        EXPECT_TRUE(req.audio_enabled);
        EXPECT_TRUE(req.hevc_supported);
    }

    if (true) {
        SrsRequest req;
        SrsHttpHooks::parse_on_publish_response("{\"code\":0,\"data\":{}}", &req);
        EXPECT_FALSE(req.audio_enabled);
        EXPECT_FALSE(req.hevc_supported);
    }

    if (true) {
        SrsRequest req;
        SrsHttpHooks::parse_on_publish_response("0", &req);
        EXPECT_FALSE(req.audio_enabled);
        EXPECT_FALSE(req.hevc_supported);
    }

    if (true) {
        SrsRequest req;
        SrsHttpHooks::parse_on_publish_response("{\"code\":0,\"data\":{\"audio_enabled\":false}}", &req);
        EXPECT_FALSE(req.audio_enabled);
        EXPECT_FALSE(req.hevc_supported);
    }
}

VOID TEST(AppDvrTest, IgnoreAudioByRequest)
{
    srs_error_t err = srs_success;

    SrsDvr dvr;
    dvr.actived = true;
    dvr.req = new SrsRequest();

    HELPER_EXPECT_SUCCESS(dvr.on_audio(NULL, NULL));
}

class MockSrsDvrFileSizeConfig
{
public:
    SrsConfig* previous;
    MockSrsConfig config;
public:
    MockSrsDvrFileSizeConfig() {
        previous = _srs_config;
        _srs_config = &config;
    }
    virtual ~MockSrsDvrFileSizeConfig() {
        _srs_config = previous;
    }
};

class MockSrsDvrFileSizePlan : public SrsDvrPlan
{
public:
    int nb_reaped;
public:
    MockSrsDvrFileSizePlan() {
        nb_reaped = 0;
    }
public:
    virtual srs_error_t on_reap_segment() {
        nb_reaped++;
        return srs_success;
    }
};

class MockSrsDvrFileSizeSegmenter : public SrsDvrSegmenter
{
public:
    virtual srs_error_t refresh_metadata() {
        return srs_success;
    }
protected:
    virtual srs_error_t open_encoder() {
        return srs_success;
    }
    virtual srs_error_t encode_metadata(SrsSharedPtrMessage* /*metadata*/) {
        return srs_success;
    }
    virtual srs_error_t encode_audio(SrsSharedPtrMessage* /*audio*/, SrsFormat* /*format*/) {
        return srs_success;
    }
    virtual srs_error_t encode_video(SrsSharedPtrMessage* /*video*/, SrsFormat* /*format*/) {
        return srs_success;
    }
    virtual srs_error_t close_encoder() {
        return srs_success;
    }
};

VOID TEST(AppDvrTest, DiscardFileAtOrBelowConfiguredMinimum)
{
    srs_error_t err = srs_success;
    string path = _srs_tmp_file_prefix + "dvr-min-file-size.flv";
    string tmp_path = path + ".tmp";
    ::unlink(path.c_str());
    ::unlink(tmp_path.c_str());

    MockSrsDvrFileSizeConfig config;
    SrsSetEnvConfig(config.config, dvr_min_file_size, "SRS_VHOST_DVR_DVR_MIN_FILE_SIZE", "267");

    SrsRequest req;
    req.vhost = "vhost";
    req.app = "live";
    req.stream = "stream";

    MockSrsDvrFileSizePlan plan;

    if (true) {
        MockSrsDvrFileSizeSegmenter segmenter;
        segmenter.req = &req;
        segmenter.plan = &plan;
        segmenter.fragment->set_path(path);

        HELPER_ASSERT_SUCCESS(segmenter.fs->open(tmp_path));
        char data[267] = {0};
        HELPER_ASSERT_SUCCESS(segmenter.fs->write(data, sizeof(data), NULL));
        HELPER_ASSERT_SUCCESS(segmenter.close());

        EXPECT_FALSE(srs_path_exists(tmp_path));
        EXPECT_FALSE(srs_path_exists(path));
        EXPECT_EQ(0, plan.nb_reaped);
    }

    if (true) {
        MockSrsDvrFileSizeSegmenter segmenter;
        segmenter.req = &req;
        segmenter.plan = &plan;
        segmenter.fragment->set_path(path);

        HELPER_ASSERT_SUCCESS(segmenter.fs->open(tmp_path));
        char data[268] = {0};
        HELPER_ASSERT_SUCCESS(segmenter.fs->write(data, sizeof(data), NULL));
        HELPER_ASSERT_SUCCESS(segmenter.close());

        EXPECT_FALSE(srs_path_exists(tmp_path));
        EXPECT_TRUE(srs_path_exists(path));
        EXPECT_EQ(1, plan.nb_reaped);
    }

    ::unlink(path.c_str());
}

class MockSrsDvrOpenFailSegmenter : public MockSrsDvrFileSizeSegmenter
{
protected:
    virtual srs_error_t open_encoder() {
        return srs_error_new(ERROR_SYSTEM_FILE_WRITE, "mock header write failed");
    }
};

VOID TEST(AppDvrTest, OpenFailureRemovesTmpFile)
{
    srs_error_t err = srs_success;
    string path = _srs_tmp_file_prefix + "dvr-open-fail-stream.flv";
    string tmp_path = path + ".tmp";
    ::unlink(path.c_str());
    ::unlink(tmp_path.c_str());

    MockSrsDvrFileSizeConfig config;
    HELPER_ASSERT_SUCCESS(config.config.parse(_MIN_OK_CONF "vhost vhost{dvr{dvr_path " + _srs_tmp_file_prefix + "dvr-open-fail-[stream].flv;}}"));

    SrsRequest req;
    req.vhost = "vhost";
    req.app = "live";
    req.stream = "stream";

    MockSrsDvrFileSizePlan plan;
    MockSrsDvrOpenFailSegmenter segmenter;
    segmenter.req = &req;
    segmenter.plan = &plan;

    // The header of the file can't be written, so the file is closed and removed.
    HELPER_EXPECT_FAILED(segmenter.open());
    EXPECT_FALSE(segmenter.fs->is_open());
    EXPECT_FALSE(srs_path_exists(tmp_path));

    // A later close never names nor reports the file.
    HELPER_EXPECT_SUCCESS(segmenter.close());
    EXPECT_FALSE(srs_path_exists(path));
    EXPECT_EQ(0, plan.nb_reaped);
}

VOID TEST(AppDvrTest, ActiveOnlyWhenEnabledAndApplied)
{
    srs_error_t err = srs_success;

    MockSrsDvrFileSizeConfig config;
    SrsDvr dvr;
    dvr.req = new SrsRequest();
    dvr.req->vhost = "v";

    // DVR applies to the stream, but the vhost DVR is disabled.
    HELPER_ASSERT_SUCCESS(config.config.parse(_MIN_OK_CONF "vhost v{dvr{enabled off;}}"));
    dvr.actived = true;
    EXPECT_FALSE(dvr.is_active());

    // The vhost DVR is enabled, but dvr_apply does not match the stream.
    HELPER_ASSERT_SUCCESS(config.config.parse(_MIN_OK_CONF "vhost v{dvr{enabled on;}}"));
    dvr.actived = false;
    EXPECT_FALSE(dvr.is_active());

    // Enabled and applied.
    dvr.actived = true;
    EXPECT_TRUE(dvr.is_active());
}

class MockSrsDvrErrorPlan : public SrsDvrPlan
{
public:
    int nb_unpublish;
public:
    MockSrsDvrErrorPlan() {
        nb_unpublish = 0;
    }
public:
    virtual void on_unpublish() {
        nb_unpublish++;
    }
};

VOID TEST(AppDvrTest, ErrorEndsPublishOnlyWhenActive)
{
    srs_error_t err = srs_success;

    MockSrsDvrFileSizeConfig config;
    HELPER_ASSERT_SUCCESS(config.config.parse(_MIN_OK_CONF "vhost v{dvr{enabled on;}}"));

    SrsDvr dvr;
    dvr.req = new SrsRequest();
    dvr.req->vhost = "v";
    MockSrsDvrErrorPlan* plan = new MockSrsDvrErrorPlan();
    dvr.plan = plan;

    // No error, nothing to do.
    dvr.actived = true;
    HELPER_EXPECT_SUCCESS(srs_dvr_on_error(&dvr, srs_success, "video"));
    EXPECT_EQ(0, plan->nb_unpublish);

    // The DVR records the stream, so the failure ends the publish as a DVR error, keeping the cause.
    if (true) {
        srs_error_t r0 = srs_dvr_on_error(&dvr, srs_error_new(ERROR_SYSTEM_FILE_WRITE, "disk full"), "video");
        EXPECT_EQ(ERROR_DVR_FAILED, srs_error_code(r0));
        EXPECT_TRUE(srs_error_summary(r0).find("dvr video") != string::npos);
        EXPECT_TRUE(srs_error_summary(r0).find("disk full") != string::npos);
        srs_freep(r0);
        EXPECT_EQ(0, plan->nb_unpublish);
    }

    // The DVR does not record the stream, so the error is ignored and the DVR stops, as before.
    dvr.actived = false;
    HELPER_EXPECT_SUCCESS(srs_dvr_on_error(&dvr, srs_error_new(ERROR_SYSTEM_FILE_WRITE, "disk full"), "audio"));
    EXPECT_EQ(1, plan->nb_unpublish);

    // The vhost DVR is disabled, for example by a reload, so the error never ends the publish.
    HELPER_ASSERT_SUCCESS(config.config.parse(_MIN_OK_CONF "vhost v{dvr{enabled off;}}"));
    dvr.actived = true;
    HELPER_EXPECT_SUCCESS(srs_dvr_on_error(&dvr, srs_error_new(ERROR_SYSTEM_FILE_WRITE, "disk full"), "video"));
    EXPECT_EQ(2, plan->nb_unpublish);
}

class MockCoroutineHandler : public ISrsCoroutineHandler {
public:
    SrsSTCoroutine* trd;
    srs_error_t err;
    srs_cond_t running;
    srs_cond_t exited;
    SrsContextId cid;
    // Quit without error.
    bool quit;
public:
    MockCoroutineHandler() : trd(NULL), err(srs_success), quit(false) {
        cid.set_value("0");
        running = srs_cond_new();
        exited = srs_cond_new();
    }
    virtual ~MockCoroutineHandler() {
        srs_cond_destroy(running);
        srs_cond_destroy(exited);
    }
public:
    virtual srs_error_t cycle() {
        srs_error_t r0 = srs_success;

        srs_cond_signal(running);

        // The cid should be generated if empty.
        cid = _srs_context->get_id();

        while (!quit && (r0 = trd->pull()) == srs_success && err == srs_success) {
            srs_usleep(10 * SRS_UTIME_MILLISECONDS);
        }

        srs_cond_signal(exited);

        // The cid might be updated.
        cid = _srs_context->get_id();

        if (err != srs_success) {
            srs_freep(r0);
            return err;
        }

        return r0;
    }
};

class MockInterruptedJoinHandler : public ISrsCoroutineHandler {
public:
    srs_cond_t stopped_cond;
    bool stopped;
public:
    MockInterruptedJoinHandler() : stopped(false) {
        stopped_cond = srs_cond_new();
    }
    virtual ~MockInterruptedJoinHandler() {
        srs_cond_destroy(stopped_cond);
    }
public:
    virtual srs_error_t cycle() {
        MockCoroutineHandler child_handler;
        SrsSTCoroutine child("child", &child_handler);
        child_handler.trd = &child;

        srs_error_t err = child.start();
        if (err != srs_success) {
            return err;
        }

        srs_cond_timedwait(child_handler.running, 100 * SRS_UTIME_MILLISECONDS);

        // Simulate API kickoff: The publisher coroutine is interrupted before
        // it enters cleanup and joins another coroutine.
        srs_thread_interrupt(srs_thread_self());
        child.stop();

        stopped = true;
        srs_cond_signal(stopped_cond);

        return srs_success;
    }
};

VOID TEST(AppCoroutineTest, SetCidOfCoroutine)
{
    srs_error_t err = srs_success;

    MockCoroutineHandler ch;
    SrsSTCoroutine sc("test", &ch);
    ch.trd = &sc;
    EXPECT_TRUE(sc.cid().empty());

    // Start coroutine, which will create the cid.
    HELPER_ASSERT_SUCCESS(sc.start());
    HELPER_ASSERT_SUCCESS(sc.pull());

    srs_cond_timedwait(ch.running, 100 * SRS_UTIME_MILLISECONDS);
    EXPECT_TRUE(!sc.cid().empty());
    EXPECT_TRUE(!ch.cid.empty());

    // Should be a new cid.
    SrsContextId cid = _srs_context->generate_id();
    EXPECT_TRUE(sc.cid().compare(cid) != 0);
    EXPECT_TRUE(ch.cid.compare(cid) != 0);

    // Set the cid and stop the coroutine.
    sc.set_cid(cid);
    sc.stop();

    // Now the cid should be the new one.
    srs_cond_timedwait(ch.exited, 100 * SRS_UTIME_MILLISECONDS);
    EXPECT_TRUE(sc.cid().compare(cid) == 0);
    EXPECT_TRUE(ch.cid.compare(cid) == 0);
}

VOID TEST(AppCoroutineTest, StartStop)
{
    if (true) {
        MockCoroutineHandler ch;
        SrsSTCoroutine sc("test", &ch);
        ch.trd = &sc;
        EXPECT_TRUE(sc.cid().empty());

        // Thread stop after created.
        sc.stop();

        EXPECT_TRUE(sc.cid().empty());

        srs_error_t err = sc.pull();
        EXPECT_TRUE(srs_success != err);
        EXPECT_TRUE(ERROR_THREAD_TERMINATED == srs_error_code(err));
        srs_freep(err);

        // Should never reuse a disposed thread.
        err = sc.start();
        EXPECT_TRUE(srs_success != err);
        EXPECT_TRUE(ERROR_THREAD_DISPOSED == srs_error_code(err));
        srs_freep(err);
    }

    if (true) {
        MockCoroutineHandler ch;
        SrsSTCoroutine sc("test", &ch);
        ch.trd = &sc;
        EXPECT_TRUE(sc.cid().empty());

        EXPECT_TRUE(srs_success == sc.start());
        EXPECT_TRUE(srs_success == sc.pull());

        srs_cond_timedwait(ch.running, 100 * SRS_UTIME_MILLISECONDS);
        EXPECT_TRUE(!sc.cid().empty());

        // Thread stop after started.
        sc.stop();

        srs_error_t err = sc.pull();
        EXPECT_TRUE(srs_success != err);
        EXPECT_TRUE(ERROR_THREAD_INTERRUPED == srs_error_code(err));
        srs_freep(err);

        // Should never reuse a disposed thread.
        err = sc.start();
        EXPECT_TRUE(srs_success != err);
        EXPECT_TRUE(ERROR_THREAD_DISPOSED == srs_error_code(err));
        srs_freep(err);
    }

    if (true) {
        MockCoroutineHandler ch;
        SrsSTCoroutine sc("test", &ch);
        ch.trd = &sc;
        EXPECT_TRUE(sc.cid().empty());

        EXPECT_TRUE(srs_success == sc.start());
        EXPECT_TRUE(srs_success == sc.pull());

        // Error when start multiple times.
        srs_error_t err = sc.start();
        EXPECT_TRUE(srs_success != err);
        EXPECT_TRUE(ERROR_THREAD_STARTED == srs_error_code(err));
        srs_freep(err);

        err = sc.pull();
        EXPECT_TRUE(srs_success != err);
        EXPECT_TRUE(ERROR_THREAD_STARTED == srs_error_code(err));
        srs_freep(err);
    }
}

VOID TEST(AppCoroutineTest, StopChildWhenCallerInterrupted)
{
    srs_error_t err = srs_success;

    MockInterruptedJoinHandler ch;
    SrsSTCoroutine sc("parent", &ch);

    HELPER_ASSERT_SUCCESS(sc.start());

    if (!ch.stopped) {
        srs_cond_timedwait(ch.stopped_cond, 1 * SRS_UTIME_SECONDS);
    }
    EXPECT_TRUE(ch.stopped);

    sc.stop();

    err = sc.pull();
    HELPER_EXPECT_SUCCESS(err);
}

// An interrupt pending on the coroutine, for example a client kicked off by the HTTP API, fails the mutex lock with
// EINTR. The locker takes the lock anyway, instead of aborting the server. Backported from upstream v7.0.213.
VOID TEST(AppCoroutineTest, LockerWhenInterrupted)
{
    srs_mutex_t lock = srs_mutex_new();

    srs_thread_interrupt(srs_thread_self());
    if (true) {
        SrsLocker(lock);
        // The lock is held, so taking it again fails with EDEADLK.
        EXPECT_NE(0, srs_mutex_lock(lock));
        EXPECT_EQ(EDEADLK, errno);
    }

    // The locker released it.
    EXPECT_EQ(0, srs_mutex_lock(lock));
    EXPECT_EQ(0, srs_mutex_unlock(lock));

    srs_mutex_destroy(lock);
}

// The State Threads clock is monotonic, so a wall clock step, such as by NTP, never fires the timeouts early nor
// stalls the timers. Backported from upstream v7.0.17.
VOID TEST(AppCoroutineTest, StClockIsMonotonic)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    int64_t mono = ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t wall = tv.tv_sec * 1000000LL + tv.tv_usec;

    int64_t now = (int64_t)st_utime();

    // Follows the monotonic clock, within a second.
    EXPECT_LT(llabs(now - mono), 1000000LL);
    // Not the wall clock, which counts from the epoch, years more than the uptime.
    EXPECT_GT(llabs(now - wall), 365LL * 24 * 3600 * 1000000LL);
}

VOID TEST(AppCoroutineTest, Cycle)
{
    if (true) {
        MockCoroutineHandler ch;
        SrsSTCoroutine sc("test", &ch);
        ch.trd = &sc;

        EXPECT_TRUE(srs_success == sc.start());
        EXPECT_TRUE(srs_success == sc.pull());

        // Set cycle to error.
        ch.err = srs_error_new(-1, "cycle");

        srs_cond_timedwait(ch.running, 100 * SRS_UTIME_MILLISECONDS);

        // The cycle error should be pulled.
        srs_error_t err = sc.pull();
        EXPECT_TRUE(srs_success != err);
        EXPECT_TRUE(-1 == srs_error_code(err));
        srs_freep(err);
    }

    if (true) {
        MockCoroutineHandler ch;
        SrsContextId cid;
        SrsSTCoroutine sc("test", &ch, cid.set_value("250"));
        ch.trd = &sc;
        EXPECT_TRUE(!sc.cid().compare(cid));

        EXPECT_TRUE(srs_success == sc.start());
        EXPECT_TRUE(srs_success == sc.pull());

        // After running, the cid in cycle should equal to the thread.
        srs_cond_timedwait(ch.running, 100 * SRS_UTIME_MILLISECONDS);
        EXPECT_TRUE(!ch.cid.compare(cid));
    }

    if (true) {
        MockCoroutineHandler ch;
        SrsSTCoroutine sc("test", &ch);
        ch.trd = &sc;

        EXPECT_TRUE(srs_success == sc.start());
        EXPECT_TRUE(srs_success == sc.pull());

        srs_cond_timedwait(ch.running, 100 * SRS_UTIME_MILLISECONDS);

        // Interrupt thread, set err to interrupted.
        sc.interrupt();

        // Set cycle to error.
        ch.err = srs_error_new(-1, "cycle");

        // When thread terminated, thread will get its error.
        srs_cond_timedwait(ch.exited, 100 * SRS_UTIME_MILLISECONDS);

        // Override the error by cycle error.
        sc.stop();

        // Should be cycle error.
        srs_error_t err = sc.pull();
        EXPECT_TRUE(srs_success != err);
        EXPECT_TRUE(-1 == srs_error_code(err));
        srs_freep(err);
    }

    if (true) {
        MockCoroutineHandler ch;
        SrsSTCoroutine sc("test", &ch);
        ch.trd = &sc;

        EXPECT_TRUE(srs_success == sc.start());
        EXPECT_TRUE(srs_success == sc.pull());

        // Quit without error.
        ch.quit = true;

        // Wait for thread to done.
        srs_cond_timedwait(ch.exited, 100 * SRS_UTIME_MILLISECONDS);

        // Override the error by cycle error.
        sc.stop();

        // Should be cycle error.
        srs_error_t err = sc.pull();
        EXPECT_TRUE(srs_success == err);
        srs_freep(err);
    }
}

void* mock_st_thread_create(void *(*/*start*/)(void *arg), void */*arg*/, int /*joinable*/, int /*stack_size*/) {
    return NULL;
}

VOID TEST(AppCoroutineTest, StartThread)
{
    MockCoroutineHandler ch;
    SrsSTCoroutine sc("test", &ch);
    ch.trd = &sc;

    _ST_THREAD_CREATE_PFN ov = _pfn_st_thread_create;
    _pfn_st_thread_create = (_ST_THREAD_CREATE_PFN)mock_st_thread_create;

    srs_error_t err = sc.start();
    _pfn_st_thread_create = ov;

    EXPECT_TRUE(srs_success != err);
    EXPECT_TRUE(ERROR_ST_CREATE_CYCLE_THREAD == srs_error_code(err));
    srs_freep(err);
}

VOID TEST(AppFragmentTest, CheckDuration)
{
	if (true) {
		SrsFragment frg;
		EXPECT_EQ(-1, frg.start_dts);
		EXPECT_EQ(0, frg.dur);
		EXPECT_FALSE(frg.sequence_header);
	}

	if (true) {
		SrsFragment frg;

		frg.append(0);
		EXPECT_EQ(0, frg.duration());

		frg.append(10);
		EXPECT_EQ(10 * SRS_UTIME_MILLISECONDS, frg.duration());

		frg.append(99);
		EXPECT_EQ(99 * SRS_UTIME_MILLISECONDS, frg.duration());

		frg.append(0x7fffffffLL);
		EXPECT_EQ(0x7fffffffLL * SRS_UTIME_MILLISECONDS, frg.duration());

		frg.append(0xffffffffLL);
		EXPECT_EQ(0xffffffffLL * SRS_UTIME_MILLISECONDS, frg.duration());

		frg.append(0x20c49ba5e353f7LL);
		EXPECT_EQ(0x20c49ba5e353f7LL * SRS_UTIME_MILLISECONDS, frg.duration());
	}

	if (true) {
		SrsFragment frg;

		frg.append(0);
		EXPECT_EQ(0, frg.duration());

		frg.append(0x7fffffffffffffffLL);
		EXPECT_EQ(0, frg.duration());
	}

	if (true) {
		SrsFragment frg;

		frg.append(100);
		EXPECT_EQ(0, frg.duration());

		frg.append(10);
		EXPECT_EQ(0, frg.duration());

		frg.append(100);
		EXPECT_EQ(90 * SRS_UTIME_MILLISECONDS, frg.duration());
	}

	if (true) {
		SrsFragment frg;

		frg.append(-10);
		EXPECT_EQ(0, frg.duration());

		frg.append(-5);
		EXPECT_EQ(0, frg.duration());

		frg.append(10);
		EXPECT_EQ(10 * SRS_UTIME_MILLISECONDS, frg.duration());
	}
}

VOID TEST(AppSecurity, CheckSecurity)
{
    srs_error_t err;

    // Deny if no rules.
    if (true) {
        SrsSecurity sec; SrsRequest rr;
        HELPER_EXPECT_FAILED(sec.do_check(NULL, SrsRtmpConnUnknown, "", &rr));
    }

    // Deny if not allowed.
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnUnknown, "", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("others"); rules.get_or_create("any");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnUnknown, "", &rr));
    }

    // Deny by rule.
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "all");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnPlay, "", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "12.13.14.15");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "11.12.13.14");
        if (true) {
            SrsConfDirective* d = new SrsConfDirective();
            d->name = "deny";
            d->args.push_back("play");
            d->args.push_back("12.13.14.15");
            rules.directives.push_back(d);
        }
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "all");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtcConnPlay, "", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "12.13.14.15");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtcConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "11.12.13.14");
        if (true) {
            SrsConfDirective* d = new SrsConfDirective();
            d->name = "deny";
            d->args.push_back("play");
            d->args.push_back("12.13.14.15");
            rules.directives.push_back(d);
        }
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtcConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "publish", "12.13.14.15");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnFMLEPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "publish", "12.13.14.15");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnFlashPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "publish", "all");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnFlashPublish, "11.12.13.14", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "publish", "12.13.14.15");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnHaivisionPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "publish", "12.13.14.15");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtcConnPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "publish", "all");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtcConnPublish, "11.12.13.14", &rr));
    }

    // Allowed if not denied.
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "all");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnFMLEPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnFMLEPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnPlay, "11.12.13.14", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "publish", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "publish", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnUnknown, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "publish", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnFlashPublish, "11.12.13.14", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtcConnPlay, "11.12.13.14", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "publish", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtcConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "all");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtcConnPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtcConnPublish, "12.13.14.15", &rr));
    }

    // Allowed by rule.
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "play", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "play", "all");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "play", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtcConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "play", "all");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtcConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "publish", "all");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtcConnPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "publish", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtcConnPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "publish", "all");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnFMLEPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "publish", "all");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnFlashPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "publish", "all");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnHaivisionPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "publish", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnHaivisionPublish, "12.13.14.15", &rr));
    }

    // Allowed if not denied.
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "12.13.14.15");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnFMLEPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("deny", "play", "all");
        HELPER_EXPECT_SUCCESS(sec.do_check(&rules, SrsRtmpConnFMLEPublish, "12.13.14.15", &rr));
    }

    // Denied if not allowd.
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "play", "11.12.13.14");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnFMLEPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "play", "11.12.13.14");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "publish", "12.13.14.15");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "play", "11.12.13.14");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtcConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "publish", "12.13.14.15");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtcConnPlay, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "publish", "11.12.13.14");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnHaivisionPublish, "12.13.14.15", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "publish", "11.12.13.14");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnUnknown, "11.12.13.14", &rr));
    }

    // Denied if dup.
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "play", "11.12.13.14");
        rules.get_or_create("deny", "play", "11.12.13.14");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtmpConnPlay, "11.12.13.14", &rr));
    }
    if (true) {
        SrsSecurity sec; SrsRequest rr; SrsConfDirective rules;
        rules.get_or_create("allow", "play", "11.12.13.14");
        rules.get_or_create("deny", "play", "11.12.13.14");
        HELPER_EXPECT_FAILED(sec.do_check(&rules, SrsRtcConnPlay, "11.12.13.14", &rr));
    }

    // SRS apply the following simple strategies one by one:
    //       1. allow all if security disabled.
    //       2. default to deny all when security enabled.
    //       3. allow if matches allow strategy.
    //       4. deny if matches deny strategy.
}

// Checks a client of the given type with the app and stream a protocol parsed against the enhanced security of the
// default vhost, which has no security rules, as every protocol does when a client joins, and returns the error code,
// 0 when the client is allowed.
static int security_utest_check_stream(SrsRtmpConnType type, SrsRequest* req)
{
    SrsEnhancedSecurity enhanced;
    req->vhost = "__defaultVhost__";

    srs_error_t err = enhanced.check(type, "127.0.0.1", req);
    int code = srs_error_code(err);
    srs_freep(err);
    return code;
}

// A server with no security rules, and an RTMP encoder publishing to rtmp://host/live/../../../../../../tmp/pwn: the
// app has a dot-dot that would put its HLS files outside hls_path, so the encoder is refused, and so is a player of
// it. A nested app of tenant/live still publishes, while a name with a dot-dot inside a segment is refused too.
VOID TEST(AppSecurity, RtmpDotDotStream_Refused)
{
    if (true) {
        SrsRequest req;
        req.tcUrl = "rtmp://127.0.0.1/live/../../../../../../tmp";
        req.stream = "pwn";
        srs_discovery_tc_url(req.tcUrl, req.schema, req.host, req.vhost, req.app, req.stream, req.port, req.param);
        req.strip();
        EXPECT_STREQ("live/../../../../../../tmp", req.app.c_str());

        EXPECT_EQ(ERROR_SYSTEM_SECURITY_NAME, security_utest_check_stream(SrsRtmpConnFMLEPublish, &req));
        EXPECT_EQ(ERROR_SYSTEM_SECURITY_NAME, security_utest_check_stream(SrsRtmpConnPlay, &req));
    }

    if (true) {
        SrsRequest req;
        req.tcUrl = "rtmp://127.0.0.1/live";
        req.stream = "../../../etc/pwn";
        srs_discovery_tc_url(req.tcUrl, req.schema, req.host, req.vhost, req.app, req.stream, req.port, req.param);
        req.strip();

        EXPECT_EQ(ERROR_SYSTEM_SECURITY_NAME, security_utest_check_stream(SrsRtmpConnFMLEPublish, &req));
    }

    if (true) {
        SrsRequest req;
        req.app = "live";
        req.stream = "show..1";
        EXPECT_EQ(ERROR_SYSTEM_SECURITY_NAME, security_utest_check_stream(SrsRtmpConnFMLEPublish, &req));
    }

    if (true) {
        SrsRequest req;
        req.tcUrl = "rtmp://127.0.0.1/tenant/live";
        req.stream = "livestream";
        srs_discovery_tc_url(req.tcUrl, req.schema, req.host, req.vhost, req.app, req.stream, req.port, req.param);
        req.strip();

        EXPECT_EQ(0, security_utest_check_stream(SrsRtmpConnFMLEPublish, &req));
    }
}

// A WHIP publisher of /rtc/v1/whip/?app=live&stream=../../../tmp/pwn, whose query the handler takes as given and
// splits on the last slash into the app live/../../../tmp and the stream pwn, is refused, as is a WHEP player of it.
VOID TEST(AppSecurity, WhipDotDotQuery_Refused)
{
    SrsRequest req;
    req.app = "live/../../../tmp";
    req.stream = "pwn";
    EXPECT_EQ(ERROR_SYSTEM_SECURITY_NAME, security_utest_check_stream(SrsRtcConnPublish, &req));
    EXPECT_EQ(ERROR_SYSTEM_SECURITY_NAME, security_utest_check_stream(SrsRtcConnPlay, &req));

    req.app = "..";
    req.stream = "pwn";
    EXPECT_EQ(ERROR_SYSTEM_SECURITY_NAME, security_utest_check_stream(SrsRtcConnPublish, &req));
}

class MockVhostSecurity : public SrsSecurity
{
public:
    int check_count_;
    SrsRtmpConnType check_type_;
    srs_error_t check_error_;
public:
    MockVhostSecurity() {
        check_count_ = 0;
        check_type_ = SrsRtmpConnUnknown;
        check_error_ = srs_success;
    }
    virtual ~MockVhostSecurity() {
        srs_freep(check_error_);
    }
public:
    virtual srs_error_t check(SrsRtmpConnType type, std::string ip, SrsRequest* req) {
        check_count_++;
        check_type_ = type;
        return srs_error_copy(check_error_);
    }
};

// The system security comes before the vhost security: a client with a dot-dot name is refused without the vhost
// rules being asked, even rules that would allow it, while a client of a valid name gets the verdict of the vhost
// rules, a deny or an allow.
VOID TEST(AppSecurity, SystemSecurityBeforeVhostSecurity)
{
    MockVhostSecurity* vhost = new MockVhostSecurity();
    SrsEnhancedSecurity enhanced;
    srs_freep(enhanced.security_);
    enhanced.security_ = vhost;

    SrsRequest req;
    req.app = "live";
    req.stream = "../pwn";
    srs_error_t err = enhanced.check(SrsRtmpConnFMLEPublish, "127.0.0.1", &req);
    EXPECT_EQ(ERROR_SYSTEM_SECURITY_NAME, srs_error_code(err));
    srs_freep(err);
    EXPECT_EQ(0, vhost->check_count_);

    req.stream = "livestream";
    vhost->check_error_ = srs_error_new(ERROR_SYSTEM_SECURITY_DENY, "mock deny");
    err = enhanced.check(SrsRtmpConnFMLEPublish, "127.0.0.1", &req);
    EXPECT_EQ(ERROR_SYSTEM_SECURITY_DENY, srs_error_code(err));
    srs_freep(err);
    EXPECT_EQ(1, vhost->check_count_);
    EXPECT_EQ(SrsRtmpConnFMLEPublish, vhost->check_type_);

    srs_freep(vhost->check_error_);
    err = enhanced.check(SrsRtmpConnPlay, "127.0.0.1", &req);
    EXPECT_TRUE(err == srs_success);
    srs_freep(err);
    EXPECT_EQ(2, vhost->check_count_);
}


// A new source is idle since it was created, not since the epoch, so the cleanup timer never drops it before its
// publisher activates it. Backported from upstream v7.0.127.
VOID TEST(AppLiveSourceTest, NewSourceIsNotDeadAtOnce)
{
    SrsLiveSource source;
    EXPECT_FALSE(source.stream_is_dead());

    // After the cleanup delay with no publisher nor player, the source is dead.
    source.stream_die_at_ = 0;
    EXPECT_TRUE(source.stream_is_dead());
}

// The source is never dead while a client fetched it and may yield before it publishes or plays, for example in
// the on_publish hook, and it gets the whole cleanup delay when the last pending client leaves.
VOID TEST(AppLiveSourceTest, PendingClientKeepsSourceAlive)
{
    SrsSharedPtr<SrsLiveSource> source(new SrsLiveSource());
    source->stream_die_at_ = 0;
    EXPECT_TRUE(source->stream_is_dead());

    if (true) {
        SrsLiveSourcePendingClient c0(source);
        EXPECT_FALSE(source->stream_is_dead());

        if (true) {
            SrsLiveSourcePendingClient c1(source);
            EXPECT_EQ(2, source->nn_pending_clients_);
        }

        // One client is still pending.
        source->stream_die_at_ = 0;
        EXPECT_FALSE(source->stream_is_dead());
    }

    // The last pending client left, so the source is idle from now on.
    EXPECT_EQ(0, source->nn_pending_clients_);
    EXPECT_FALSE(source->stream_is_dead());

    source->stream_die_at_ = 0;
    EXPECT_TRUE(source->stream_is_dead());

    // An unbalanced detach never makes the counter negative.
    source->detach_pending_client();
    EXPECT_EQ(0, source->nn_pending_clients_);
}

// A publishing source keeps its die time when a pending client leaves, since it is not idle.
VOID TEST(AppLiveSourceTest, PendingClientLeavesPublishingSource)
{
    SrsSharedPtr<SrsLiveSource> source(new SrsLiveSource());
    source->stream_die_at_ = 0;

    if (true) {
        SrsLiveSourcePendingClient c0(source);
        source->can_publish_ = false;
    }

    EXPECT_EQ(0, source->stream_die_at_);
    EXPECT_FALSE(source->stream_is_dead());
}
