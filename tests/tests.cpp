#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include <QCoreApplication>
#include <QTimer>

#include <audiotube/VideoMetadata.h>
#include <audiotube/NetworkFetcher.h>

#include <chrono>
#include <thread>

bool youtube_metadata_fetching_succeeded(const QString &ytId) {
    auto container = VideoMetadata::fromVideoId(ytId);
    auto refrsh = NetworkFetcher::refreshMetadata(&container);
    while(!container.ranOnce()) {
      QCoreApplication::processEvents();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return !container.hasFailed();
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

// TEST_CASE( "Available video", "[metadata]" ) {
//   REQUIRE(youtube_metadata_fetching_succeeded("-Q5Y037vIyc"));
// }

TEST_CASE( "Available video", "[metadata]" ) {
  REQUIRE(youtube_metadata_fetching_succeeded("qyYFF3Eh6lw"));
}