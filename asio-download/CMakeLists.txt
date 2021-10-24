# Distributed under the OSI-approved MIT License.  See accompanying
# file LICENSE or https://github.com/Crascit/DownloadProject for details.

cmake_minimum_required(VERSION 2.8.2)

project(asio-download NONE)

include(ExternalProject)
ExternalProject_Add(asio-download
                    GIT_REPOSITORY;https://github.com/chriskohlhoff/asio.git;GIT_TAG;22afb86;UPDATE_DISCONNECTED;1
                    SOURCE_DIR          "/home/ec2-user/q-bot/sleepy-discord/deps/asio"
                    BINARY_DIR          "/home/ec2-user/q-bot/asio-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
)
