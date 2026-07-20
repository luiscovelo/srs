// The MIT License (MIT)
//
// # Copyright (c) 2025 Winlin
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
package blackbox

import (
	"context"
	"fmt"
	"github.com/ossrs/go-oryx-lib/errors"
	"github.com/ossrs/go-oryx-lib/logger"
	srsbench "github.com/ossrs/srs-bench/srs"
	"math/rand"
	"os"
	"os/exec"
	"path"
	"runtime"
	"strings"
	"sync"
	"testing"
	"time"
)

func TestSlow_RtmpPublish_RtmpPlay_HEVC_Basic(t *testing.T) {
	// This case is run in parallel.
	t.Parallel()

	// Setup the max timeout for this case.
	ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
	defer cancel()

	// Only enable for github actions, ignore for darwin.
	if runtime.GOOS == "darwin" {
		logger.Tf(ctx, "Depends on FFmpeg(HEVC over RTMP), only available for GitHub actions")
		return
	}

	// Check a set of errors.
	var r0, r1, r2, r3, r4, r5, r6, r7 error
	defer func(ctx context.Context) {
		if err := filterTestError(ctx.Err(), r0, r1, r2, r3, r4, r5, r6, r7); err != nil {
			t.Errorf("Fail for err %+v", err)
		} else {
			logger.Tf(ctx, "test done with err %+v", err)
		}
	}(ctx)

	var wg sync.WaitGroup
	defer wg.Wait()

	// Start hooks service.
	hooks := NewHooksService(hooksOnPublishFlags(false, true))
	wg.Add(1)
	go func() {
		defer wg.Done()
		r7 = hooks.Run(ctx, cancel)
	}()

	// Start SRS server and wait for it to be ready.
	svr := NewSRSServer(func(v *srsServer) {
		v.envs = []string{
			"SRS_VHOST_HTTP_HOOKS_ENABLED=on",
			fmt.Sprintf("SRS_VHOST_HTTP_HOOKS_ON_PUBLISH=http://localhost:%v/api/v1/publish", hooks.HooksAPI()),
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-hooks.ReadyCtx().Done()
		r0 = svr.Run(ctx, cancel)
	}()

	// Start FFmpeg to publish stream.
	streamID := fmt.Sprintf("stream-%v-%v", os.Getpid(), rand.Int())
	streamURL := fmt.Sprintf("rtmp://localhost:%v/live/%v", svr.RTMPPort(), streamID)
	ffmpeg := NewFFmpeg(func(v *ffmpegClient) {
		v.args = []string{
			// Use the fastest preset of x265, see https://x265.readthedocs.io/en/master/presets.html
			"-stream_loop", "-1", "-re", "-i", *srsPublishAvatar, "-acodec", "copy", "-vcodec", "libx265",
			"-profile:v", "main", "-preset", "ultrafast", "-f", "flv", streamURL,
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()
		r1 = ffmpeg.Run(ctx, cancel)
	}()

	// Start FFprobe to detect and verify stream.
	duration := time.Duration(*srsFFprobeDuration) * time.Millisecond
	ffprobe := NewFFprobe(func(v *ffprobeClient) {
		v.dvrFile = path.Join(svr.WorkDir(), "objs", fmt.Sprintf("srs-ffprobe-%v.ts", streamID))
		v.streamURL = fmt.Sprintf("rtmp://localhost:%v/live/%v", svr.RTMPPort(), streamID)
		v.duration, v.timeout = duration, time.Duration(*srsFFprobeTimeout)*time.Millisecond
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()
		r2 = ffprobe.Run(ctx, cancel)
	}()

	// Fast quit for probe done.
	select {
	case <-ctx.Done():
	case <-ffprobe.ProbeDoneCtx().Done():
		defer cancel()

		str, m := ffprobe.Result()
		if len(m.Streams) != 2 {
			r3 = errors.Errorf("invalid streams=%v, %v, %v", len(m.Streams), m.String(), str)
		}

		// Note that HLS score is low, so we only check duration.
		if dv := m.Duration(); dv < duration {
			r5 = errors.Errorf("short duration=%v < %v, %v, %v", dv, duration, m.String(), str)
		}

		if v := m.Video(); v == nil {
			r5 = errors.Errorf("no video %v, %v", m.String(), str)
		} else if v.CodecName != "hevc" {
			r6 = errors.Errorf("invalid video codec=%v, %v, %v", v.CodecName, m.String(), str)
		}
	}
}

func TestSlow_RtmpPublish_HttpFlvPlay_HEVC_Basic(t *testing.T) {
	// This case is run in parallel.
	t.Parallel()

	// Setup the max timeout for this case.
	ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
	defer cancel()

	// Only enable for github actions, ignore for darwin.
	if runtime.GOOS == "darwin" {
		logger.Tf(ctx, "Depends on FFmpeg(HEVC over RTMP), only available for GitHub actions")
		return
	}

	// Check a set of errors.
	var r0, r1, r2, r3, r4, r5, r6, r7 error
	defer func(ctx context.Context) {
		if err := filterTestError(ctx.Err(), r0, r1, r2, r3, r4, r5, r6, r7); err != nil {
			t.Errorf("Fail for err %+v", err)
		} else {
			logger.Tf(ctx, "test done with err %+v", err)
		}
	}(ctx)

	var wg sync.WaitGroup
	defer wg.Wait()

	// Start hooks service.
	hooks := NewHooksService(hooksOnPublishFlags(false, true))
	wg.Add(1)
	go func() {
		defer wg.Done()
		r7 = hooks.Run(ctx, cancel)
	}()

	// Start SRS server and wait for it to be ready.
	svr := NewSRSServer(func(v *srsServer) {
		v.envs = []string{
			"SRS_HTTP_SERVER_ENABLED=on",
			"SRS_VHOST_HTTP_REMUX_ENABLED=on",
			// If guessing, we might got no audio because transcoding might be delay for sending audio packets.
			"SRS_VHOST_HTTP_REMUX_GUESS_HAS_AV=off",
			"SRS_VHOST_HTTP_HOOKS_ENABLED=on",
			fmt.Sprintf("SRS_VHOST_HTTP_HOOKS_ON_PUBLISH=http://localhost:%v/api/v1/publish", hooks.HooksAPI()),
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-hooks.ReadyCtx().Done()
		r0 = svr.Run(ctx, cancel)
	}()

	// Start FFmpeg to publish stream.
	streamID := fmt.Sprintf("stream-%v-%v", os.Getpid(), rand.Int())
	streamURL := fmt.Sprintf("rtmp://localhost:%v/live/%v", svr.RTMPPort(), streamID)
	ffmpeg := NewFFmpeg(func(v *ffmpegClient) {
		v.args = []string{
			// Use the fastest preset of x265, see https://x265.readthedocs.io/en/master/presets.html
			"-stream_loop", "-1", "-re", "-i", *srsPublishAvatar, "-acodec", "copy", "-vcodec", "libx265",
			"-profile:v", "main", "-preset", "ultrafast", "-f", "flv", streamURL,
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()
		r1 = ffmpeg.Run(ctx, cancel)
	}()

	// Start FFprobe to detect and verify stream.
	duration := time.Duration(*srsFFprobeDuration) * time.Millisecond
	ffprobe := NewFFprobe(func(v *ffprobeClient) {
		v.dvrFile = path.Join(svr.WorkDir(), "objs", fmt.Sprintf("srs-ffprobe-%v.ts", streamID))
		v.streamURL = fmt.Sprintf("http://localhost:%v/live/%v.flv", svr.HTTPPort(), streamID)
		v.duration, v.timeout = duration, time.Duration(*srsFFprobeTimeout)*time.Millisecond
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()
		r2 = ffprobe.Run(ctx, cancel)
	}()

	// Fast quit for probe done.
	select {
	case <-ctx.Done():
	case <-ffprobe.ProbeDoneCtx().Done():
		defer cancel()

		str, m := ffprobe.Result()
		if len(m.Streams) != 2 {
			r3 = errors.Errorf("invalid streams=%v, %v, %v", len(m.Streams), m.String(), str)
		}

		// Note that HLS score is low, so we only check duration.
		if dv := m.Duration(); dv < duration {
			r5 = errors.Errorf("short duration=%v < %v, %v, %v", dv, duration, m.String(), str)
		}

		if v := m.Video(); v == nil {
			r5 = errors.Errorf("no video %v, %v", m.String(), str)
		} else if v.CodecName != "hevc" {
			r6 = errors.Errorf("invalid video codec=%v, %v, %v", v.CodecName, m.String(), str)
		}
	}
}

func TestSlow_RtmpPublish_HttpTsPlay_HEVC_Basic(t *testing.T) {
	// This case is run in parallel.
	t.Parallel()

	// Setup the max timeout for this case.
	ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
	defer cancel()

	// Only enable for github actions, ignore for darwin.
	if runtime.GOOS == "darwin" {
		logger.Tf(ctx, "Depends on FFmpeg(HEVC over RTMP), only available for GitHub actions")
		return
	}

	// Check a set of errors.
	var r0, r1, r2, r3, r4, r5, r6, r7 error
	defer func(ctx context.Context) {
		if err := filterTestError(ctx.Err(), r0, r1, r2, r3, r4, r5, r6, r7); err != nil {
			t.Errorf("Fail for err %+v", err)
		} else {
			logger.Tf(ctx, "test done with err %+v", err)
		}
	}(ctx)

	var wg sync.WaitGroup
	defer wg.Wait()

	// Start hooks service.
	hooks := NewHooksService(hooksOnPublishFlags(false, true))
	wg.Add(1)
	go func() {
		defer wg.Done()
		r7 = hooks.Run(ctx, cancel)
	}()

	// Start SRS server and wait for it to be ready.
	svr := NewSRSServer(func(v *srsServer) {
		v.envs = []string{
			"SRS_HTTP_SERVER_ENABLED=on",
			"SRS_VHOST_HTTP_REMUX_ENABLED=on",
			"SRS_VHOST_HTTP_REMUX_MOUNT=[vhost]/[app]/[stream].ts",
			"SRS_VHOST_HTTP_HOOKS_ENABLED=on",
			fmt.Sprintf("SRS_VHOST_HTTP_HOOKS_ON_PUBLISH=http://localhost:%v/api/v1/publish", hooks.HooksAPI()),
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-hooks.ReadyCtx().Done()
		r0 = svr.Run(ctx, cancel)
	}()

	// Start FFmpeg to publish stream.
	streamID := fmt.Sprintf("stream-%v-%v", os.Getpid(), rand.Int())
	streamURL := fmt.Sprintf("rtmp://localhost:%v/live/%v", svr.RTMPPort(), streamID)
	ffmpeg := NewFFmpeg(func(v *ffmpegClient) {
		v.args = []string{
			// Use the fastest preset of x265, see https://x265.readthedocs.io/en/master/presets.html
			"-stream_loop", "-1", "-re", "-i", *srsPublishAvatar, "-acodec", "copy", "-vcodec", "libx265",
			"-profile:v", "main", "-preset", "ultrafast", "-f", "flv", streamURL,
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()
		r1 = ffmpeg.Run(ctx, cancel)
	}()

	// Start FFprobe to detect and verify stream.
	duration := time.Duration(*srsFFprobeDuration) * time.Millisecond
	ffprobe := NewFFprobe(func(v *ffprobeClient) {
		v.dvrFile = path.Join(svr.WorkDir(), "objs", fmt.Sprintf("srs-ffprobe-%v.ts", streamID))
		v.streamURL = fmt.Sprintf("http://localhost:%v/live/%v.ts", svr.HTTPPort(), streamID)
		v.duration, v.timeout = duration, time.Duration(*srsFFprobeTimeout)*time.Millisecond
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()
		r2 = ffprobe.Run(ctx, cancel)
	}()

	// Fast quit for probe done.
	select {
	case <-ctx.Done():
	case <-ffprobe.ProbeDoneCtx().Done():
		defer cancel()

		str, m := ffprobe.Result()
		if len(m.Streams) != 2 {
			r3 = errors.Errorf("invalid streams=%v, %v, %v", len(m.Streams), m.String(), str)
		}

		// Note that HLS score is low, so we only check duration.
		if dv := m.Duration(); dv < duration {
			r5 = errors.Errorf("short duration=%v < %v, %v, %v", dv, duration, m.String(), str)
		}

		if v := m.Video(); v == nil {
			r5 = errors.Errorf("no video %v, %v", m.String(), str)
		} else if v.CodecName != "hevc" {
			r6 = errors.Errorf("invalid video codec=%v, %v, %v", v.CodecName, m.String(), str)
		}
	}
}

func TestSlow_RtmpPublish_HlsPlay_HEVC_Basic(t *testing.T) {
	// This case is run in parallel.
	t.Parallel()

	// Setup the max timeout for this case.
	ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
	defer cancel()

	// Only enable for github actions, ignore for darwin.
	if runtime.GOOS == "darwin" {
		logger.Tf(ctx, "Depends on FFmpeg(HEVC over RTMP), only available for GitHub actions")
		return
	}

	// Check a set of errors.
	var r0, r1, r2, r3, r4, r5, r6, r7 error
	defer func(ctx context.Context) {
		if err := filterTestError(ctx.Err(), r0, r1, r2, r3, r4, r5, r6, r7); err != nil {
			t.Errorf("Fail for err %+v", err)
		} else {
			logger.Tf(ctx, "test done with err %+v", err)
		}
	}(ctx)

	var wg sync.WaitGroup
	defer wg.Wait()

	// Start hooks service.
	hooks := NewHooksService(hooksOnPublishFlags(false, true))
	wg.Add(1)
	go func() {
		defer wg.Done()
		r7 = hooks.Run(ctx, cancel)
	}()

	// Start SRS server and wait for it to be ready.
	svr := NewSRSServer(func(v *srsServer) {
		v.envs = []string{
			"SRS_HTTP_SERVER_ENABLED=on",
			"SRS_VHOST_HLS_ENABLED=on",
			"SRS_VHOST_HTTP_HOOKS_ENABLED=on",
			fmt.Sprintf("SRS_VHOST_HTTP_HOOKS_ON_PUBLISH=http://localhost:%v/api/v1/publish", hooks.HooksAPI()),
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-hooks.ReadyCtx().Done()
		r0 = svr.Run(ctx, cancel)
	}()

	// Start FFmpeg to publish stream.
	streamID := fmt.Sprintf("stream-%v-%v", os.Getpid(), rand.Int())
	streamURL := fmt.Sprintf("rtmp://localhost:%v/live/%v", svr.RTMPPort(), streamID)
	ffmpeg := NewFFmpeg(func(v *ffmpegClient) {
		v.args = []string{
			// Use the fastest preset of x265, see https://x265.readthedocs.io/en/master/presets.html
			"-stream_loop", "-1", "-re", "-i", *srsPublishAvatar, "-acodec", "copy", "-vcodec", "libx265",
			"-profile:v", "main", "-preset", "ultrafast", "-f", "flv", streamURL,
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()
		r1 = ffmpeg.Run(ctx, cancel)
	}()

	// Start FFprobe to detect and verify stream.
	duration := time.Duration(*srsFFprobeDuration) * time.Millisecond * 2
	ffprobe := NewFFprobe(func(v *ffprobeClient) {
		v.dvrFile = path.Join(svr.WorkDir(), "objs", fmt.Sprintf("srs-ffprobe-%v.ts", streamID))
		v.streamURL = fmt.Sprintf("http://localhost:%v/live/%v.m3u8", svr.HTTPPort(), streamID)
		v.duration, v.timeout = duration, time.Duration(*srsFFprobeTimeout)*time.Millisecond*2
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()
		r2 = ffprobe.Run(ctx, cancel)
	}()

	// Fast quit for probe done.
	select {
	case <-ctx.Done():
	case <-ffprobe.ProbeDoneCtx().Done():
		defer cancel()

		str, m := ffprobe.Result()
		if len(m.Streams) != 2 {
			r3 = errors.Errorf("invalid streams=%v, %v, %v", len(m.Streams), m.String(), str)
		}

		// Note that HLS score is low, so we only check duration. Note that only check part of duration, because we
		// might get only some pieces of segments.
		if dv := m.Duration(); dv < duration/3 {
			r4 = errors.Errorf("short duration=%v < %v, %v, %v", dv, duration/3, m.String(), str)
		}

		if v := m.Video(); v == nil {
			r5 = errors.Errorf("no video %v, %v", m.String(), str)
		} else if v.CodecName != "hevc" {
			r6 = errors.Errorf("invalid video codec=%v, %v, %v", v.CodecName, m.String(), str)
		}
	}
}

func TestSlow_RtmpPublish_DvrFlv_HEVC_Basic(t *testing.T) {
	// This case is run in parallel.
	t.Parallel()

	// Setup the max timeout for this case.
	ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
	defer cancel()

	// Check a set of errors.
	var r0, r1, r2, r3, r4, r5, r6 error
	defer func(ctx context.Context) {
		if err := filterTestError(ctx.Err(), r0, r1, r2, r3, r4, r5, r6); err != nil {
			t.Errorf("Fail for err %+v", err)
		} else {
			logger.Tf(ctx, "test done with err %+v", err)
		}
	}(ctx)

	var wg sync.WaitGroup
	defer wg.Wait()

	// Start hooks service.
	hooks := NewHooksService(hooksOnPublishFlags(false, true))
	wg.Add(1)
	go func() {
		defer wg.Done()
		r6 = hooks.Run(ctx, cancel)
	}()

	// Start SRS server and wait for it to be ready.
	svr := NewSRSServer(func(v *srsServer) {
		v.envs = []string{
			"SRS_VHOST_DVR_ENABLED=on",
			"SRS_VHOST_DVR_DVR_PLAN=session",
			"SRS_VHOST_DVR_DVR_PATH=./objs/nginx/html/[app]/[stream].[timestamp].flv",
			fmt.Sprintf("SRS_VHOST_DVR_DVR_DURATION=%v", *srsFFprobeDuration),
			"SRS_VHOST_HTTP_HOOKS_ENABLED=on",
			fmt.Sprintf("SRS_VHOST_HTTP_HOOKS_ON_PUBLISH=http://localhost:%v/api/v1/publish", hooks.HooksAPI()),
			fmt.Sprintf("SRS_VHOST_HTTP_HOOKS_ON_DVR=http://localhost:%v/api/v1/dvrs", hooks.HooksAPI()),
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-hooks.ReadyCtx().Done()
		r0 = svr.Run(ctx, cancel)
	}()

	// Start FFmpeg to publish stream.
	duration := time.Duration(*srsFFprobeDuration) * time.Millisecond
	streamID := fmt.Sprintf("stream-%v-%v", os.Getpid(), rand.Int())
	streamURL := fmt.Sprintf("rtmp://localhost:%v/live/%v", svr.RTMPPort(), streamID)
	ffmpeg := NewFFmpeg(func(v *ffmpegClient) {
		// When process quit, still keep case to run.
		v.cancelCaseWhenQuit, v.ffmpegDuration = false, duration
		v.args = []string{
			"-stream_loop", "-1", "-re", "-i", *srsPublishAvatar, "-acodec", "copy", "-vcodec", "libx265",
			"-profile:v", "main", "-preset", "ultrafast", "-f", "flv", streamURL,
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()
		r1 = ffmpeg.Run(ctx, cancel)
	}()

	// Start FFprobe to detect and verify stream.
	ffprobe := NewFFprobe(func(v *ffprobeClient) {
		v.dvrByFFmpeg, v.streamURL = false, streamURL
		v.duration, v.timeout = duration, time.Duration(*srsFFprobeTimeout)*time.Millisecond

		wg.Add(1)
		go func() {
			defer wg.Done()
			for evt := range hooks.HooksEvents() {
				if onDvrEvt, ok := evt.(*HooksEventOnDvr); ok {
					fp := path.Join(svr.WorkDir(), onDvrEvt.File)
					logger.Tf(ctx, "FFprobe: Set the dvrFile=%v from callback", fp)
					v.dvrFile = fp
				}
			}
		}()
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()
		r2 = ffprobe.Run(ctx, cancel)
	}()

	// Fast quit for probe done.
	select {
	case <-ctx.Done():
	case <-ffprobe.ProbeDoneCtx().Done():
		defer cancel()

		str, m := ffprobe.Result()
		if len(m.Streams) != 2 {
			r3 = errors.Errorf("invalid streams=%v, %v, %v", len(m.Streams), m.String(), str)
		}

		if ts := 90; m.Format.ProbeScore < ts {
			r4 = errors.Errorf("low score=%v < %v, %v, %v", m.Format.ProbeScore, ts, m.String(), str)
		}
		if dv := m.Duration(); dv < duration/2 {
			r5 = errors.Errorf("short duration=%v < %v, %v, %v", dv, duration/2, m.String(), str)
		}

		if v := m.Video(); v == nil {
			r5 = errors.Errorf("no video %v, %v", m.String(), str)
		} else if v.CodecName != "hevc" {
			r6 = errors.Errorf("invalid video codec=%v, %v, %v", v.CodecName, m.String(), str)
		}
	}
}

func TestSlow_RtmpPublish_DvrMp4_HEVC_Basic(t *testing.T) {
	// This case is run in parallel.
	t.Parallel()

	// Setup the max timeout for this case.
	ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
	defer cancel()

	// Check a set of errors.
	var r0, r1, r2, r3, r4, r5, r6 error
	defer func(ctx context.Context) {
		if err := filterTestError(ctx.Err(), r0, r1, r2, r3, r4, r5, r6); err != nil {
			t.Errorf("Fail for err %+v", err)
		} else {
			logger.Tf(ctx, "test done with err %+v", err)
		}
	}(ctx)

	var wg sync.WaitGroup
	defer wg.Wait()

	// Start hooks service.
	hooks := NewHooksService(hooksOnPublishFlags(false, true))
	wg.Add(1)
	go func() {
		defer wg.Done()
		r6 = hooks.Run(ctx, cancel)
	}()

	// Start SRS server and wait for it to be ready.
	svr := NewSRSServer(func(v *srsServer) {
		v.envs = []string{
			"SRS_VHOST_DVR_ENABLED=on",
			"SRS_VHOST_DVR_DVR_PLAN=session",
			"SRS_VHOST_DVR_DVR_PATH=./objs/nginx/html/[app]/[stream].[timestamp].mp4",
			fmt.Sprintf("SRS_VHOST_DVR_DVR_DURATION=%v", *srsFFprobeDuration),
			"SRS_VHOST_HTTP_HOOKS_ENABLED=on",
			fmt.Sprintf("SRS_VHOST_HTTP_HOOKS_ON_PUBLISH=http://localhost:%v/api/v1/publish", hooks.HooksAPI()),
			fmt.Sprintf("SRS_VHOST_HTTP_HOOKS_ON_DVR=http://localhost:%v/api/v1/dvrs", hooks.HooksAPI()),
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-hooks.ReadyCtx().Done()
		r0 = svr.Run(ctx, cancel)
	}()

	// Start FFmpeg to publish stream.
	duration := time.Duration(*srsFFprobeDuration) * time.Millisecond
	streamID := fmt.Sprintf("stream-%v-%v", os.Getpid(), rand.Int())
	streamURL := fmt.Sprintf("rtmp://localhost:%v/live/%v", svr.RTMPPort(), streamID)
	ffmpeg := NewFFmpeg(func(v *ffmpegClient) {
		// When process quit, still keep case to run.
		v.cancelCaseWhenQuit, v.ffmpegDuration = false, duration
		v.args = []string{
			"-stream_loop", "-1", "-re", "-i", *srsPublishAvatar, "-acodec", "copy", "-vcodec", "libx265",
			"-profile:v", "main", "-preset", "ultrafast", "-f", "flv", streamURL,
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()
		r1 = ffmpeg.Run(ctx, cancel)
	}()

	// Start FFprobe to detect and verify stream.
	ffprobe := NewFFprobe(func(v *ffprobeClient) {
		v.dvrByFFmpeg, v.streamURL = false, streamURL
		v.duration, v.timeout = duration, time.Duration(*srsFFprobeTimeout)*time.Millisecond

		wg.Add(1)
		go func() {
			defer wg.Done()
			for evt := range hooks.HooksEvents() {
				if onDvrEvt, ok := evt.(*HooksEventOnDvr); ok {
					fp := path.Join(svr.WorkDir(), onDvrEvt.File)
					logger.Tf(ctx, "FFprobe: Set the dvrFile=%v from callback", fp)
					v.dvrFile = fp
				}
			}
		}()
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()
		r2 = ffprobe.Run(ctx, cancel)
	}()

	// Fast quit for probe done.
	select {
	case <-ctx.Done():
	case <-ffprobe.ProbeDoneCtx().Done():
		defer cancel()

		str, m := ffprobe.Result()
		if len(m.Streams) != 2 {
			r3 = errors.Errorf("invalid streams=%v, %v, %v", len(m.Streams), m.String(), str)
		}

		if ts := 90; m.Format.ProbeScore < ts {
			r4 = errors.Errorf("low score=%v < %v, %v, %v", m.Format.ProbeScore, ts, m.String(), str)
		}
		if dv := m.Duration(); dv < duration/2 {
			r5 = errors.Errorf("short duration=%v < %v, %v, %v", dv, duration/2, m.String(), str)
		}

		if v := m.Video(); v == nil {
			r5 = errors.Errorf("no video %v, %v", m.String(), str)
		} else if v.CodecName != "hevc" {
			r6 = errors.Errorf("invalid video codec=%v, %v, %v", v.CodecName, m.String(), str)
		}
	}
}

func TestSlow_RtmpPublish_DvrMp4_HEVC_Unsupported(t *testing.T) {
	// This case is run in parallel.
	t.Parallel()

	// Setup the max timeout for this case.
	ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
	defer cancel()

	// Only enable for github actions, ignore for darwin.
	if runtime.GOOS == "darwin" {
		logger.Tf(ctx, "Depends on FFmpeg(HEVC over RTMP), only available for GitHub actions")
		return
	}

	// Check a set of errors.
	var r0, r1, r2, r3 error
	defer func(ctx context.Context) {
		if err := filterTestError(ctx.Err(), r0, r1, r2, r3); err != nil {
			t.Errorf("Fail for err %+v", err)
		} else {
			logger.Tf(ctx, "test done with err %+v", err)
		}
	}(ctx)

	var wg sync.WaitGroup
	defer wg.Wait()

	// Reject HEVC in on_publish and require the empty DVR artifact in on_dvr.
	hooks := NewHooksService(hooksOnPublishFlags(true, false))
	wg.Add(1)
	go func() {
		defer wg.Done()
		r2 = hooks.Run(ctx, cancel)
	}()

	// Start SRS server and wait for it to be ready.
	svr := NewSRSServer(func(v *srsServer) {
		v.envs = []string{
			"SRS_VHOST_DVR_ENABLED=on",
			"SRS_VHOST_DVR_DVR_PLAN=session",
			"SRS_VHOST_DVR_DVR_PATH=./objs/nginx/html/[app]/[stream].[timestamp].mp4",
			"SRS_VHOST_HTTP_HOOKS_ENABLED=on",
			fmt.Sprintf("SRS_VHOST_HTTP_HOOKS_ON_PUBLISH=http://localhost:%v/api/v1/publish", hooks.HooksAPI()),
			fmt.Sprintf("SRS_VHOST_HTTP_HOOKS_ON_DVR=http://localhost:%v/api/v1/dvrs", hooks.HooksAPI()),
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-hooks.ReadyCtx().Done()
		r0 = svr.Run(ctx, cancel)
	}()

	// Publish HEVC. SRS must reject it, but the publisher exit must not cancel
	// the case before the asynchronous on_dvr callback is received.
	streamID := fmt.Sprintf("stream-%v-%v", os.Getpid(), rand.Int())
	streamURL := fmt.Sprintf("rtmp://localhost:%v/live/%v", svr.RTMPPort(), streamID)
	ffmpeg := NewFFmpeg(func(v *ffmpegClient) {
		v.cancelCaseWhenQuit = false
		v.args = []string{
			"-stream_loop", "-1", "-re", "-i", *srsPublishAvatar, "-an", "-vcodec", "libx265",
			"-profile:v", "main", "-preset", "ultrafast", "-f", "flv", streamURL,
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()
		r1 = ffmpeg.Run(ctx, cancel)
	}()

	select {
	case <-ctx.Done():
	case evt := <-hooks.HooksEvents():
		onDvrEvt, ok := evt.(*HooksEventOnDvr)
		if !ok {
			r3 = errors.Errorf("invalid on_dvr event %T", evt)
			cancel()
			break
		}

		if onDvrEvt.Stream != streamID {
			r3 = errors.Errorf("invalid on_dvr stream=%v, expected=%v", onDvrEvt.Stream, streamID)
			cancel()
			break
		}

		dvrFile := path.Join(svr.WorkDir(), onDvrEvt.File)
		stat, err := os.Stat(dvrFile)
		if err != nil {
			r3 = errors.Wrapf(err, "stat empty mp4=%v", dvrFile)
		} else if stat.Size() == 0 {
			r3 = errors.Errorf("empty mp4 container has zero bytes, file=%v", dvrFile)
		} else if _, err = os.Stat(dvrFile + ".tmp"); !os.IsNotExist(err) {
			r3 = errors.Errorf("temporary mp4 still exists, file=%v.tmp, err=%v", dvrFile, err)
		}

		if r3 == nil {
			cmd := exec.CommandContext(ctx, *srsFFprobe, "-v", "error", "-show_entries", "format=format_name",
				"-of", "default=noprint_wrappers=1:nokey=1", dvrFile)
			output, err := cmd.CombinedOutput()
			if err != nil {
				r3 = errors.Wrapf(err, "ffprobe empty mp4=%v, output=%v", dvrFile, string(output))
			} else if !strings.Contains(string(output), "mp4") {
				r3 = errors.Errorf("invalid empty mp4 format=%v, file=%v", string(output), dvrFile)
			}
		}

		if r3 == nil {
			cmd := exec.CommandContext(ctx, *srsFFprobe, "-v", "error", "-show_entries", "stream=index",
				"-of", "csv=p=0", dvrFile)
			output, err := cmd.CombinedOutput()
			if err != nil {
				r3 = errors.Wrapf(err, "ffprobe streams from empty mp4=%v, output=%v", dvrFile, string(output))
			} else if strings.TrimSpace(string(output)) != "" {
				r3 = errors.Errorf("empty mp4 has streams=%v, file=%v", string(output), dvrFile)
			}
		}

		cancel()
	}
}

func TestSlow_RtmpPublish_DvrMp4_HEVC_CodecChangeUnsupported(t *testing.T) {
	// This case is run in parallel.
	t.Parallel()

	// Setup the max timeout for this case.
	ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
	defer cancel()

	// Only enable for github actions, ignore for darwin.
	if runtime.GOOS == "darwin" {
		logger.Tf(ctx, "Depends on FFmpeg(HEVC over RTMP), only available for GitHub actions")
		return
	}

	// Generate a short HEVC FLV used after the AVC input on the same RTMP connection.
	hevcInput := path.Join(t.TempDir(), "hevc.flv")
	cmd := exec.CommandContext(ctx, *srsFFmpeg, "-i", *srsPublishAvatar, "-an", "-frames:v", "5",
		"-vcodec", "libx265", "-profile:v", "main", "-preset", "ultrafast", "-f", "flv", "-y", hevcInput)
	if output, err := cmd.CombinedOutput(); err != nil {
		t.Fatalf("Generate HEVC input failed, err=%v, output=%v", err, string(output))
	}

	// Check a set of errors.
	var r0, r1, r2, r3 error
	defer func(ctx context.Context) {
		if err := filterTestError(ctx.Err(), r0, r1, r2, r3); err != nil {
			t.Errorf("Fail for err %+v", err)
		} else {
			logger.Tf(ctx, "test done with err %+v", err)
		}
	}(ctx)

	var wg sync.WaitGroup
	defer wg.Wait()

	// AVC is accepted initially, while HEVC must be rejected after the codec change.
	hooks := NewHooksService(hooksOnPublishFlags(true, false))
	wg.Add(1)
	go func() {
		defer wg.Done()
		r2 = hooks.Run(ctx, cancel)
	}()

	// Start SRS server and wait for it to be ready.
	svr := NewSRSServer(func(v *srsServer) {
		v.envs = []string{
			"SRS_VHOST_DVR_ENABLED=on",
			"SRS_VHOST_DVR_DVR_PLAN=session",
			"SRS_VHOST_DVR_DVR_PATH=./objs/nginx/html/[app]/[stream].[timestamp].mp4",
			"SRS_VHOST_HTTP_HOOKS_ENABLED=on",
			fmt.Sprintf("SRS_VHOST_HTTP_HOOKS_ON_PUBLISH=http://localhost:%v/api/v1/publish", hooks.HooksAPI()),
			fmt.Sprintf("SRS_VHOST_HTTP_HOOKS_ON_DVR=http://localhost:%v/api/v1/dvrs", hooks.HooksAPI()),
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-hooks.ReadyCtx().Done()
		r0 = svr.Run(ctx, cancel)
	}()

	// Publish AVC and then HEVC through the same RTMP transport.
	streamID := fmt.Sprintf("stream-%v-%v", os.Getpid(), rand.Int())
	streamURL := fmt.Sprintf("rtmp://localhost:%v/live/%v", svr.RTMPPort(), streamID)
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()

		publisher := srsbench.NewRTMPPublisher()
		defer publisher.Close()

		if err := publisher.Publish(ctx, streamURL); err != nil {
			r1 = errors.Wrap(err, "publish RTMP")
			cancel()
			return
		}
		if err := publisher.Ingest(ctx, *srsPublishAvatar); err != nil {
			r1 = errors.Wrap(err, "ingest AVC")
			cancel()
			return
		}

		// The server is expected to close the transport while ingesting HEVC.
		if err := publisher.Ingest(ctx, hevcInput); err != nil {
			logger.Tf(ctx, "HEVC ingest stopped after codec rejection, err=%v", err)
		}
	}()

	select {
	case <-ctx.Done():
	case evt := <-hooks.HooksEvents():
		onDvrEvt, ok := evt.(*HooksEventOnDvr)
		if !ok {
			r3 = errors.Errorf("invalid on_dvr event %T", evt)
			cancel()
			break
		}

		if onDvrEvt.Stream != streamID {
			r3 = errors.Errorf("invalid on_dvr stream=%v, expected=%v", onDvrEvt.Stream, streamID)
			cancel()
			break
		}

		dvrFile := path.Join(svr.WorkDir(), onDvrEvt.File)
		stat, err := os.Stat(dvrFile)
		if err != nil {
			r3 = errors.Wrapf(err, "stat finalized AVC mp4=%v", dvrFile)
		} else if stat.Size() == 0 {
			r3 = errors.Errorf("finalized AVC mp4 has zero bytes, file=%v", dvrFile)
		} else if _, err = os.Stat(dvrFile + ".tmp"); !os.IsNotExist(err) {
			r3 = errors.Errorf("temporary mp4 still exists, file=%v.tmp, err=%v", dvrFile, err)
		}

		if r3 == nil {
			cmd := exec.CommandContext(ctx, *srsFFprobe, "-v", "error", "-show_entries", "stream=codec_name,codec_type",
				"-of", "csv=p=0", dvrFile)
			output, err := cmd.CombinedOutput()
			streams := strings.TrimSpace(string(output))
			if err != nil {
				r3 = errors.Wrapf(err, "ffprobe finalized AVC mp4=%v, output=%v", dvrFile, string(output))
			} else if lines := strings.Split(streams, "\n"); len(lines) != 1 ||
				!strings.Contains(lines[0], "h264") || !strings.Contains(lines[0], "video") {
				r3 = errors.Errorf("invalid finalized AVC streams=%v, file=%v", streams, dvrFile)
			}
		}

		cancel()
	}
}

func TestSlow_SrtPublish_RtmpPlay_HEVC_Basic(t *testing.T) {
	// This case is run in parallel.
	t.Parallel()

	// Setup the max timeout for this case.
	ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
	defer cancel()

	// Only enable for github actions, ignore for darwin.
	if runtime.GOOS == "darwin" {
		logger.Tf(ctx, "Depends on FFmpeg(HEVC over RTMP), only available for GitHub actions")
		return
	}

	// Check a set of errors.
	var r0, r1, r2, r3, r4, r5, r6, r7 error
	defer func(ctx context.Context) {
		if err := filterTestError(ctx.Err(), r0, r1, r2, r3, r4, r5, r6, r7); err != nil {
			t.Errorf("Fail for err %+v", err)
		} else {
			logger.Tf(ctx, "test done with err %+v", err)
		}
	}(ctx)

	var wg sync.WaitGroup
	defer wg.Wait()

	// Start hooks service.
	hooks := NewHooksService(hooksOnPublishFlags(false, true))
	wg.Add(1)
	go func() {
		defer wg.Done()
		r7 = hooks.Run(ctx, cancel)
	}()

	// Start SRS server and wait for it to be ready.
	svr := NewSRSServer(func(v *srsServer) {
		v.envs = []string{
			"SRS_SRT_SERVER_ENABLED=on",
			"SRS_VHOST_SRT_ENABLED=on",
			"SRS_VHOST_SRT_SRT_TO_RTMP=on",
			"SRS_VHOST_HTTP_HOOKS_ENABLED=on",
			fmt.Sprintf("SRS_VHOST_HTTP_HOOKS_ON_PUBLISH=http://localhost:%v/api/v1/publish", hooks.HooksAPI()),
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-hooks.ReadyCtx().Done()
		r0 = svr.Run(ctx, cancel)
	}()

	// Start FFmpeg to publish stream.
	streamID := fmt.Sprintf("stream-%v-%v", os.Getpid(), rand.Int())
	streamURL := fmt.Sprintf("srt://localhost:%v?streamid=#!::r=live/%v,m=publish", svr.SRTPort(), streamID)
	ffmpeg := NewFFmpeg(func(v *ffmpegClient) {
		v.args = []string{
			// Use the fastest preset of x265, see https://x265.readthedocs.io/en/master/presets.html
			"-stream_loop", "-1", "-re", "-i", *srsPublishAvatar, "-acodec", "copy", "-vcodec", "libx265",
			"-profile:v", "main", "-preset", "ultrafast", "-pes_payload_size", "0", "-f", "mpegts", streamURL,
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()
		r1 = ffmpeg.Run(ctx, cancel)
	}()

	// Start FFprobe to detect and verify stream.
	duration := time.Duration(*srsFFprobeDuration) * time.Millisecond
	ffprobe := NewFFprobe(func(v *ffprobeClient) {
		v.dvrFile = path.Join(svr.WorkDir(), "objs", fmt.Sprintf("srs-ffprobe-%v.ts", streamID))
		v.streamURL = fmt.Sprintf("rtmp://localhost:%v/live/%v", svr.RTMPPort(), streamID)
		v.duration, v.timeout = duration, time.Duration(*srsFFprobeTimeout)*time.Millisecond
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()
		r2 = ffprobe.Run(ctx, cancel)
	}()

	// Fast quit for probe done.
	select {
	case <-ctx.Done():
	case <-ffprobe.ProbeDoneCtx().Done():
		defer cancel()

		str, m := ffprobe.Result()
		if len(m.Streams) != 2 {
			r3 = errors.Errorf("invalid streams=%v, %v, %v", len(m.Streams), m.String(), str)
		}

		// Note that HLS score is low, so we only check duration.
		if dv := m.Duration(); dv < duration {
			r5 = errors.Errorf("short duration=%v < %v, %v, %v", dv, duration, m.String(), str)
		}

		if v := m.Video(); v == nil {
			r5 = errors.Errorf("no video %v, %v", m.String(), str)
		} else if v.CodecName != "hevc" {
			r6 = errors.Errorf("invalid video codec=%v, %v, %v", v.CodecName, m.String(), str)
		}
	}
}

func TestSlow_SrtPublish_HttpFlvPlay_HEVC_Basic(t *testing.T) {
	// This case is run in parallel.
	t.Parallel()

	// Setup the max timeout for this case.
	ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
	defer cancel()

	// Only enable for github actions, ignore for darwin.
	if runtime.GOOS == "darwin" {
		logger.Tf(ctx, "Depends on FFmpeg(HEVC over RTMP), only available for GitHub actions")
		return
	}

	// Check a set of errors.
	var r0, r1, r2, r3, r4, r5, r6, r7 error
	defer func(ctx context.Context) {
		if err := filterTestError(ctx.Err(), r0, r1, r2, r3, r4, r5, r6, r7); err != nil {
			t.Errorf("Fail for err %+v", err)
		} else {
			logger.Tf(ctx, "test done with err %+v", err)
		}
	}(ctx)

	var wg sync.WaitGroup
	defer wg.Wait()

	// Start hooks service.
	hooks := NewHooksService(hooksOnPublishFlags(false, true))
	wg.Add(1)
	go func() {
		defer wg.Done()
		r7 = hooks.Run(ctx, cancel)
	}()

	// Start SRS server and wait for it to be ready.
	svr := NewSRSServer(func(v *srsServer) {
		v.envs = []string{
			"SRS_HTTP_SERVER_ENABLED=on",
			"SRS_SRT_SERVER_ENABLED=on",
			"SRS_VHOST_SRT_ENABLED=on",
			"SRS_VHOST_SRT_SRT_TO_RTMP=on",
			"SRS_VHOST_HTTP_REMUX_ENABLED=on",
			"SRS_VHOST_HTTP_HOOKS_ENABLED=on",
			fmt.Sprintf("SRS_VHOST_HTTP_HOOKS_ON_PUBLISH=http://localhost:%v/api/v1/publish", hooks.HooksAPI()),
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-hooks.ReadyCtx().Done()
		r0 = svr.Run(ctx, cancel)
	}()

	// Start FFmpeg to publish stream.
	streamID := fmt.Sprintf("stream-%v-%v", os.Getpid(), rand.Int())
	streamURL := fmt.Sprintf("srt://localhost:%v?streamid=#!::r=live/%v,m=publish", svr.SRTPort(), streamID)
	ffmpeg := NewFFmpeg(func(v *ffmpegClient) {
		v.args = []string{
			// Use the fastest preset of x265, see https://x265.readthedocs.io/en/master/presets.html
			"-stream_loop", "-1", "-re", "-i", *srsPublishAvatar, "-acodec", "copy", "-vcodec", "libx265",
			"-profile:v", "main", "-preset", "ultrafast", "-pes_payload_size", "0", "-f", "mpegts", streamURL,
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()
		r1 = ffmpeg.Run(ctx, cancel)
	}()

	// Start FFprobe to detect and verify stream.
	duration := time.Duration(*srsFFprobeDuration) * time.Millisecond
	ffprobe := NewFFprobe(func(v *ffprobeClient) {
		v.dvrFile = path.Join(svr.WorkDir(), "objs", fmt.Sprintf("srs-ffprobe-%v.ts", streamID))
		v.streamURL = fmt.Sprintf("http://localhost:%v/live/%v.flv", svr.HTTPPort(), streamID)
		v.duration, v.timeout = duration, time.Duration(*srsFFprobeTimeout)*time.Millisecond
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()
		r2 = ffprobe.Run(ctx, cancel)
	}()

	// Fast quit for probe done.
	select {
	case <-ctx.Done():
	case <-ffprobe.ProbeDoneCtx().Done():
		defer cancel()

		str, m := ffprobe.Result()
		if len(m.Streams) != 2 {
			r3 = errors.Errorf("invalid streams=%v, %v, %v", len(m.Streams), m.String(), str)
		}

		// Note that HLS score is low, so we only check duration.
		if dv := m.Duration(); dv < duration {
			r5 = errors.Errorf("short duration=%v < %v, %v, %v", dv, duration, m.String(), str)
		}

		if v := m.Video(); v == nil {
			r5 = errors.Errorf("no video %v, %v", m.String(), str)
		} else if v.CodecName != "hevc" {
			r6 = errors.Errorf("invalid video codec=%v, %v, %v", v.CodecName, m.String(), str)
		}
	}
}

func TestSlow_SrtPublish_HttpTsPlay_HEVC_Basic(t *testing.T) {
	// This case is run in parallel.
	t.Parallel()

	// Setup the max timeout for this case.
	ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
	defer cancel()

	// Only enable for github actions, ignore for darwin.
	if runtime.GOOS == "darwin" {
		logger.Tf(ctx, "Depends on FFmpeg(HEVC over RTMP), only available for GitHub actions")
		return
	}

	// Check a set of errors.
	var r0, r1, r2, r3, r4, r5, r6, r7 error
	defer func(ctx context.Context) {
		if err := filterTestError(ctx.Err(), r0, r1, r2, r3, r4, r5, r6, r7); err != nil {
			t.Errorf("Fail for err %+v", err)
		} else {
			logger.Tf(ctx, "test done with err %+v", err)
		}
	}(ctx)

	var wg sync.WaitGroup
	defer wg.Wait()

	// Start hooks service.
	hooks := NewHooksService(hooksOnPublishFlags(false, true))
	wg.Add(1)
	go func() {
		defer wg.Done()
		r7 = hooks.Run(ctx, cancel)
	}()

	// Start SRS server and wait for it to be ready.
	svr := NewSRSServer(func(v *srsServer) {
		v.envs = []string{
			"SRS_HTTP_SERVER_ENABLED=on",
			"SRS_SRT_SERVER_ENABLED=on",
			"SRS_VHOST_SRT_ENABLED=on",
			"SRS_VHOST_SRT_SRT_TO_RTMP=on",
			"SRS_VHOST_HTTP_REMUX_ENABLED=on",
			"SRS_VHOST_HTTP_REMUX_MOUNT=[vhost]/[app]/[stream].ts",
			"SRS_VHOST_HTTP_HOOKS_ENABLED=on",
			fmt.Sprintf("SRS_VHOST_HTTP_HOOKS_ON_PUBLISH=http://localhost:%v/api/v1/publish", hooks.HooksAPI()),
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-hooks.ReadyCtx().Done()
		r0 = svr.Run(ctx, cancel)
	}()

	// Start FFmpeg to publish stream.
	streamID := fmt.Sprintf("stream-%v-%v", os.Getpid(), rand.Int())
	streamURL := fmt.Sprintf("srt://localhost:%v?streamid=#!::r=live/%v,m=publish", svr.SRTPort(), streamID)
	ffmpeg := NewFFmpeg(func(v *ffmpegClient) {
		v.args = []string{
			// Use the fastest preset of x265, see https://x265.readthedocs.io/en/master/presets.htmlß
			"-stream_loop", "-1", "-re", "-i", *srsPublishAvatar, "-acodec", "copy", "-vcodec", "libx265",
			"-profile:v", "main", "-preset", "ultrafast", "-pes_payload_size", "0", "-f", "mpegts", streamURL,
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()
		r1 = ffmpeg.Run(ctx, cancel)
	}()

	// Should wait for TS to generate the contents.
	select {
	case <-ctx.Done():
		r2 = fmt.Errorf("timeout")
		return
	case <-time.After(5 * time.Second):
	}

	// Start FFprobe to detect and verify stream.
	duration := time.Duration(*srsFFprobeDuration) * time.Millisecond
	ffprobe := NewFFprobe(func(v *ffprobeClient) {
		v.dvrFile = path.Join(svr.WorkDir(), "objs", fmt.Sprintf("srs-ffprobe-%v.ts", streamID))
		v.streamURL = fmt.Sprintf("http://localhost:%v/live/%v.ts", svr.HTTPPort(), streamID)
		v.duration, v.timeout = duration, time.Duration(*srsFFprobeTimeout)*time.Millisecond
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()

		r2 = ffprobe.Run(ctx, cancel)
	}()

	// Fast quit for probe done.
	select {
	case <-ctx.Done():
	case <-ffprobe.ProbeDoneCtx().Done():
		defer cancel()

		str, m := ffprobe.Result()
		if len(m.Streams) != 2 {
			r3 = errors.Errorf("invalid streams=%v, %v, %v", len(m.Streams), m.String(), str)
		}

		// Note that HLS score is low, so we only check duration.
		if dv := m.Duration(); dv < duration/2 {
			r5 = errors.Errorf("short duration=%v < %v, %v, %v", dv, duration/2, m.String(), str)
		}

		if v := m.Video(); v == nil {
			r5 = errors.Errorf("no video %v, %v", m.String(), str)
		} else if v.CodecName != "hevc" {
			r6 = errors.Errorf("invalid video codec=%v, %v, %v", v.CodecName, m.String(), str)
		}
	}
}

func TestSlow_SrtPublish_HlsPlay_HEVC_Basic(t *testing.T) {
	// This case is run in parallel.
	t.Parallel()

	// Setup the max timeout for this case.
	ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
	defer cancel()
	// Check a set of errors.
	var r0, r1, r2, r3, r4, r5 error
	defer func(ctx context.Context) {
		if err := filterTestError(ctx.Err(), r0, r1, r2, r3, r4, r5); err != nil {
			t.Errorf("Fail for err %+v", err)
		} else {
			logger.Tf(ctx, "test done with err %+v", err)
		}
	}(ctx)

	var wg sync.WaitGroup
	defer wg.Wait()

	// Start hooks service.
	hooks := NewHooksService(hooksOnPublishFlags(false, true))
	wg.Add(1)
	go func() {
		defer wg.Done()
		r5 = hooks.Run(ctx, cancel)
	}()

	// Start SRS server and wait for it to be ready.
	svr := NewSRSServer(func(v *srsServer) {
		v.envs = []string{
			"SRS_HTTP_SERVER_ENABLED=on",
			"SRS_SRT_SERVER_ENABLED=on",
			"SRS_VHOST_SRT_ENABLED=on",
			"SRS_VHOST_SRT_SRT_TO_RTMP=on",
			"SRS_VHOST_HLS_ENABLED=on",
			"SRS_VHOST_HTTP_HOOKS_ENABLED=on",
			fmt.Sprintf("SRS_VHOST_HTTP_HOOKS_ON_PUBLISH=http://localhost:%v/api/v1/publish", hooks.HooksAPI()),
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-hooks.ReadyCtx().Done()
		r0 = svr.Run(ctx, cancel)
	}()

	// Start FFmpeg to publish stream.
	streamID := fmt.Sprintf("stream-%v-%v", os.Getpid(), rand.Int())
	streamURL := fmt.Sprintf("srt://localhost:%v?streamid=#!::r=live/%v,m=publish", svr.SRTPort(), streamID)
	ffmpeg := NewFFmpeg(func(v *ffmpegClient) {
		v.args = []string{
			// Use the fastest preset of x265, see https://x265.readthedocs.io/en/master/presets.html
			"-stream_loop", "-1", "-re", "-i", *srsPublishAvatar, "-acodec", "copy", "-vcodec", "libx265",
			"-profile:v", "main", "-preset", "ultrafast", "-r", "25", "-g", "50", "-pes_payload_size", "0",
			"-f", "mpegts", streamURL,
		}
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()

		r1 = ffmpeg.Run(ctx, cancel)
	}()

	// Should wait for HLS to generate the ts files.
	select {
	case <-ctx.Done():
		r2 = fmt.Errorf("timeout")
		return
	case <-time.After(20 * time.Second):
	}

	// Start FFprobe to detect and verify stream.
	duration := time.Duration(*srsFFprobeDuration) * time.Millisecond
	ffprobe := NewFFprobe(func(v *ffprobeClient) {
		v.dvrFile = path.Join(svr.WorkDir(), "objs", fmt.Sprintf("srs-ffprobe-%v.ts", streamID))
		v.streamURL = fmt.Sprintf("http://localhost:%v/live/%v.m3u8", svr.HTTPPort(), streamID)
		v.duration, v.timeout = duration, time.Duration(*srsFFprobeHEVCTimeout)*time.Millisecond
	})
	wg.Add(1)
	go func() {
		defer wg.Done()
		<-svr.ReadyCtx().Done()
		r2 = ffprobe.Run(ctx, cancel)
	}()

	// Fast quit for probe done.
	select {
	case <-ctx.Done():
	case <-ffprobe.ProbeDoneCtx().Done():
		defer cancel()

		str, m := ffprobe.Result()
		if len(m.Streams) != 2 {
			r3 = errors.Errorf("invalid streams=%v, %v, %v", len(m.Streams), m.String(), str)
		}

		// Note that HLS score is low, so we only check duration. Note that only check half of duration, because we
		// might get only some pieces of segments.
		if dv := m.Duration(); dv < duration/2 {
			r4 = errors.Errorf("short duration=%v < %v, %v, %v", dv, duration/2, m.String(), str)
		}
	}
}
