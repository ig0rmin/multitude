// For PRIx64
#define __STDC_FORMAT_MACROS

#include "AVDecoderFFMPEG.hpp"
#include "MemoryPool.hpp"
#include "AudioTransfer2.hpp"

#include <Nimble/Vector2.hpp>

#include <Radiant/Allocators.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/Sleep.hpp>

#include <Resonant/DSPNetwork.hpp>

#include <QSet>
#include <QString>
#include <QThread>

#include <array>
#include <cassert>

extern "C" {
  typedef uint64_t UINT64_C;
  typedef int64_t INT64_C;

# include <libavdevice/avdevice.h>

# include <libavutil/imgutils.h>
# include <libavutil/opt.h>
# include <libavutil/pixdesc.h>

# include <libavformat/avformat.h>

# include <libavcodec/avcodec.h>

# include <libavfilter/avfiltergraph.h>
# include <libavfilter/avcodec.h>
# include <libavfilter/buffersink.h>
# include <libavfilter/buffersrc.h>

# include <libswscale/swscale.h>
}

namespace
{
  template <typename T, size_t N>
  class LockFreeQueue
  {
  public:
    LockFreeQueue();

    bool setSize(int items);
    int size() const;

    T * takeFree();
    void put();

    int itemCount() const;

    T * readyItem(int index = 0);
    T * lastReadyItem();
    void next();

  private:
    std::array<T, N> m_data;
    QAtomicInt m_readyItems;
    // index of the current queue head, "next ready item" (if m_readyItems > 0)
    int m_reader;
    // index of the next free item (if m_readyItems < m_size)
    int m_writer;
    int m_size;
  };

  template <typename T, size_t N>
  LockFreeQueue<T, N>::LockFreeQueue()
    : m_reader(0)
    , m_writer(0)
    , m_size(N)
  {}

  template <typename T, size_t N>
  bool LockFreeQueue<T, N>::setSize(int items)
  {
    m_size = std::min<int>(items, m_data.size());
    return m_size == items;
  }

  template <typename T, size_t N>
  int LockFreeQueue<T, N>::size() const
  {
    return m_size;
  }

  template <typename T, size_t N>
  T * LockFreeQueue<T, N>::takeFree()
  {
    if(m_readyItems >= m_size)
      return 0;

    int index = m_writer++;

    return & m_data[index % N];
  }

  template <typename T, size_t N>
  void LockFreeQueue<T, N>::put()
  {
    m_readyItems.ref();
  }

  template <typename T, size_t N>
  int LockFreeQueue<T, N>::itemCount() const
  {
    return m_readyItems;
  }

  template <typename T, size_t N>
  T * LockFreeQueue<T, N>::readyItem(int index)
  {
    if(index >= m_readyItems) return nullptr;
    return & m_data[(m_reader + index) % N];
  }

  template <typename T, size_t N>
  T * LockFreeQueue<T, N>::lastReadyItem()
  {
    if(m_readyItems < 1) return nullptr;
    return & m_data[(m_reader + m_readyItems - 1) % N];
  }

  template <typename T, size_t N>
  void LockFreeQueue<T, N>::next()
  {
    m_readyItems.deref();
    ++m_reader;
  }

  int ffmpegLock(void ** mutexPtr, enum AVLockOp op)
  {
    Radiant::Mutex *& mutex = reinterpret_cast<Radiant::Mutex *&>(*mutexPtr);

    switch(op) {
    case AV_LOCK_CREATE:
      mutex = new Radiant::Mutex();
      return !mutex;

    case AV_LOCK_OBTAIN:
      mutex->lock();
      return 0;

    case AV_LOCK_RELEASE:
      mutex->unlock();
      return 0;

    case AV_LOCK_DESTROY:
      delete mutex;
      mutex = 0;
      return 0;
    }
    return 1;
  }

  RADIANT_TLS(const char *) s_src = nullptr;

  void ffmpegLog(void *, int level, const char * fmt, va_list vl)
  {
    if(level > AV_LOG_INFO) return;
    //AVClass ** avclass = reinterpret_cast<AVClass**>(ptr);

    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), fmt, vl);
    for(int i = strlen(buffer) - 1; i >= 0; --i) {
      if(buffer[i] == '\r' || buffer[i] == '\n')
        buffer[i] = '\0';
      else
        break;
    }

#ifdef RADIANT_OSX
    QString msg = QString("%1: %2").arg(s_src.get()).arg(buffer);
#else
    QString msg = QString("%1: %2").arg(s_src).arg(buffer);
#endif

    if(level > AV_LOG_WARNING) {
      Radiant::info("%s", msg.toUtf8().data());
    } else if(level > AV_LOG_ERROR) {
      Radiant::warning("%s", msg.toUtf8().data());
    } else {
      Radiant::error("%s", msg.toUtf8().data());
    }
  }

  void ffmpegInit()
  {
    MULTI_ONCE_BEGIN

    av_log_set_callback(ffmpegLog);
    avcodec_register_all();
    avdevice_register_all();
    av_register_all();
    avformat_network_init();
    avfilter_register_all();

    int err = av_lockmgr_register(ffmpegLock);
    if(err != 0)
      Radiant::error("ffmpegInit # Failed to register new FFMPEG lock manager");

    MULTI_ONCE_END
  }

  typedef int (*QueryFormatsFunc)(AVFilterContext *);
  QueryFormatsFunc s_origQueryFormats = nullptr;
  int asinkQueryFormats(AVFilterContext * filterContext)
  {
    assert(s_origQueryFormats);
    int ret = s_origQueryFormats(filterContext);
    /// @todo should not hard-code
    const int lst[] = {44100, -1};
    AVFilterFormats * fmts = avfilter_make_format_list(lst);
    avfilter_formats_ref(fmts, &filterContext->inputs[0]->in_samplerates);
    return ret;
  }

  void avError(const QString & prefix, int err)
  {
    char buffer[128];
    av_strerror(err, buffer, sizeof(buffer));
    Radiant::error("%s - %s", prefix.toUtf8().data(), buffer);
  }

  // Supported audio formats
  const AVSampleFormat s_sampleFmts[] = {
    // We have to make the conversion to planar float for Resonant anyway,
    // why not let avformat to do it for us
    AV_SAMPLE_FMT_FLTP,
    (AVSampleFormat)-1
  };


  // Supported video formats
  // We support:
  //   - all 8 bit planar YUV formats
  //   - grayscale formats
  // We don't support on purpose:
  //   - packed YUV, rendering those is silly and slow
  //   - any other RGB-style format except bgr24 and bgra, better convert it
  //     here than in drivers / render thread - except with OpenGL ES.
  //     And GL_ARB_texture_swizzle isn't supported in OS X
  //   - palette formats
  //   - 1 bit monowhite/monoblack
  //   - accelerated formats like xvmc / va api / vdpau, they don't work with multi-threaded rendering
  //   - nv12 / nv21 (first plane for Y, second plane for UV) - rendering would be slow and weird
  const PixelFormat s_pixFmts[] = {
    PIX_FMT_YUV420P,   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
    PIX_FMT_YUV422P,   ///< planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
    PIX_FMT_YUV444P,   ///< planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
    PIX_FMT_YUV410P,   ///< planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
    PIX_FMT_YUV411P,   ///< planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
    PIX_FMT_GRAY8,     ///<        Y        ,  8bpp
    PIX_FMT_YUVJ420P,  ///< planar YUV 4:2:0, 12bpp, full scale (JPEG), deprecated in favor of PIX_FMT_YUV420P and setting color_range
    PIX_FMT_YUVJ422P,  ///< planar YUV 4:2:2, 16bpp, full scale (JPEG), deprecated in favor of PIX_FMT_YUV422P and setting color_range
    PIX_FMT_YUVJ444P,  ///< planar YUV 4:4:4, 24bpp, full scale (JPEG), deprecated in favor of PIX_FMT_YUV444P and setting color_range

#ifdef LUMINOUS_OPENGLES
    PIX_FMT_RGB24,     ///< packed RGB 8:8:8, 24bpp, RGBRGB...
    PIX_FMT_RGBA,      ///< packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
#else
    PIX_FMT_BGR24,     ///< packed RGB 8:8:8, 24bpp, BGRBGR...
    PIX_FMT_BGRA,      ///< packed BGRA 8:8:8:8, 32bpp, BGRABGRA...
#endif

    PIX_FMT_YUV440P,   ///< planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)
    PIX_FMT_YUVJ440P,  ///< planar YUV 4:4:0 full scale (JPEG), deprecated in favor of PIX_FMT_YUV440P and setting color_range
    PIX_FMT_YUVA420P,  ///< planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A samples)

    PIX_FMT_GRAY8A,    ///< 8bit gray, 8bit alpha

    PIX_FMT_YUVA444P,  ///< planar YUV 4:4:4 32bpp, (1 Cr & Cb sample per 1x1 Y & A samples)
    PIX_FMT_YUVA422P,  ///< planar YUV 4:2:2 24bpp, (1 Cr & Cb sample per 2x1 Y & A samples)

    PIX_FMT_NONE
  };
}

