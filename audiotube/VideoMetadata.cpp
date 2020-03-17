#include "VideoMetadata.h"

QRegularExpression VideoMetadata::getUrlMatcher() {
    return _ytRegexIdFinder;
}

VideoMetadata VideoMetadata::fromVideoId(const QString &videoId) {
    auto url = VideoMetadata::urlFromVideoId(videoId);
    return VideoMetadata::fromVideoUrl(url);
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

QString VideoMetadata::sts() const {
    return this->_sts;
}

QString VideoMetadata::playerSourceUrl() const {
    return this->_playerSourceUrl;
}

int VideoMetadata::duration() {
    return this->_durationInSeconds;
}

bool VideoMetadata::isMetadataValid() {
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

void VideoMetadata::setSts(const QString &sts) {
    this->_sts = sts;
};

void VideoMetadata::setPlayerSourceUrl(const QString &pSourceUrl) {
    this->_playerSourceUrl = pSourceUrl;
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

void VideoMetadata::setAudioStreamInfos(const AudioStreamInfos &streamInfos) {
    this->_audioStreamInfos = streamInfos;
}

const AudioStreamInfos& VideoMetadata::audioStreams() const {
    return this->_audioStreamInfos;
}