# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/ec2-user/q-bot/sleepy-discord/deps/asio"
  "/home/ec2-user/q-bot/asio-build"
  "/home/ec2-user/q-bot/asio-download/asio-download-prefix"
  "/home/ec2-user/q-bot/asio-download/asio-download-prefix/tmp"
  "/home/ec2-user/q-bot/asio-download/asio-download-prefix/src/asio-download-stamp"
  "/home/ec2-user/q-bot/asio-download/asio-download-prefix/src"
  "/home/ec2-user/q-bot/asio-download/asio-download-prefix/src/asio-download-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
  file(MAKE_DIRECTORY "/home/ec2-user/q-bot/asio-download/asio-download-prefix/src/asio-download-stamp/${subDir}")
endforeach()