namespace VideoPlayer2
{
  class VideoFrameFFMPEG : public VideoFrame
  {
  public:
    VideoFrameFFMPEG() : VideoFrame(), bufferRef(nullptr) {}
    AVFilterBufferRef * bufferRef;
  };

  class AVDecoderFFMPEG::D
  {
  public:

    struct MyAV
    {
      AVPacket packet;
      AVFrame * frame;

      AVFormatContext * formatContext;

      AVCodecContext * videoCodecContext;
      AVCodec * videoCodec;

      AVCodecContext * audioCodecContext;
      AVCodec * audioCodec;

      int videoStreamIndex;
      int audioStreamIndex;

      double videoTsToSecs;
      double audioTsToSecs;
      int decodedAudioBufferSamples;
      bool needFlushAtEof;
      bool seekByBytes;
      bool seekingSupported;

      double duration;
      double start;
      Nimble::Vector2i videoSize;

      bool dr1;
    };

    D()
      : seekGeneration(0)
      , running(true)
      , finished(false)
      , av()
      , realTimeSeeking(false)
      , pauseTimestamp(Radiant::TimeStamp::getTime())
      , videoFilter()
      , audioFilter()
      , radiantTimestampToPts(std::numeric_limits<double>::quiet_NaN())
      , loopOffset(0)
      , audioTransfer(nullptr)
    {
      av.videoStreamIndex = -1;
      av.audioStreamIndex = -1;
      av.videoSize.clear();
    }

    int seekGeneration;

    bool running;
    bool finished;

    MyAV av;

    MemoryPool<DecodedImageBuffer, 80> imageBuffers;

    bool realTimeSeeking;
    SeekRequest seekRequest;

    AVDecoder::Options options;
    Radiant::TimeStamp pauseTimestamp;

    struct FilterGraph
    {
      AVFilterContext * bufferSourceContext;
      AVFilterContext * bufferSinkContext;
      AVFilterGraph * graph;
    };
    FilterGraph videoFilter;
    FilterGraph audioFilter;

    // only used when there is no audio
    double radiantTimestampToPts;

    double loopOffset;

    AudioTransfer * audioTransfer;

    /// From main thread to decoder thread, list of BufferRefs that should be
    /// released. Can't run that in the main thread without locking.
    LockFreeQueue<AVFilterBufferRef*, 40> consumedBufferRefs;

    LockFreeQueue<VideoFrameFFMPEG, 40> decodedVideoFrames;

    bool initFilters(FilterGraph & filterGraph, const QString & description,
                     bool video);
    bool open();
    void close();

    bool seekToBeginning();
    bool seek();

    bool decodeVideoPacket(double & dpts, double & nextDpts);
    bool decodeAudioPacket(double & dpts, double & nextDpts);
    VideoFrameFFMPEG * getFreeFrame(bool setTimestampToPts, double & dpts);
    void checkSeek(double & nextVideoDpts, double & videoDpts, double & nextAudioDpts);

    static int getBuffer(AVCodecContext * context, AVFrame * frame);
    ///static int regetBuffer(struct AVCodecContext * context, AVFrame * frame);
    static void releaseBuffer(AVCodecContext * context, AVFrame * frame);
    static void releaseFilterBuffer(AVFilterBuffer * filterBuffer);
    void setFormat(VideoFrameFFMPEG & frame, const AVPixFmtDescriptor & fmtDescriptor,
                   Nimble::Vector2i size);
  };

  bool AVDecoderFFMPEG::D::initFilters(FilterGraph & filterGraph,
                                       const QString & description, bool video)
  {
    QByteArray errorMsg("AVDecoderFFMPEG::D::initFilters # " + options.src.toUtf8() +
                        " " + (video ? "video" : "audio") +":");

    AVFilter * buffersrc = nullptr;
    AVFilter * buffersink = nullptr;
    AVFilterInOut * outputs = nullptr;
    AVFilterInOut * inputs  = nullptr;
    int err = 0;

    try {
      buffersrc = avfilter_get_by_name(video ? "buffer" : "abuffer");
      if(!buffersrc) throw "Failed to find video filter \"(a)buffer\"";

      buffersink = avfilter_get_by_name(video ? "buffersink" : "abuffersink");
      if(!buffersink) throw "Failed to find video filter \"(a)buffersink\"";

      filterGraph.graph = avfilter_graph_alloc();
      if(!filterGraph.graph) throw "Failed to allocate filter graph";

      QString args;
      if(video) {
        args.sprintf("%d:%d:%d:%d:%d:%d:%d",
                     av.videoCodecContext->width, av.videoCodecContext->height,
                     av.videoCodecContext->pix_fmt,
                     av.videoCodecContext->time_base.num, av.videoCodecContext->time_base.den,
                     av.videoCodecContext->sample_aspect_ratio.num,
                     av.videoCodecContext->sample_aspect_ratio.den);
        int err = avfilter_graph_create_filter(&filterGraph.bufferSourceContext, buffersrc,
                                               "in", args.toUtf8().data(), nullptr, filterGraph.graph);
        if(err < 0) throw "Failed to create video buffer source";

#if FF_API_OLD_VSINK_API
        // Const cast because this old avfilter api just takes void * when we
        // want to give it a list of pixel formats (const)
        err = avfilter_graph_create_filter(&filterGraph.bufferSinkContext, buffersink,
                                           "out", nullptr, const_cast<PixelFormat*>(s_pixFmts),
                                           filterGraph.graph);
#else
        AVBufferSinkParams * bufferSinkParams = av_buffersink_params_alloc();
        if(!bufferSinkParams) throw "Failed to allocate AVBufferSinkParams";
        bufferSinkParams->pixel_fmts = s_pixFmts;
        ret = avfilter_graph_create_filter(&filterGraph.bufferSinkContext, buffersink,
                                           "out", nullptr, bufferSinkParams, filterGraph.graph);
        av_freep(&bufferSinkParams);
        bufferSinkParams = nullptr;
#endif
        if(err < 0) throw "Failed to create video buffer sink";
      } else {
        if(!av.audioCodecContext->channel_layout)
          av.audioCodecContext->channel_layout = av_get_default_channel_layout(
                av.audioCodecContext->channels);

        /// @todo ffmpeg application uses AVStream instead of codec context to
        ///       read time_base, is this wrong?
        args.sprintf("time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
                     av.audioCodecContext->time_base.num,
                     av.audioCodecContext->time_base.den,
                     av.audioCodecContext->sample_rate,
                     av_get_sample_fmt_name(av.audioCodecContext->sample_fmt),
                     av.audioCodecContext->channel_layout);
        err = avfilter_graph_create_filter(&filterGraph.bufferSourceContext, buffersrc,
                                           "in", args.toUtf8().data(), nullptr, filterGraph.graph);
        if(err < 0) throw "Failed to create audio buffer source";

        AVABufferSinkParams * bufferSinkParams = av_abuffersink_params_alloc();
        bufferSinkParams->sample_fmts = s_sampleFmts;
        /// @todo maybe if the original audioChannels is not defined, we
        ///       should use avfilter_all_channel_layouts heree
        const int64_t channelLayouts[] = {
          av_get_default_channel_layout(options.audioChannels), -1 };
        bufferSinkParams->channel_layouts = channelLayouts;
        err = avfilter_graph_create_filter(&filterGraph.bufferSinkContext, buffersink, "out",
                                           nullptr, bufferSinkParams, filterGraph.graph);
        av_freep(&bufferSinkParams);
        if(err < 0) throw "Failed to create audio buffer sink";

        QueryFormatsFunc func = filterGraph.bufferSinkContext->filter->query_formats;
        if(!s_origQueryFormats && func != asinkQueryFormats)
          s_origQueryFormats = func;
        filterGraph.bufferSinkContext->filter->query_formats = asinkQueryFormats;
      }

      if(!description.isEmpty()) {
        outputs = avfilter_inout_alloc();
        if(!outputs) throw "Failed to allocate AVFilterInOut";

        inputs  = avfilter_inout_alloc();
        if(!inputs) throw "Failed to allocate AVFilterInOut";

        outputs->name = av_strdup("in");
        outputs->filter_ctx = filterGraph.bufferSourceContext;
        outputs->pad_idx = 0;
        outputs->next = nullptr;

        inputs->name = av_strdup("out");
        inputs->filter_ctx = filterGraph.bufferSinkContext;
        inputs->pad_idx = 0;
        inputs->next = nullptr;

        err = avfilter_graph_parse(filterGraph.graph, description.toUtf8().data(),
                                   &inputs, &outputs, nullptr);
        if(err < 0) throw "Failed to parse filter description";

        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
      } else {
        err = avfilter_link(filterGraph.bufferSourceContext, 0,
                            filterGraph.bufferSinkContext, 0);
        if(err < 0) throw "Failed to link buffer source and buffer sink";
      }

      err = avfilter_graph_config(filterGraph.graph, nullptr);
      if(err < 0) throw "Graph failed validity test";

      return true;
    } catch (const char * error) {
      if(err < 0) {
        avError(QString("%1 %2").arg(errorMsg.data(), error), err);
      } else {
        Radiant::error("%s %s", errorMsg.data(), error);
      }
      avfilter_inout_free(&inputs);
      avfilter_inout_free(&outputs);
      avfilter_graph_free(&filterGraph.graph);
    }
    return false;
  }

