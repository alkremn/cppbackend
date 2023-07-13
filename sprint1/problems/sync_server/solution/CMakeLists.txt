cmake_minimum_required(VERSION 3.11)

# ������ ���������� Hello � ������� �� C++
project(Hello CXX)
# ������� ��� ����� ��������������� � ���������� ��������� �++ 20
set(CMAKE_CXX_STANDARD 20)

# ���������� ��������������� ������ conanbuildinfo.cmake, ��������� Conan
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
# ��������� ������ �� conanbuildinfo.cmake, ������� �������� �Make �� ������ � ������������, �������������� Conan
conan_basic_setup()

# ���� Boost ������ 1.78
find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  # boost ������, ��������� � ��������� ������������ ������ ������� ���� �
  # ������������ ������ boost
  include_directories(${Boost_INCLUDE_DIRS})
endif()

# ��������� ����� linux ������� ����������� ���������� pthread ���
# ��������� ����������� �������.
# ��������� ��� ������ ��������� ��� ���������� �� ����� ����������
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_compile_definitions(BOOST_BEAST_USE_STD_STRING_VIEW)

include_directories(${CMAKE_INCLUDE_PATH})

# ������ �������� ������������ �������� ���� src/main.cpp
add_executable(hello src/main.cpp)
# ������ ����������� ���������� ���������� ��� ��������� �������
target_link_libraries(hello PRIVATE Threads::Threads)
