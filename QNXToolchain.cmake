set(CMAKE_SYSTEM_NAME QNX)
set(CMAKE_SYSTEM_VERSION 7.0.0)
set(CMAKE_SYSTEM_PROCESSOR x86)
set(TOOLCHAIN QNX)
set(QNX YES)
set(HOST_EXECUTABLE_SUFFIX .exe)

find_path(QNX_HOST
    NAME usr/bin/qcc${HOST_EXECUTABLE_SUFFIX}
    PATHS $ENV{QNX_HOST}
    NO_CMAKE_PATH
    NO_CMAKE_ENVIRONMENT_PATH
)

find_path(QNX_TARGET
    NAME usr/include/qnx_errno.h
    PATHS $ENV{QNX_TARGET}
    NO_CMAKE_PATH
    NO_CMAKE_ENVIRONMENT_PATH
)

set(CMAKE_C_COMPILER ${QNX_HOST}/usr/bin/nto${CMAKE_SYSTEM_PROCESSOR}-gcc-4.8.3${HOST_EXECUTABLE_SUFFIX})
set(CMAKE_CXX_COMPILER ${QNX_HOST}/usr/bin/nto${CMAKE_SYSTEM_PROCESSOR}-c++-4.8.3${HOST_EXECUTABLE_SUFFIX})

get_property(cxx_features GLOBAL PROPERTY CMAKE_CXX_KNOWN_FEATURES)
set(CMAKE_CXX_COMPILE_FEATURES ${cxx_features})
set(GLOBAL PROPERTY CMAKE_C_COMPILE_FEATURES ${cxx_features})