  bool AVDecoderFFMPEG::D::open()
  {
    AVInputFormat * inputFormat = nullptr;
    AVDictionary * avoptions = nullptr;

    QByteArray errorMsg("AVDecoderFFMPEG::D::open # " + options.src.toUtf8() + ":");

    if(!options.demuxerOptions.isEmpty()) {
      for(auto it = options.demuxerOptions.begin(); it != options.demuxerOptions.end(); ++it) {
        int err = av_dict_set(&avoptions, it.key().toUtf8().data(), it.value().toUtf8().data(), 0);
        if(err < 0) {
          Radiant::warning("%s av_dict_set(%s, %s): %d", errorMsg.data(),
                           it.key().toUtf8().data(), it.value().toUtf8().data(), err);
        }
      }
    }

    // If user specified any specific format, try to use that.
    // Otherwise avformat_open_input will just auto-detect the format.
    if(!options.format.isEmpty()) {
      inputFormat = av_find_input_format(options.format.toUtf8().data());
      if(!inputFormat)
        Radiant::warning("%s Failed to find input format '%s'", errorMsg.data(), options.format.toUtf8().data());
    }

    // Open the actual video, should be thread-safe
    int err = avformat_open_input(&av.formatContext, options.src.toUtf8().data(),
                                  inputFormat, &avoptions);

    {
      AVDictionaryEntry * it = nullptr;
      while((it = av_dict_get(avoptions, "", it, AV_DICT_IGNORE_SUFFIX))) {
        Radiant::warning("%s Unrecognized demuxer option %s = %s",
                         errorMsg.data(), it->key, it->value);
      }
      av_dict_free(&avoptions);
      avoptions = nullptr;
    }

    if(err != 0) {
      avError(QString("%1 Failed to open the source file").arg(errorMsg.data()), err);
      return false;
    }

    // Retrieve stream information, avformat processes some stream data, so
    // this might take a while, and it might fail with some files (at least
    // with some mkv files), so we don't abort on error
    err = avformat_find_stream_info(av.formatContext, nullptr);
    if(err < 0)
      avError(QString("%1 Failed to find stream info").arg(errorMsg.data()), err);

    if(options.video) {
      av.videoStreamIndex = av_find_best_stream(av.formatContext, AVMEDIA_TYPE_VIDEO,
                                                options.videoStreamIndex, -1,
                                                &av.videoCodec, 0);
      if(av.videoStreamIndex < 0) {
        if(av.videoStreamIndex == AVERROR_STREAM_NOT_FOUND) {
          Radiant::warning("%s Video stream not found", errorMsg.data());
        } else if(av.videoStreamIndex == AVERROR_DECODER_NOT_FOUND) {
          Radiant::error("%s No decoder found for any video stream", errorMsg.data());
        } else {
          Radiant::error("%s Error #%d when trying to find video stream",
                         errorMsg.data(), av.videoStreamIndex);
        }
      } else {
        av.videoCodecContext = av.formatContext->streams[av.videoStreamIndex]->codec;
        assert(av.videoCodecContext);
        av.videoCodecContext->opaque = this;
        av.videoCodecContext->thread_count = 1;
      }
    }

    if(options.audio) {
      av.audioStreamIndex = av_find_best_stream(av.formatContext, AVMEDIA_TYPE_AUDIO,
                                                options.audioStreamIndex, -1,
                                                &av.audioCodec, 0);
      if(av.audioStreamIndex < 0) {
        if(av.audioStreamIndex == AVERROR_STREAM_NOT_FOUND) {
          Radiant::debug("%s Audio stream not found", errorMsg.data());
        } else if(av.audioStreamIndex == AVERROR_DECODER_NOT_FOUND) {
          Radiant::error("%s No decoder found for any audio stream", errorMsg.data());
        } else {
          Radiant::error("%s Error #%d when trying to find audio stream",
                         errorMsg.data(), av.audioStreamIndex);
        }
      } else {
        av.audioCodecContext = av.formatContext->streams[av.audioStreamIndex]->codec;
        assert(av.audioCodecContext);
        av.audioCodecContext->opaque = this;
        av.audioCodecContext->thread_count = 1;
      }
    }

    if(!av.videoCodec && !av.audioCodec) {
      Radiant::error("%s Didn't open any media streams", errorMsg.data());
      avformat_close_input(&av.formatContext);
      return false;
    }

    // Open codecs
    if(av.videoCodec) {
      if(!options.videoOptions.isEmpty()) {
        for(auto it = options.videoOptions.begin(); it != options.videoOptions.end(); ++it) {
          int err = av_dict_set(&avoptions, it.key().toUtf8().data(), it.value().toUtf8().data(), 0);
          if(err < 0) {
            Radiant::warning("%s av_dict_set(%s, %s): %d", errorMsg.data(),
                             it.key().toUtf8().data(), it.value().toUtf8().data(), err);
          }
        }
      }

      err = avcodec_open2(av.videoCodecContext, av.videoCodec, &avoptions);

      {
        AVDictionaryEntry * it = nullptr;
        while((it = av_dict_get(avoptions, "", it, AV_DICT_IGNORE_SUFFIX))) {
          Radiant::warning("%s Unrecognized video codec option %s = %s",
                           errorMsg.data(), it->key, it->value);
        }
        av_dict_free(&avoptions);
        avoptions = nullptr;
      }

      if(err < 0) {
        av.videoCodecContext = nullptr;
        av.videoCodec = nullptr;
        avError(QString("%1 Failed to open video codec").arg(errorMsg.data()), err);
      }
    }

    if(av.audioCodec) {
      if(!options.audioOptions.isEmpty()) {
        for(auto it = options.audioOptions.begin(); it != options.audioOptions.end(); ++it) {
          int err = av_dict_set(&avoptions, it.key().toUtf8().data(), it.value().toUtf8().data(), 0);
          if(err < 0) {
            Radiant::warning("%s av_dict_set(%s, %s): %d", errorMsg.data(),
                             it.key().toUtf8().data(), it.value().toUtf8().data(), err);
          }
        }
      }

      err = avcodec_open2(av.audioCodecContext, av.audioCodec, &avoptions);

      {
        AVDictionaryEntry * it = nullptr;
        while((it = av_dict_get(avoptions, "", it, AV_DICT_IGNORE_SUFFIX))) {
          Radiant::warning("%s Unrecognized audio codec option %s = %s",
                           errorMsg.data(), it->key, it->value);
        }
        av_dict_free(&avoptions);
        avoptions = nullptr;
      }

      if(err < 0) {
        av.audioCodecContext = nullptr;
        av.audioCodec = nullptr;
        avError(QString("%1 Failed to open audio codec").arg(errorMsg.data()), err);
      }
    }

    if(!av.videoCodec && !av.audioCodec) {
      Radiant::error("%s Failed to open any media stream codecs", errorMsg.data());
      avformat_close_input(&av.formatContext);
      return false;
    }

    // We want to use our own ImageBuffers with AVFrames to avoid data copying
    // and to extend the lifetimes of the buffers outside this class
    // If the codec doesn't support that, we have to make a copy of the data
    // buffer after decoding. When using filters, we just use buffer refs

    if(av.videoCodecContext) {
      if(av.videoCodecContext && (av.videoCodec->capabilities & CODEC_CAP_DR1)) {
        av.videoCodecContext->get_buffer = getBuffer;
        //videoCodecContext->reget_buffer = AVDecoderFFMPEG::D::regetBuffer;
        av.videoCodecContext->release_buffer = releaseBuffer;
        av.dr1 = true;
      } else {
        Radiant::debug("%s Codec has no CODEC_CAP_DR1, need to copy the image data every frame", errorMsg.data());
        av.dr1 = false;
      }

      bool pixelFormatSupported = false;
      for(auto it = s_pixFmts; *it != PIX_FMT_NONE; ++it) {
        if(av.videoCodecContext->pix_fmt == *it) {
          pixelFormatSupported = true;
          break;
        }
      }
      const bool useVideoFilters = !pixelFormatSupported || !options.videoFilters.isEmpty();

      if(useVideoFilters)
        initFilters(videoFilter, options.videoFilters, true);
    }

    if(av.audioCodecContext) {
      if(options.audioChannels <= 0)
        options.audioChannels = av.audioCodecContext->channels;

      bool audioFormatSupported = false;
      for(auto it = s_sampleFmts; *it != (AVSampleFormat)-1; ++it) {
        if(av.audioCodecContext->sample_fmt == *it) {
          audioFormatSupported = true;
          break;
        }
      }
      /// @todo shouldn't be hard-coded
      const int targetSampleRate = 44100;
      const bool useAudioFilters = !audioFormatSupported ||
          !options.audioFilters.isEmpty() ||
          av.audioCodecContext->sample_rate != targetSampleRate ||
          av.audioCodecContext->channels != options.audioChannels;

      if(useAudioFilters)
        initFilters(audioFilter, options.audioFilters, false);
    }

    // pts/dts x video/audioTsToSecs == timestamp in seconds
    if(av.videoCodecContext) {
      const auto & time_base = av.formatContext->streams[av.videoStreamIndex]->time_base;
      av.videoTsToSecs = time_base.den != 0 ? av_q2d(time_base) :
                                              av_q2d(av.videoCodecContext->time_base) *
                                              av.videoCodecContext->ticks_per_frame;
    }

    if(av.audioCodecContext) {
      const auto & time_base = av.formatContext->streams[av.audioStreamIndex]->time_base;
      av.audioTsToSecs = time_base.den != 0 ? av_q2d(time_base) :
                                              av_q2d(av.audioCodecContext->time_base) *
                                              av.audioCodecContext->ticks_per_frame;
    }

    // Size of the decoded audio buffer, in samples (~44100 samples means one second buffer)
    av.decodedAudioBufferSamples = av.audioCodecContext ?
          options.audioBufferSeconds * av.audioCodecContext->sample_rate : 0;

    av.needFlushAtEof = (av.audioCodec && (av.audioCodec->capabilities & CODEC_CAP_DELAY)) ||
        (av.videoCodec && (av.videoCodec->capabilities & CODEC_CAP_DELAY));

    // We seek by bytes only if the input file has timestamp discontinuities
    // (seeking by timestamp doesn't really make sense in that case). If the
    // format doesn't support byte seek, we still use timestamp seeking as a
    // fallback, and then just hope for the best.
    av.seekByBytes = (av.formatContext->iformat->flags & AVFMT_TS_DISCONT) &&
        !(av.formatContext->iformat->flags & AVFMT_NO_BYTE_SEEK);

    /// @todo can seeking be supported even if format context doesn't have an IO Context?
    av.seekingSupported = av.formatContext->pb && av.formatContext->pb->seekable;

    av_init_packet(&av.packet);

    // In theory we could write this, but we wouldn't be binary-compatible with
    // different ffmpeg versions:
    // AVFrame avFrame;
    // avcodec_get_frame_defaults(&avFrame);
    av.frame = avcodec_alloc_frame();
    if(!av.frame) {
      Radiant::error("%s Failed to allocate new AVFrame", errorMsg.data());
      close();
      return false;
    }

    if(av.audioCodec) {
      audioTransfer = new AudioTransfer(options.audioChannels);
      audioTransfer->setSeekGeneration(seekGeneration);
      audioTransfer->setPlayMode(options.playMode);

      static QAtomicInt counter;
      int value = counter.fetchAndAddRelease(1);
      audioTransfer->setId(QString("VideoPlayer2.AudioTransfer.%1").arg(value));

      auto item = Resonant::DSPNetwork::Item();
      item.setModule(audioTransfer);
      item.setTargetChannel(0);

      Resonant::DSPNetwork::instance()->addModule(item);
    }

    if(av.videoCodecContext) {
      av.videoSize = Nimble::Vector2i(av.videoCodecContext->width, av.videoCodecContext->height);
    } else {
      av.videoSize.clear();
    }
    av.duration = av.formatContext->duration / double(AV_TIME_BASE);
    av.start = std::numeric_limits<double>::quiet_NaN();

    return true;
  }

