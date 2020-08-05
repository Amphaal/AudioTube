// AudioTube C++
// C++ fork based on https://github.com/Tyrrrz/YoutubeExplode
// Copyright (C) 2019-2020 Guillaume Vara

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "_NetworkHelper.h"

promise::Defer AudioTube::NetworkHelper::downloadHTTPS(const std::string &downloadUrl, bool head = false) {
    return promise::newPromise([=](promise::Defer d) {
        // decompose url
        UrlParser url_decomposer(downloadUrl);
        auto serverName = url_decomposer.host();
        auto scheme = url_decomposer.scheme();

        auto getCommand = url_decomposer.pathAndQuery();

        // setup service
        asio::io_service io_service;

        // setup SSL
        ssl::context ssl_ctx(ssl::context::sslv23);
        ssl_ctx.set_default_verify_paths();
        asio::ssl::stream<tcp::socket> ssl_sock(io_service, ssl_ctx);

        // setup Resolver
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(serverName, scheme);
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        tcp::resolver::iterator end;

        // Connect to host
        asio::connect(ssl_sock.lowest_layer(), resolver.resolve(query));
        ssl_sock.lowest_layer().set_option(tcp::no_delay(true));

        // Perform SSL handshake and verify the remote host's certificate.
        ssl_sock.set_verify_mode(ssl::verify_peer);
        ssl_sock.set_verify_callback(ssl::rfc2818_verification(serverName));
        ssl_sock.handshake(ssl::stream<tcp::socket>::client);

        // start writing
        asio::streambuf request;
        std::ostream request_stream(&request);

        auto method = head ? "HEAD" : "GET";

        request_stream << method << " " << getCommand << " HTTP/1.0\r\n";
        request_stream << "Host: " << serverName << "\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Connection: close\r\n\r\n";

        spdlog::debug("HTTPSDownloader : Downloading from [{}]...", downloadUrl);

        // Send the request.
        asio::write(socket, request);

        // Read the response status line.
        asio::streambuf response;
        asio::read_until(socket, response, "\r\n");

        // Check that response is OK.
        std::istream response_stream(&response);
        std::string http_version;
        response_stream >> http_version;
        unsigned int status_code;
        response_stream >> status_code;
        std::string status_message;
        std::getline(response_stream, status_message);

        // Read the response headers, which are terminated by a blank line.
        asio::read_until(socket, response, "\r\n\r\n");

        // Process the response headers.
        std::string headerTmp;
        std::vector<std::string> headers;
        bool hasContentLengthHeader = false;

        // iterate
        while (std::getline(response_stream, headerTmp) && headerTmp != "\r") {
            headers.push_back(headerTmp);
            if (!hasContentLengthHeader) {
                hasContentLengthHeader = headerTmp.find("Content-Length") != std::string::npos;
            }
        }

        if (!hasContentLengthHeader) {
            throw std::logic_error("HTTPSDownloader : Targeted URL is not a file");
        }

        // Write whatever content we already have to output.
        std::ostringstream output_stream;
        if (response.size() > 0) {
            output_stream << &response;
        }
        // Read until EOF, writing data to output as we go.
        asio::error_code error;
        while (asio::read(socket, response, asio::transfer_at_least(1), error)) {
            output_stream << &response;
        }

        spdlog::debug("HTTPSDownloader : Finished downloading [{}]", downloadUrl);

        return output_stream.str();
    });
}
