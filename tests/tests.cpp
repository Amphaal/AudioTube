#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include <QCoreApplication>
#include <QTimer>

#include <audiotube/VideoMetadata.h>
#include <audiotube/NetworkFetcher.h>
#include <audiotube/_base/_NetworkHelper.h>

#include <chrono>
#include <thread>

bool stream_are_working(VideoMetadata &metadata) {
    
    bool isStreamAvailable; bool ended;
    NetworkFetcher::isStreamAvailable(&metadata, &ended, &isStreamAvailable);
    
    while(!ended) {
      QCoreApplication::processEvents();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return isStreamAvailable;

}


bool youtube_metadata_fetching_succeeded(const QString &ytId) {
    
    auto container = VideoMetadata::fromVideoId(ytId);
    NetworkFetcher::refreshMetadata(&container);
    
    while(!container.ranOnce()) {
      QCoreApplication::processEvents();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if(container.hasFailed()) return false;
    return stream_are_working(container);

}



int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);
  QTimer::singleShot(0, [&]{
    app.exit(Catch::Session().run(argc, argv));
  });
  return app.exec();
}

//
// Test cases
//

// TEST_CASE( "Unavailable video", "[metadata]" ) {
//   REQUIRE_FALSE(youtube_metadata_fetching_succeeded("MnoajJelaAo"));
// }

// TEST_CASE( "OK from JSON Adaptative Stream - no deciphering", "[metadata]" ) {
//   REQUIRE(youtube_metadata_fetching_succeeded("-Q5Y037vIyc"));
// }

// TEST_CASE( "OK from JSON Adaptative Stream - must decipher", "[metadata]" ) {
//   REQUIRE(youtube_metadata_fetching_succeeded("qyYFF3Eh6lw"));
// }

TEST_CASE( "Restricted viewing", "[metadata]" ) {
  REQUIRE(youtube_metadata_fetching_succeeded("dNv1ImIa1-4"));
}