  void AVDecoderFFMPEG::D::close()
  {
    av.duration = 0;
    av.videoSize.clear();

    // Close the codecs
    if(av.audioCodecContext || av.videoCodecContext) {
      if(av.audioCodecContext)
        avcodec_close(av.audioCodecContext);
      if(av.videoCodecContext)
        avcodec_close(av.videoCodecContext);
    }

    // Close the video file
    if(av.formatContext)
      avformat_close_input(&av.formatContext);

    av_free(av.frame);

    if(audioTransfer)
      Resonant::DSPNetwork::instance()->markDone(*audioTransfer);
    audioTransfer = nullptr;
  }

  bool AVDecoderFFMPEG::D::seekToBeginning()
  {
    int err = 0;
    if(av.seekingSupported) {
      if(av.seekByBytes) {
        err = avformat_seek_file(av.formatContext, -1,
                                 std::numeric_limits<int64_t>::min(), 0,
                                 std::numeric_limits<int64_t>::max(),
                                 AVSEEK_FLAG_BYTE);
      } else {
        int64_t pos = av.formatContext->start_time == (int64_t) AV_NOPTS_VALUE
            ? 0 : av.formatContext->start_time;
        err = avformat_seek_file(av.formatContext, -1,
                                 std::numeric_limits<int64_t>::min(), pos,
                                 std::numeric_limits<int64_t>::max(), 0);
      }
      if(err < 0) {
        avError(QString("AVDecoderFFMPEG::D::seekToBeginning # %1: Seek error, re-opening the stream").arg(options.src), err);
        close();
        return open();
      } else {
        if(av.audioCodecContext)
          avcodec_flush_buffers(av.audioCodecContext);
        if(av.videoCodecContext)
          avcodec_flush_buffers(av.videoCodecContext);
      }
    } else {
      // If we want to loop, but there is no way to seek, we just close
      // and re-open the stream
      close();
      return open();
    }
    return true;
  }

  bool AVDecoderFFMPEG::D::seek()
  {
    QByteArray errorMsg("AVDecoderFFMPEG::D::seek # " + options.src.toUtf8() + ":");

    if(seekRequest.value <= std::numeric_limits<double>::epsilon()) {
      bool ok = seekToBeginning();
      if(ok) {
        ++seekGeneration;
        if(audioTransfer)
          audioTransfer->setSeekGeneration(seekGeneration);
        radiantTimestampToPts = std::numeric_limits<double>::quiet_NaN();
        if(options.playMode == Pause)
          pauseTimestamp = Radiant::TimeStamp::getTime();
      }
      return ok;
    }

    if(!av.seekingSupported)
      return false;

    bool seekByBytes = av.seekByBytes || seekRequest.type == SeekByBytes;

    if(seekRequest.type == SeekByBytes &&
       (av.formatContext->iformat->flags & AVFMT_NO_BYTE_SEEK)) {
      Radiant::error("%s Seek failed, media doesn't support byte seeking",
                     errorMsg.data());
      return false;
    }

    int64_t pos = 0;
    if(!seekByBytes) {
      if(seekRequest.type == SeekBySeconds) {
        pos = seekRequest.value * AV_TIME_BASE;
      } else {
        assert(seekRequest.type == SeekRelative);
        if(av.formatContext->duration > 0) {
          pos = seekRequest.value * av.formatContext->duration;
        } else {
          if(av.formatContext->iformat->flags & AVFMT_NO_BYTE_SEEK) {
            Radiant::error("%s Seek failed, couldn't get the content duration"
                           " and the media doesn't support byte seeking",
                           errorMsg.data());
            return false;
          }
          seekByBytes = true;
        }
      }
      if(av.formatContext->start_time != (int64_t) AV_NOPTS_VALUE)
          pos += av.formatContext->start_time;
    }

    if(seekByBytes) {
      if(seekRequest.type == SeekByBytes) {
        pos = seekRequest.value;
      } else if(seekRequest.type == SeekBySeconds) {
        int64_t size = avio_size(av.formatContext->pb);
        if(av.formatContext->duration <= 0 || size <= 0) {
          Radiant::error("%s Seek failed, couldn't get the media duration/size",
                         errorMsg.data());
          return false;
        }
        // This is just a guess, since there is no byte size and time 1:1 mapping
        pos = size * seekRequest.value / av.duration;

      } else {
        assert(seekRequest.type == SeekRelative);
        int64_t size = avio_size(av.formatContext->pb);
        if(size <= 0) {
          Radiant::error("%s Seek failed, couldn't get the media size",
                         errorMsg.data());
          return false;
        }
        pos = seekRequest.value * size;
      }
    }

    int64_t minTs = seekRequest.direction == SeekOnlyForward
        ? pos : std::numeric_limits<int64_t>::min();
    int64_t maxTs = seekRequest.direction == SeekOnlyBackward
        ? pos : std::numeric_limits<int64_t>::max();

    int err = avformat_seek_file(av.formatContext, -1, minTs, pos, maxTs,
                                 seekByBytes ? AVSEEK_FLAG_BYTE : 0);
    if(err < 0) {
      Radiant::error("%s Seek failed", errorMsg.data());
      return false;
    }

    if(av.audioCodecContext)
      avcodec_flush_buffers(av.audioCodecContext);
    if(av.videoCodecContext)
      avcodec_flush_buffers(av.videoCodecContext);
    ++seekGeneration;
    if(audioTransfer)
      audioTransfer->setSeekGeneration(seekGeneration);
    radiantTimestampToPts = std::numeric_limits<double>::quiet_NaN();
    if(options.playMode == Pause)
      pauseTimestamp = Radiant::TimeStamp::getTime();

    return true;
  }

