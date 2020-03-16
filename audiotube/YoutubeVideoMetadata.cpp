#include "YoutubeVideoMetadata.h"

QRegularExpression YoutubeVideoMetadata::getYoutubeUrlMatcher() {
    return _ytRegexIdFinder;
}

YoutubeVideoMetadata YoutubeVideoMetadata::fromVideoId(const QString &videoId) {
    auto url = YoutubeVideoMetadata::urlFromVideoId(videoId);
    return YoutubeVideoMetadata::fromVideoUrl(url);
}

YoutubeVideoMetadata YoutubeVideoMetadata::fromVideoUrl(const QString &url) {
    
    //find id
    auto match = _ytRegexIdFinder.match(url);

    //returns
    if(!match.hasMatch()) {
        throw new std::invalid_argument("URL is not a valid Youtube URL !");
    }

    auto videoId = match.captured("videoId");
    return YoutubeVideoMetadata(videoId);
}

QString YoutubeVideoMetadata::urlFromVideoId(const QString &videoId) {
    return QStringLiteral(u"https://www.youtube.com/watch?v=") + videoId;
}

YoutubeVideoMetadata::YoutubeVideoMetadata(const YoutubeVideoMetadata::Id &videoId) : _videoId(videoId), _url(urlFromVideoId(videoId)) { };

YoutubeVideoMetadata::Id YoutubeVideoMetadata::id() const {
    return this->_videoId;
}

QString YoutubeVideoMetadata::title() const {
    return this->_title;
}

QString YoutubeVideoMetadata::url() const {
    return this->_url;
}

QString YoutubeVideoMetadata::sts() const {
    return this->_sts;
}

QString YoutubeVideoMetadata::playerSourceUrl() const {
    return this->_playerSourceUrl;
}

int YoutubeVideoMetadata::duration() {
    return this->_durationInSeconds;
}

bool YoutubeVideoMetadata::isMetadataValid() {
    if(this->_validUntil.isNull()) return false;
    return QDateTime::currentDateTime() < this->_validUntil;
}

bool YoutubeVideoMetadata::hasFailed() const {
    return this->_failed;
}

void YoutubeVideoMetadata::setRanOnce() {
    this->_ranOnce = true;
}

bool YoutubeVideoMetadata::ranOnce() const {
    return this->_ranOnce;
}

void YoutubeVideoMetadata::setFailure(bool failed) {
    if(failed == true && this->_failed != failed) emit streamFailed();
    this->_failed = failed;
}

void YoutubeVideoMetadata::setSts(const QString &sts) {
    this->_sts = sts;
};

void YoutubeVideoMetadata::setPlayerSourceUrl(const QString &pSourceUrl) {
    this->_playerSourceUrl = pSourceUrl;
}

void YoutubeVideoMetadata::setTitle(const QString &title) {
    this->_title = title;
}

void YoutubeVideoMetadata::setDuration(int durationInSeconds) {
    this->_durationInSeconds = durationInSeconds;
}

void YoutubeVideoMetadata::setExpirationDate(const QDateTime &expiration) {
    this->_validUntil = expiration;
}

void YoutubeVideoMetadata::setAudioStreamInfos(const YoutubeAudioStreamInfos &streamInfos) {
    this->_audioStreamInfos = streamInfos;
}

const YoutubeAudioStreamInfos& YoutubeVideoMetadata::audioStreams() const {
    return this->_audioStreamInfos;
}