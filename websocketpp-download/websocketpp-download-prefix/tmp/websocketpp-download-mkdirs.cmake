# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/ec2-user/q-bot/sleepy-discord/deps/websocketpp"
  "/home/ec2-user/q-bot/websocketpp-build"
  "/home/ec2-user/q-bot/websocketpp-download/websocketpp-download-prefix"
  "/home/ec2-user/q-bot/websocketpp-download/websocketpp-download-prefix/tmp"
  "/home/ec2-user/q-bot/websocketpp-download/websocketpp-download-prefix/src/websocketpp-download-stamp"
  "/home/ec2-user/q-bot/websocketpp-download/websocketpp-download-prefix/src"
  "/home/ec2-user/q-bot/websocketpp-download/websocketpp-download-prefix/src/websocketpp-download-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
  file(MAKE_DIRECTORY "/home/ec2-user/q-bot/websocketpp-download/websocketpp-download-prefix/src/websocketpp-download-stamp/${subDir}")
endforeach()