  VideoFrameFFMPEG * AVDecoderFFMPEG::D::getFreeFrame(bool setTimestampToPts, double & dpts)
  {
    while(running) {
      VideoFrameFFMPEG * frame = decodedVideoFrames.takeFree();
      if(frame) return frame;
      // Set this here, because another frame might be waiting for us
      // However, if we have a filter that changes pts, this might not be right.
      if(Nimble::Math::isNAN(radiantTimestampToPts)) {
        const Radiant::TimeStamp now = Radiant::TimeStamp::getTime();
        radiantTimestampToPts = dpts + loopOffset - now.secondsD() + 4.0/60.0;
        setTimestampToPts = true;
      }
      if(!running) break;
      // if the video buffer is full, and audio buffer is almost empty,
      // we need to resize the video buffer, otherwise we could starve.
      // Growing the video buffer is safe, as long as the buffer size
      // doesn't grow over the hard-limit (setSize checks that)
      if(audioTransfer && audioTransfer->bufferStateSeconds() < options.audioBufferSeconds * 0.15f) {
        if(decodedVideoFrames.setSize(decodedVideoFrames.size() + 1)) {
          options.videoBufferFrames = decodedVideoFrames.size();
          continue;
        }
      }

      Radiant::Sleep::sleepMs(10);
    }
    return nullptr;
  }

  void AVDecoderFFMPEG::D::setFormat(VideoFrameFFMPEG & frame, const AVPixFmtDescriptor & fmtDescriptor,
                                     Nimble::Vector2i size)
  {
    // not exactly true for all formats, but it is true for all formats that we support
    frame.planes = (fmtDescriptor.flags & PIX_FMT_PLANAR) ? fmtDescriptor.nb_components : 1;

    if(fmtDescriptor.nb_components == 1)
      frame.format = VideoFrame::GRAY;
    else if(fmtDescriptor.nb_components == 2)
      frame.format = VideoFrame::GRAY_ALPHA;
    else if(fmtDescriptor.nb_components == 3 && (fmtDescriptor.flags & PIX_FMT_RGB))
      frame.format = VideoFrame::RGB;
    else if(fmtDescriptor.nb_components == 3)
      frame.format = VideoFrame::YUV;
    else if(fmtDescriptor.nb_components == 4 && (fmtDescriptor.flags & PIX_FMT_RGB))
      frame.format = VideoFrame::RGBA;
    else if(fmtDescriptor.nb_components == 4)
      frame.format = VideoFrame::YUVA;
    else {
      frame.format = VideoFrame::UNKNOWN;
      frame.planes = 0;
    }

    for(int i = 0; i < frame.planes; ++i) {
      frame.planeSize[i] = size;
      if((frame.format == VideoFrame::YUV || frame.format == VideoFrame::YUVA) && (i == 1 || i == 2)) {
        frame.planeSize[i] = Nimble::Vector2i(
              -((-size.x) >> fmtDescriptor.log2_chroma_w),
              -((-size.y) >> fmtDescriptor.log2_chroma_h));
      }
      frame.lineSize[i] = 0;
      frame.data[i] = nullptr;
    }
    for(int i = frame.planes; i < 4; ++i) {
      frame.planeSize[i] = Nimble::Vector2i(0, 0);
      frame.lineSize[i] = 0;
      frame.data[i] = nullptr;
    }
  }

  bool AVDecoderFFMPEG::D::decodeVideoPacket(double & dpts, double & nextDpts)
  {
    const double prevDpts = dpts;
    dpts = std::numeric_limits<double>::quiet_NaN();

    int gotPicture = 0;
    avcodec_get_frame_defaults(av.frame);
    int err = avcodec_decode_video2(av.videoCodecContext, av.frame, &gotPicture, &av.packet);
    if(err < 0) {
      avError(QString("AVDecoderFFMPEG::D::decodeVideoPacket # %1: Failed to decode a video frame").
              arg(options.src), err);
      return false;
    }

    if(!gotPicture)
      return false;

    int64_t pts = av_frame_get_best_effort_timestamp(av.frame);
    if(pts == (int64_t) AV_NOPTS_VALUE)
      pts = av.frame->pts;
    if(pts == (int64_t) AV_NOPTS_VALUE)
      pts = av.frame->pkt_pts;

    dpts = av.videoTsToSecs * pts;

    VideoFrameFFMPEG * frame = 0;
    bool setTimestampToPts = false;

    DecodedImageBuffer * buffer = nullptr;
    if(av.dr1 && av.frame->opaque) {
      buffer = static_cast<DecodedImageBuffer*>(av.frame->opaque);
      buffer->refcount.ref();
    }

    if(videoFilter.graph) {
      AVFilterBufferRef * ref = avfilter_get_video_buffer_ref_from_frame(av.frame,
                                                                         AV_PERM_READ | AV_PERM_WRITE);
      if(buffer) {
        ref->buf->priv = new std::pair<AVDecoderFFMPEG::D *, DecodedImageBuffer *>(this, buffer);
        ref->buf->free = releaseFilterBuffer;
      }

      int err = av_buffersrc_add_ref(videoFilter.bufferSourceContext, ref,
                                     AV_BUFFERSRC_FLAG_NO_CHECK_FORMAT |
                                     AV_BUFFERSRC_FLAG_NO_COPY);
      if(err < 0) {
        avError(QString("AVDecoderFFMPEG::D::decodeVideoPacket # %1: av_buffersrc_add_ref failed").
                arg(options.src), err);
        avfilter_unref_buffer(ref);
      } else {
        while((err = avfilter_poll_frame(videoFilter.bufferSinkContext->inputs[0])) > 0) {
          AVFilterBufferRef * output = nullptr;
          // we either use custom deleter (releaseFilterBuffer) or the
          // default one with the actual data cleared
          if(!buffer)
            av.packet.data = nullptr;
          int err2 = av_buffersink_get_buffer_ref(videoFilter.bufferSinkContext, &output, 0);
          if(err2 < 0) {
            avError(QString("AVDecoderFFMPEG::D::decodeVideoPacket # %1: av_buffersink_get_buffer_ref failed").
                    arg(options.src), err2);
            break;
          }

          if(output) {
            frame = getFreeFrame(setTimestampToPts, dpts);
            if(!frame)
              return false;

            frame->bufferRef = output;
            frame->imageBuffer = nullptr;

            auto & fmtDescriptor = av_pix_fmt_descriptors[output->format];
            setFormat(*frame, fmtDescriptor, Nimble::Vector2i(output->video->w, output->video->h));
            for (int i = 0; i < frame->planes; ++i) {
              frame->lineSize[i] = output->linesize[i];
              frame->data[i] = output->data[i];
            }

            /// output->pts should be AV_NOPTS_VALUE if not defined,
            /// but some filters just set it always to zero
            if(output->pts != (int64_t) AV_NOPTS_VALUE && output->pts != 0) {
              pts = output->pts;
              dpts = av.videoTsToSecs * output->pts;
            }

            frame->imageSize = Nimble::Vector2i(output->video->w, output->video->h);
            frame->timestamp = Timestamp(dpts + loopOffset, seekGeneration);

            decodedVideoFrames.put();
          }
        }
        if(err < 0) {
          avError(QString("AVDecoderFFMPEG::D::decodeVideoPacket # %1: avfilter_poll_frame failed").
                  arg(options.src), err);
        }
      }
    } else {
      frame = getFreeFrame(setTimestampToPts, dpts);
      if(!frame)
        return false;

      frame->bufferRef = nullptr;
      frame->imageBuffer = buffer;

      auto & fmtDescriptor = av_pix_fmt_descriptors[av.frame->format];
      int bytes = 0;
      setFormat(*frame, fmtDescriptor, Nimble::Vector2i(av.frame->width, av.frame->height));
      for(int i = 0; i < frame->planes; ++i) {
        frame->lineSize[i] = av.frame->linesize[i];
        frame->data[i] = av.frame->data[i];
        bytes += frame->lineSize[i] * frame->planeSize[i].y;
      }

      if(!buffer) {
        buffer = imageBuffers.get();
        if(!buffer) {
          Radiant::error("AVDecoderFFMPEG::D::decodeVideoPacket # %s: Not enough ImageBuffers",
                         options.src.toUtf8().data());
          for(int i = 0; i < frame->planes; ++i)
            frame->data[i] = nullptr;
          frame->planes = 0;
        } else {
          buffer->refcount = 1;
          frame->imageBuffer = buffer;
          buffer->data.resize(bytes);
          for(int offset = 0, i = 0; i < frame->planes; ++i) {
            uint8_t * dst = buffer->data.data() + offset;
            bytes = frame->lineSize[i] * frame->planeSize[i].y;
            offset += bytes;
            memcpy(dst, av.frame->data[i], bytes);
            frame->data[i] = dst;
          }
        }
      }

      frame->imageSize = Nimble::Vector2i(av.frame->width, av.frame->height);
      frame->timestamp = Timestamp(dpts + loopOffset, seekGeneration);
      decodedVideoFrames.put();
    }

    // Normally av.packet.duration can't be trusted
    if(Nimble::Math::isNAN(prevDpts)) {
      nextDpts = av.videoTsToSecs * (av.packet.duration + pts);
    } else {
      nextDpts = dpts + (dpts - prevDpts);
    }

    if(Nimble::Math::isNAN(radiantTimestampToPts) || setTimestampToPts) {
      const Radiant::TimeStamp now = Radiant::TimeStamp::getTime();
      radiantTimestampToPts = dpts + loopOffset - now.secondsD() + 4.0/60.0;
    }

    return true;
  }

