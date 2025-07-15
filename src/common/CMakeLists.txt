# Copyright (c) 2018-2022, The Arqma Network
# Copyright (c) 2014-2018, The Monero Project
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are
# permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of
#    conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list
#    of conditions and the following disclaimer in the documentation and/or other
#    materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may be
#    used to endorse or promote products derived from this software without specific
#    prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
# THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## exp2 + round <-- if somebody would like to do some precise equations near future :)

add_library(common
  base58.cpp
  command_line.cpp
  dns_utils.cpp
  download.cpp
  error.cpp
  expect.cpp
  arqma.cpp
  util.cpp
  i18n.cpp
  notify.cpp
  password.cpp
  perf_timer.cpp
  random.cpp
  rules.cpp
  pruning.cpp
  spawn.cpp
  scoped_message_writer.cpp
  threadpool.cpp
  updates.cpp
  aligned.c
  combinator.cpp)

if (STACK_TRACE)
  target_sources(common PRIVATE stack_trace.cpp)
  if (WIN32 OR STATIC)
    target_compile_definitions(common PRIVATE STATICLIB)
  endif()
endif()

if (BACKCOMPAT)
  target_sources(common PRIVATE compat/glibc_compat.cpp)
endif()

add_dependencies(common generate_translations_header)

target_link_libraries(common
  PUBLIC
    cncrypto
  PRIVATE
    libunbound
    epee
    Boost::regex
    OpenSSL::SSL
    OpenSSL::Crypto
    extra)

target_include_directories(common PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/../../translations")

option(STACK_TRACE "Install a hook that dumps stack on exception" OFF)

if (STACK_TRACE)
  if (APPLE)
  elseif (DEPENDS AND NOT LINUX)
  elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT MINGW)
    target_link_libraries(common PRIVATE easylogging)
  elseif (ARM AND STATIC)
  else()
    find_package(Libunwind)
    if (LIBUNWIND_FOUND)
      target_link_libraries(common PRIVATE ${LIBUNWIND_LIBRARIES})
      target_include_directories(common PRIVATE ${LIBUNWIND_INCLUDE_DIR})
    endif()
  endif()
endif()
