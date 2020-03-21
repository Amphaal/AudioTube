#include "VideoMetadata.h"

QRegularExpression VideoMetadata::getUrlMatcher() {
    return _ytRegexIdFinder;
}

VideoMetadata VideoMetadata::fromVideoId(const QString &videoId) {
    auto url = VideoMetadata::urlFromVideoId(videoId);
    return VideoMetadata::fromVideoUrl(url);
}

void VideoMetadata::setPreferedPlayerConfigFetchingMethod(const VideoMetadata::PreferedPlayerConfigFetchingMethod &method) {
    this->_preferedPlayerConfigFetchingMethod = method;
}

VideoMetadata::PreferedPlayerConfigFetchingMethod VideoMetadata::preferedPlayerConfigFetchingMethod() const {
    return this->_preferedPlayerConfigFetchingMethod;
}

PlayerConfiguration::PreferedAudioStreamsInfosSource VideoMetadata::preferedAudioStreamsInfosSource() const {
    return this->_preferedAudioStreamsInfosSource;
}

QUrl VideoMetadata::getBestAvailableStreamUrl() const {
    
    //sort itags [first is best]
    auto itags = this->_audioStreamInfos.uniqueKeys();
    std::sort(itags.begin(), itags.end());

    return this->_audioStreamInfos.value(itags.first());

}

VideoMetadata VideoMetadata::fromVideoUrl(const QString &url) {
    
    //find id
    auto match = _ytRegexIdFinder.match(url);

    //returns
    if(!match.hasMatch()) {
        throw std::invalid_argument("URL is not a valid  URL !");
    }

    auto videoId = match.captured("videoId");
    return VideoMetadata(videoId);
}

QString VideoMetadata::urlFromVideoId(const QString &videoId) {
    return QStringLiteral(u"https://www.youtube.com/watch?v=") + videoId;
}

VideoMetadata::VideoMetadata(const VideoMetadata::Id &videoId) : _videoId(videoId), _url(urlFromVideoId(videoId)) { };

VideoMetadata::Id VideoMetadata::id() const {
    return this->_videoId;
}

QString VideoMetadata::title() const {
    return this->_title;
}

QString VideoMetadata::url() const {
    return this->_url;
}

int VideoMetadata::duration() const {
    return this->_durationInSeconds;
}

bool VideoMetadata::isMetadataValid() const {
    if(this->_validUntil.isNull()) return false;
    return QDateTime::currentDateTime() < this->_validUntil;
}

bool VideoMetadata::hasFailed() const {
    return this->_failed;
}

void VideoMetadata::setRanOnce() {
    this->_ranOnce = true;
}

bool VideoMetadata::ranOnce() const {
    return this->_ranOnce;
}

void VideoMetadata::setFailure(bool failed) {
    if(failed == true && this->_failed != failed) emit streamFailed();
    this->_failed = failed;
}

void VideoMetadata::setTitle(const QString &title) {
    this->_title = title;
}

void VideoMetadata::setDuration(int durationInSeconds) {
    this->_durationInSeconds = durationInSeconds;
}

void VideoMetadata::setExpirationDate(const QDateTime &expiration) {
    this->_validUntil = expiration;
}

void VideoMetadata::setAudioStreamsPackage(const PlayerConfiguration::AudioStreamsPackage &streamInfos) {
    
    if(streamInfos.first == PlayerConfiguration::PreferedAudioStreamsInfosSource::Unknown || !streamInfos.second.count()) 
        throw std::logic_error("Setting empty audio stream Infos is not allowed !");
    
    this->_audioStreamInfos = streamInfos.second;
    this->_preferedAudioStreamsInfosSource = streamInfos.first;

}

const PlayerConfiguration::AudioStreamUrlByITag& VideoMetadata::audioStreams() const {
    return this->_audioStreamInfos;
}