  bool AVDecoderFFMPEG::D::decodeAudioPacket(double & dpts, double & nextDpts)
  {
    AVPacket packet = av.packet;
    bool gotFrames = false;
    bool flush = packet.size == 0;

    while(running && (packet.size > 0 || flush)) {
      int gotFrame = 0;
      avcodec_get_frame_defaults(av.frame);
      int consumedBytes = avcodec_decode_audio4(av.audioCodecContext, av.frame,
                                                &gotFrame, &packet);
      if(consumedBytes < 0) {
        avError(QString("AVDecoderFFMPEG::D::decodeAudioPacket # %1: Audio decoding error").
                arg(options.src), consumedBytes);
        break;
      }

      if(gotFrame) {
        gotFrames = true;
        int64_t pts = av_frame_get_best_effort_timestamp(av.frame);
        if(pts == (int64_t) AV_NOPTS_VALUE)
          pts = av.frame->pts;
        if(pts == (int64_t) AV_NOPTS_VALUE)
          pts = av.frame->pkt_pts;

        dpts = av.audioTsToSecs * pts;
        nextDpts = dpts + double(av.frame->nb_samples) / av_frame_get_sample_rate(av.frame);

        DecodedAudioBuffer * decodedAudioBuffer = nullptr;
        if(audioFilter.graph) {
          AVFilterBufferRef * ref = nullptr;
          ref = avfilter_get_audio_buffer_ref_from_frame(av.frame, AV_PERM_READ | AV_PERM_WRITE);
          int err = av_buffersrc_add_ref(audioFilter.bufferSourceContext, ref,
                                         0 /*AV_BUFFERSRC_FLAG_NO_CHECK_FORMAT |
                                         AV_BUFFERSRC_FLAG_NO_COPY*/);
          // av_buffersrc_write_frame(audioFilter.bufferSourceContext, av.frame);

          if(err < 0) {
            avError(QString("AVDecoderFFMPEG::D::decodeAudioPacket # %1: av_buffersrc_add_ref failed").
                    arg(options.src), err);
          } else {
            while((err = avfilter_poll_frame(audioFilter.bufferSinkContext->inputs[0])) > 0) {
              AVFilterBufferRef * output = nullptr;
              int err2 = av_buffersink_get_buffer_ref(audioFilter.bufferSinkContext, &output, 0);
              if(err2 < 0) {
                avError(QString("AVDecoderFFMPEG::D::decodeAudioPacket # %1: av_buffersink_get_buffer_ref failed").
                        arg(options.src), err2);
                break;
              }

              if(output) {
                while(true) {
                  decodedAudioBuffer = audioTransfer->takeFreeBuffer(
                        av.decodedAudioBufferSamples - output->audio->nb_samples);
                  if(decodedAudioBuffer) break;
                  if(!running) return gotFrames;
                  Radiant::Sleep::sleepMs(10);
                }

                if(output->pts != (int64_t) AV_NOPTS_VALUE) {
                  pts = output->pts;
                  dpts = av.audioTsToSecs * output->pts;
                  nextDpts = dpts + double(output->audio->nb_samples) / output->audio->sample_rate;
                }

                // av_get_channel_layout_nb_channels
                decodedAudioBuffer->fillPlanar(Timestamp(dpts + loopOffset, seekGeneration),
                                               options.audioChannels, output->audio->nb_samples,
                                               (const float **)(output->data));
                audioTransfer->putReadyBuffer(output->audio->nb_samples);
                avfilter_unref_buffer(output);
              }
            }
            if(err < 0) {
              avError(QString("AVDecoderFFMPEG::D::decodeAudioPacket # %1: avfilter_poll_frame failed").
                      arg(options.src), err);
            }
          }
        } else {
          while(true) {
            decodedAudioBuffer = audioTransfer->takeFreeBuffer(
                  av.decodedAudioBufferSamples - av.frame->nb_samples);
            if(decodedAudioBuffer) break;
            if(!running) return gotFrames;
            Radiant::Sleep::sleepMs(10);
          }

          decodedAudioBuffer->fill(Timestamp(dpts + loopOffset, seekGeneration),
                                   av.audioCodecContext->channels, av.frame->nb_samples,
                                   reinterpret_cast<const int16_t *>(av.frame->data[0]));
          audioTransfer->putReadyBuffer(av.frame->nb_samples);
        }
      } else {
        flush = false;
      }
      packet.data += consumedBytes;
      packet.size -= consumedBytes;
    }
    return gotFrames;
  }

