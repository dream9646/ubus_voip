cmake_minimum_required(VERSION 2.6)

INCLUDE(CheckFunctionExists)

PROJECT(rpcd C)
ADD_DEFINITIONS(-Os -Wall -Werror --std=gnu99 -g3 -Wmissing-declarations -DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
INCLUDE_DIRECTORIES(include)

OPTION(FILE_SUPPORT "File plugin support" ON)
OPTION(IWINFO_SUPPORT "libiwinfo plugin support" ON)
OPTION(RPCSYS_SUPPORT "rpc-sys plugin support" ON)
OPTION(UCODE_SUPPORT "ucode plugin support" ON)

SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

#IF(APPLE)
#  INCLUDE_DIRECTORIES(/opt/local/include)
#  LINK_DIRECTORIES(/opt/local/lib)
#ENDIF()

CHECK_FUNCTION_EXISTS(getspnam HAVE_SHADOW)
IF(HAVE_SHADOW)
  ADD_DEFINITIONS(-DHAVE_SHADOW)
ENDIF()

FIND_LIBRARY(uci NAMES uci)
FIND_LIBRARY(ubus NAMES ubus)
FIND_LIBRARY(ubox NAMES ubox)
FIND_LIBRARY(blobmsg_json NAMES blobmsg_json)
FIND_LIBRARY(json NAMES json-c json)
#FIND_LIBRARY(crypt NAMES crypt)
#IF(crypt STREQUAL "crypt-NOTFOUND")
#  SET(crypt "")
#ENDIF()

FIND_PATH(ubus_include_dir libubus.h)
INCLUDE_DIRECTORIES(${ubus_include_dir})

FIND_PATH(ubox_include_dir libubox/blobmsg_json.h)
INCLUDE_DIRECTORIES(${ubox_include_dir})

ADD_EXECUTABLE(fvt_evoip  ubus_voip.c)
#TARGET_LINK_LIBRARIES(fvt_evoip ${ubox} ${ubus} ${uci} ${blobmsg_json} ${json} ${crypt} dl)
TARGET_LINK_LIBRARIES(fvt_evoip ${ubox} ${ubus} ${uci} ${blobmsg_json} ${json} dl)


SET(PLUGINS "")



INSTALL(TARGETS fvt_evoip ${PLUGINS}
	RUNTIME DESTINATION sbin
	LIBRARY DESTINATION lib/fvt_evoip
)