  // This function basically follows the same pattern that is used inside ffmpeg,
  // with some fixes and our custom buffer memory management
  int AVDecoderFFMPEG::D::getBuffer(AVCodecContext * context, AVFrame * frame)
  {
    frame->opaque = nullptr;

    Nimble::Vector2i bufferSize(context->width, context->height);
    if(av_image_check_size(context->width, context->height, 0, context) || context->pix_fmt < 0)
      return -1;

    // ffplay nor the default get_buffer will check this (they might crash with
    // SVQ1 content), but "ffmpeg" application does always allocate too large
    // buffer just because there are some issues in SVQ1 decoder. We will just
    // check the type and then decide the size. A the current version, 32
    // pixels should be enough.
    static const unsigned avEdgeWidth = avcodec_get_edge_width();
    const unsigned edgeWidth = std::max(context->codec_id == CODEC_ID_SVQ1 ? 32u : 0u,
                                        avEdgeWidth);

    // For some reason ffplay and the default get_buffer will do this _after_
    // align_dimensions, even though that is probably wrong (since edgewidth ~ 16
    // and we usually align to 16, the result is the same, for now).
    if((context->flags & CODEC_FLAG_EMU_EDGE) == 0)
      bufferSize += Nimble::Vector2i(edgeWidth*2, edgeWidth*2);

    const int pixelSize = av_pix_fmt_descriptors[context->pix_fmt].comp[0].step_minus1+1;

    int hChromaShift, vChromaShift;
    avcodec_get_chroma_sub_sample(context->pix_fmt, &hChromaShift, &vChromaShift);

    int strideAlign[AV_NUM_DATA_POINTERS];
    avcodec_align_dimensions2(context, &bufferSize.x, &bufferSize.y, strideAlign);

    int unaligned = 0;
    AVPicture picture;
    do {
      // NOTE: do not align linesizes individually, this breaks e.g. assumptions
      // that linesize[0] == 2*linesize[1] in the MPEG-encoder for 4:2:2
      av_image_fill_linesizes(picture.linesize, context->pix_fmt, bufferSize.x);
      // increase alignment of w for next try (rhs gives the lowest bit set in w)
      bufferSize.x += bufferSize.x & ~(bufferSize.x-1);

      unaligned = 0;
      for(int i = 0; i < 4; ++i)
        unaligned |= picture.linesize[i] % strideAlign[i];
    } while (unaligned);

    // we use offsets to null pointer to calculate the number of image planes
    // and their size
    const int tmpsize = av_image_fill_pointers(picture.data, context->pix_fmt,
                                               bufferSize.y, NULL, picture.linesize);
    if(tmpsize < 0)
      return -1;

    int size[4] = {0, 0, 0, 0};
    int lastPlane = 0;
    for(; lastPlane < 3 && picture.data[lastPlane+1]; ++lastPlane)
      size[lastPlane] = picture.data[lastPlane+1] - picture.data[lastPlane];
    size[lastPlane] = tmpsize - (picture.data[lastPlane] - picture.data[0]);

    // For unknown reason the default get_buffer will have a 16 extra bytes in each line.
    // Maybe some codecs need it.
    const int totalsize = size[0] + size[1] + size[2] + size[3] + (lastPlane+1)*16;

    assert(context->opaque);
    AVDecoderFFMPEG::D & d = *static_cast<AVDecoderFFMPEG::D*>(context->opaque);
    DecodedImageBuffer * buffer = d.imageBuffers.get();
    if(!buffer) {
      Radiant::error("AVDecoderFFMPEG::D::getBuffer # %s: not enough ImageBuffers",
                     d.options.src.toUtf8().data());
      return -1;
    }

    buffer->refcount = 0;
    buffer->refcount.ref();
    frame->opaque = buffer;
    buffer->data.resize(totalsize);

    int offset = 0;
    int plane = 0;
    for(; plane < 4 && size[plane]; ++plane) {
      const int hShift = plane == 0 ? 0 : hChromaShift;
      const int vShift = plane == 0 ? 0 : vChromaShift;

      frame->linesize[plane] = picture.linesize[plane];

      frame->base[plane] = buffer->data.data() + offset;
      offset += size[plane] + 16;

      // no edge if EDGE EMU or not planar YUV
      if((context->flags & CODEC_FLAG_EMU_EDGE) || !size[2])
        frame->data[plane] = frame->base[plane];
      else
        frame->data[plane] = frame->base[plane] + FFALIGN(
              ((frame->linesize[plane] * edgeWidth) >> vShift) +
              ((pixelSize * edgeWidth) >> hShift), strideAlign[plane]);
    }
    for (; plane < AV_NUM_DATA_POINTERS; ++plane) {
      frame->base[plane] = frame->data[plane] = nullptr;
      frame->linesize[plane] = 0;
    }

    if(size[1] && !size[2])
      ff_set_systematic_pal2((uint32_t*)frame->data[1], context->pix_fmt);

    // Tell ffmpeg not to do anything weird with this buffer, since this is ours
    frame->type = FF_BUFFER_TYPE_USER;

    frame->extended_data = frame->data;
    frame->sample_aspect_ratio = context->sample_aspect_ratio;

    if(context->pkt) {
      frame->pkt_pts = context->pkt->pts;
      //frame->pkt_pos = contexts->pkt->pos;
    } else {
      frame->pkt_pts = AV_NOPTS_VALUE;
      //frame->pkt_pos = -1;
    }
    frame->reordered_opaque = context->reordered_opaque;
    frame->sample_aspect_ratio = context->sample_aspect_ratio;
    frame->width = context->width;
    frame->height = context->height;
    frame->format = context->pix_fmt;

    return 0;
  }

  /// @todo should implement this. However, it is unclear if we should
  ///       a) make a copy, b) increase use count, c) do nothing with the
  ///       actual ImageBuffer or d) use base implementation.
  ///       We don't know if releaseBuffer will be called twice or once.
  /*int AVDecoderFFMPEG::D::regetBuffer(AVCodecContext * context, AVFrame * frame)
  {
    if(!frame->data[0])
      return context->get_buffer(context, frame);

    frame->reordered_opaque = context->reordered_opaque;
    frame->pkt_pts = context->pkt ? context->pkt->pts : AV_NOPTS_VALUE;
    return 0;
  }*/

  void AVDecoderFFMPEG::D::releaseBuffer(struct AVCodecContext * context, AVFrame * frame)
  {
    assert(context->opaque);
    assert(frame->opaque);
    assert(frame->type == FF_BUFFER_TYPE_USER);

    DecodedImageBuffer & buffer = *static_cast<DecodedImageBuffer*>(frame->opaque);
    if(!buffer.refcount.deref()) {
      AVDecoderFFMPEG::D & d = *static_cast<AVDecoderFFMPEG::D *>(context->opaque);
      d.imageBuffers.put(buffer);
    }
    frame->opaque = nullptr;
    memset(frame->data, 0, sizeof(frame->data));
  }

  void AVDecoderFFMPEG::D::releaseFilterBuffer(AVFilterBuffer * filterBuffer)
  {
    auto * param = static_cast<std::pair<AVDecoderFFMPEG::D *, DecodedImageBuffer *> *>(filterBuffer->priv);

    if(!param->second->refcount.deref())
      param->first->imageBuffers.put(*param->second);

    av_free(filterBuffer);
    delete param;
  }

  void AVDecoderFFMPEG::D::checkSeek(double & nextVideoDpts, double & videoDpts, double & nextAudioDpts)
  {
    if((seekRequest.type != SeekNone)) {
      if(seek()) {
        loopOffset = 0;
        nextVideoDpts = std::numeric_limits<double>::quiet_NaN();
        nextAudioDpts = std::numeric_limits<double>::quiet_NaN();
        videoDpts = std::numeric_limits<double>::quiet_NaN();
      }
      seekRequest.type = SeekNone;
    }
  }

  AVDecoderFFMPEG::AVDecoderFFMPEG()
    : m_d(new D())
  {
    Thread::setName("AVDecoderFFMPEG");
  }

  AVDecoderFFMPEG::~AVDecoderFFMPEG()
  {
    /// @todo We might be forgetting something here, some buffers might leak?
    close();
    while(true) {
      AVFilterBufferRef ** ref = m_d->consumedBufferRefs.readyItem();
      if(!ref) break;
      avfilter_unref_buffer(*ref);
      m_d->consumedBufferRefs.next();
    }
    if(isRunning())
      waitEnd();
    m_d->close();
    delete m_d;
  }

  AVDecoder::PlayMode AVDecoderFFMPEG::playMode() const
  {
    return m_d->options.playMode;
  }

  void AVDecoderFFMPEG::setPlayMode(AVDecoder::PlayMode mode)
  {
    if(m_d->options.playMode == mode)
      return;

    m_d->options.playMode = mode;
    if(m_d->audioTransfer)
      m_d->audioTransfer->setPlayMode(mode);
    if(mode == Pause)
      m_d->pauseTimestamp = Radiant::TimeStamp::getTime();
    if(mode == Play)
      m_d->radiantTimestampToPts -= m_d->pauseTimestamp.sinceSecondsD();
  }

  Timestamp AVDecoderFFMPEG::getTimestampAt(const Radiant::TimeStamp & ts) const
  {
    if(m_d->realTimeSeeking && m_d->av.videoCodec) {
      VideoFrameFFMPEG * frame = m_d->decodedVideoFrames.lastReadyItem();
      if(frame)
        return Timestamp(frame->timestamp.pts + 0.0001, m_d->seekGeneration);
    }

    if(m_d->audioTransfer) {
      Timestamp t = m_d->audioTransfer->toPts(ts);
      if(t.seekGeneration < m_d->seekGeneration)
        return Timestamp();
      return t;
    }

    if(Nimble::Math::isNAN(m_d->radiantTimestampToPts))
      return Timestamp();

    if(m_d->options.playMode == Pause)
      return Timestamp(m_d->pauseTimestamp.secondsD() + m_d->radiantTimestampToPts, m_d->seekGeneration);

    return Timestamp(ts.secondsD() + m_d->radiantTimestampToPts, m_d->seekGeneration);
  }

  VideoFrame * AVDecoderFFMPEG::getFrame(const Timestamp & ts) const
  {
    VideoFrameFFMPEG * ret = 0;
    for(int i = 0;; ++i) {
      VideoFrameFFMPEG * frame = m_d->decodedVideoFrames.readyItem(i);
      if(!frame) break;

      if(frame->timestamp.seekGeneration < ts.seekGeneration)
        continue;

      if(frame->timestamp.pts > ts.pts) {
        if(ret) return ret;
        return frame;
      }
      ret = frame;
    }
    return ret;
  }

  void AVDecoderFFMPEG::releaseOldVideoFrames(const Timestamp & ts, bool * eof)
  {
    int frameIndex = 0;
    for(;; ++frameIndex) {
      VideoFrameFFMPEG * frame = m_d->decodedVideoFrames.readyItem(frameIndex);
      if(!frame) break;

      if(frame->timestamp.seekGeneration >= ts.seekGeneration &&
         frame->timestamp.pts > ts.pts)
        break;
    }

    // always keep one frame alive
    --frameIndex;

    for(int i = 0; i < frameIndex; ++i) {
      VideoFrameFFMPEG * frame = m_d->decodedVideoFrames.readyItem();
      assert(frame);

      DecodedImageBuffer * buffer = frame->imageBuffer;
      if(buffer && !buffer->refcount.deref())
        m_d->imageBuffers.put(*buffer);

      if(frame->bufferRef) {
        AVFilterBufferRef ** ref = m_d->consumedBufferRefs.takeFree();
        if(ref) {
          *ref = frame->bufferRef;
          m_d->consumedBufferRefs.put();
        } else {
          Radiant::error("AVDecoderFFMPEG::releaseOldVideoFrames # consumedBufferRefs is full, leaking memory");
        }
        frame->bufferRef = nullptr;
      }

      m_d->decodedVideoFrames.next();
    }

    if(eof) {
      *eof = m_d->finished && (!m_d->audioTransfer || m_d->audioTransfer->bufferStateSeconds() <= 0.0f)
          && m_d->decodedVideoFrames.itemCount() <= 1;
    }
  }

  Nimble::Matrix4f AVDecoderFFMPEG::yuvMatrix() const
  {
    if(!m_d->av.videoCodecContext)
      return Nimble::Matrix4f::IDENTITY;
    /// @todo why does everything look so wrong when using the correct colorspace?
    ///       for now we just force ITU-R BT601-6 (same as SMPTE170M)
    // this should be m_d->av.videoCodecContext->colorspace
    const int colorspace = SWS_CS_SMPTE170M;
    const int * coeffs = sws_getCoefficients(colorspace);
    int l = 16, h = 235;
    if(m_d->av.videoCodecContext->color_range == AVCOL_RANGE_JPEG) {
      l = 0;
      h = 255;
    }
    // a and b scale the y value from [l, h] -> [0, 1]
    const float a = 255.0f/(h-l);
    const float b = l/255.0;

    const float c[4] = {  coeffs[0]/65536.0f, -coeffs[2]/65536.0f,
                         -coeffs[3]/65536.0f,  coeffs[1]/65536.0f };

    // Last column transform uv from 0..1 to -0.5..0.5
    return Nimble::Matrix4f(
        a, 0.0f, c[0], -b*a - 0.5f * c[0],
        a, c[1], c[2], -b*a - 0.5f * (c[2]+c[1]),
        a, c[3],    0, -b*a - 0.5f * c[3],
        0,    0,    0, 1);
  }

  void AVDecoderFFMPEG::load(const Options & options)
  {
    assert(!isRunning());
    m_d->options = options;
  }

  void AVDecoderFFMPEG::close()
  {
    m_d->running = false;
  }

  Nimble::Vector2i AVDecoderFFMPEG::videoSize() const
  {
    return m_d->av.videoSize;
  }

  void AVDecoderFFMPEG::setLooping(bool doLoop)
  {
    m_d->options.loop = doLoop;
  }

  double AVDecoderFFMPEG::duration() const
  {
    return m_d->av.duration;
  }

  void AVDecoderFFMPEG::seek(const SeekRequest & req)
  {
    m_d->seekRequest = req;
  }

  void AVDecoderFFMPEG::setRealTimeSeeking(bool value)
  {
    m_d->realTimeSeeking = value;
    if(m_d->audioTransfer)
      m_d->audioTransfer->setSeeking(value);
  }

  void AVDecoderFFMPEG::childLoop()
  {
    QByteArray errorMsg("AVDecoderFFMPEG::D::childLoop # " + m_d->options.src.toUtf8() + ":");
    QThread::currentThread()->setPriority(QThread::LowPriority);

    QByteArray src = m_d->options.src.toUtf8();
    s_src = src.data();

    ffmpegInit();

    if(!m_d->open()) {
      m_d->finished = true;
      eventSend("error");
      return;
    }
    eventSend("ready");

    enum EofState {
      Normal,
      Flush,
      Eof
    } eof = EofState::Normal;

    double nextVideoDpts = std::numeric_limits<double>::quiet_NaN();
    double nextAudioDpts = std::numeric_limits<double>::quiet_NaN();
    double videoDpts = std::numeric_limits<double>::quiet_NaN();

    auto & av = m_d->av;

    m_d->pauseTimestamp == Radiant::TimeStamp::getTime();
    bool waitingFrame = false;
    while(m_d->running) {
      m_d->decodedVideoFrames.setSize(m_d->options.videoBufferFrames);

      while(true) {
        AVFilterBufferRef ** ref = m_d->consumedBufferRefs.readyItem();
        if(!ref) break;
        avfilter_unref_buffer(*ref);
        m_d->consumedBufferRefs.next();
      }

      int err = 0;

      if(!waitingFrame || !m_d->realTimeSeeking)
        m_d->checkSeek(nextVideoDpts, videoDpts, nextAudioDpts);

      if(m_d->running && m_d->realTimeSeeking && av.videoCodec) {
        VideoFrameFFMPEG * frame = m_d->decodedVideoFrames.lastReadyItem();
        if(frame && frame->timestamp.seekGeneration == m_d->seekGeneration) {
          Radiant::Sleep::sleepMs(1);
          continue;
        }
      }

      if(eof == EofState::Normal) {
        err = av_read_frame(av.formatContext, &av.packet);
      }

      if(err < 0) {
        /// @todo if we are reading a socket-based stream, it might be possible
        ///       to get eof if our input buffer just ends. We should now call
        ///       read_packet to make sure we actually are at eof
        if(err != AVERROR_EOF) {
          avError(QString("%1 Read error").arg(errorMsg.data()), err);
          break;
        }

        if(av.needFlushAtEof) {
          eof = EofState::Flush;
        } else {
          eof = EofState::Eof;
        }
      }

      // We really are at the end of the stream and we have flushed all the packages
      if(eof == EofState::Eof) {
        if(m_d->realTimeSeeking) {
          Radiant::Sleep::sleepMs(1);
          continue;
        }
        if(m_d->options.loop) {
          m_d->seekToBeginning();
          eof = EofState::Normal;

          if(!Nimble::Math::isNAN(av.start)) {
            // might be NaN
            // no need to check because the comparision will just be false
            double newDuration = nextVideoDpts - av.start;
            if(newDuration > m_d->av.duration) {
              m_d->av.duration = newDuration;
            }
            newDuration = nextAudioDpts - av.start;
            if(newDuration > m_d->av.duration)
              m_d->av.duration = newDuration;
          }

          m_d->loopOffset += m_d->av.duration;
          continue;
        } else {
          // all done
          break;
        }
      }

      av.frame->opaque = nullptr;
      bool gotFrames = false;
      double audioDpts = std::numeric_limits<double>::quiet_NaN();

      if(av.videoCodec && (
           (eof == EofState::Normal && av.packet.stream_index == av.videoStreamIndex) ||
           (eof == EofState::Flush && (av.videoCodec->capabilities & CODEC_CAP_DELAY)))) {
        if(eof == EofState::Flush) {
          av_init_packet(&av.packet);
          av.packet.data = nullptr;
          av.packet.size = 0;
          av.packet.stream_index = av.videoStreamIndex;
        }
        gotFrames = m_d->decodeVideoPacket(videoDpts, nextVideoDpts);
      }

      av.frame->opaque = nullptr;
      if(/*!m_d->realTimeSeeking &&*/ av.audioCodec && (
           (eof == EofState::Normal && av.packet.stream_index == av.audioStreamIndex) ||
           (eof == EofState::Flush && (av.audioCodec->capabilities & CODEC_CAP_DELAY)))) {
        if(eof == EofState::Flush) {
          av_init_packet(&av.packet);
          av.packet.data = nullptr;
          av.packet.size = 0;
          av.packet.stream_index = av.audioStreamIndex;
        }
        gotFrames |= m_d->decodeAudioPacket(audioDpts, nextAudioDpts);
      }

      // Flush is done if there are no more frames
      if(eof == EofState::Flush && !gotFrames)
        eof = EofState::Eof;

      if(Nimble::Math::isNAN(av.start) && gotFrames) {
        if(Nimble::Math::isNAN(videoDpts))
          av.start = audioDpts;
        else if(Nimble::Math::isNAN(audioDpts))
          av.start = videoDpts;
        else
          av.start = std::min(videoDpts, audioDpts);
      }

      waitingFrame = m_d->realTimeSeeking && av.videoCodec && !gotFrames;

      // Free the packet that was allocated by av_read_frame
      av_free_packet(&av.packet);
    }

    eventSend("finished");
    m_d->finished = true;
    s_src = nullptr;
  }
}
