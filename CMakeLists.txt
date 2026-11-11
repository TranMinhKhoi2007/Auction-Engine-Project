cmake_minimum_required(VERSION 3.21)
project(AuctionEngine VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Cấu hình Qt 6
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

# Gợi ý đường dẫn Qt 6 nếu chưa thiết lập trong môi trường
if(NOT CMAKE_PREFIX_PATH AND EXISTS "C:/Qt/6.11.1/mingw_64")
    set(CMAKE_PREFIX_PATH "C:/Qt/6.11.1/mingw_64")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg Network WebSockets Sql)
qt_standard_project_setup()

# Options
option(ENABLE_SANITIZERS "Kích hoạt Address và Undefined Behavior Sanitizers" OFF)
option(AUCTION_BUILD_TESTS "Build các target kiểm thử (chỉ dành cho môi trường phát triển)" OFF)
option(AUCTION_TESTING "Kích hoạt các hàm hỗ trợ kiểm thử" ${AUCTION_BUILD_TESTS})

if(AUCTION_BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()

if(AUCTION_TESTING)
    add_compile_definitions(AUCTION_TESTING=1)
endif()

# Compiler flags cho các target lõi (thuần STL, -Werror)
set(AUCTION_CORE_COMPILE_OPTIONS "")
if(MSVC)
    set(AUCTION_CORE_COMPILE_OPTIONS /W4 /WX /utf-8)
else()
    set(AUCTION_CORE_COMPILE_OPTIONS
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
        -Werror
        -Wno-error=maybe-uninitialized
    )
    if(ENABLE_SANITIZERS)
        set(AUCTION_CORE_COMPILE_OPTIONS ${AUCTION_CORE_COMPILE_OPTIONS} -fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

set(AUCTION_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 1. Common Layer (Header-only / Interface)
add_library(auction_common INTERFACE)
target_include_directories(auction_common INTERFACE ${AUCTION_INCLUDE_DIR})

# 2. Domain Layer (Thuần STL)
file(GLOB_RECURSE DOMAIN_SOURCES "src/domain/*.cpp")
add_library(auction_domain STATIC ${DOMAIN_SOURCES})
target_include_directories(auction_domain PUBLIC ${AUCTION_INCLUDE_DIR})
target_link_libraries(auction_domain PUBLIC auction_common)
target_compile_options(auction_domain PRIVATE ${AUCTION_CORE_COMPILE_OPTIONS})

# 3. Application Layer (Thuần STL)
file(GLOB_RECURSE APPLICATION_SOURCES "src/application/*.cpp")
add_library(auction_application STATIC ${APPLICATION_SOURCES})
target_include_directories(auction_application PUBLIC ${AUCTION_INCLUDE_DIR})
target_link_libraries(auction_application PUBLIC auction_domain)
target_compile_options(auction_application PRIVATE ${AUCTION_CORE_COMPILE_OPTIONS})

# 4. Infrastructure Layer (Lõi, Thuần STL)
file(GLOB_RECURSE INFRASTRUCTURE_SOURCES "src/infrastructure/*.cpp")
add_library(auction_infrastructure STATIC ${INFRASTRUCTURE_SOURCES})
target_include_directories(auction_infrastructure PUBLIC ${AUCTION_INCLUDE_DIR})
target_link_libraries(auction_infrastructure PUBLIC auction_domain)
if(WIN32)
    target_link_libraries(auction_infrastructure PUBLIC bcrypt)
else()
    find_package(OpenSSL REQUIRED COMPONENTS Crypto)
    target_link_libraries(auction_infrastructure PUBLIC OpenSSL::Crypto)
endif()
target_compile_options(auction_infrastructure PRIVATE ${AUCTION_CORE_COMPILE_OPTIONS})

# 5. Infrastructure Qt Layer (Adapter QtEventLoopDriver)
add_library(auction_media STATIC src/media/ProductImage.cpp)
target_include_directories(auction_media PUBLIC ${AUCTION_INCLUDE_DIR})
target_link_libraries(auction_media PUBLIC Qt6::Gui)
target_compile_options(auction_media PRIVATE ${AUCTION_CORE_COMPILE_OPTIONS})
file(GLOB_RECURSE INFRASTRUCTURE_QT_SOURCES "src/infrastructure_qt/*.cpp")
file(GLOB_RECURSE INFRASTRUCTURE_QT_HEADERS "include/auction/infrastructure_qt/*.h")
add_library(auction_infrastructure_qt STATIC ${INFRASTRUCTURE_QT_SOURCES} ${INFRASTRUCTURE_QT_HEADERS} resources/database.qrc)
target_include_directories(auction_infrastructure_qt PUBLIC ${AUCTION_INCLUDE_DIR})
target_link_libraries(auction_infrastructure_qt PUBLIC auction_infrastructure auction_application auction_protocol Qt6::Core Qt6::Network Qt6::Sql)
target_compile_options(auction_infrastructure_qt PRIVATE ${AUCTION_CORE_COMPILE_OPTIONS})

# Wire protocol contains value serialization only; no application services.
add_library(auction_protocol STATIC src/protocol/Envelope.cpp src/protocol/JsonCodec.cpp)
target_include_directories(auction_protocol PUBLIC ${AUCTION_INCLUDE_DIR})
target_link_libraries(auction_protocol PUBLIC auction_domain Qt6::Core)
target_compile_options(auction_protocol PRIVATE ${AUCTION_CORE_COMPILE_OPTIONS})

file(GLOB SERVER_SOURCES CONFIGURE_DEPENDS "src/server/*.cpp")
add_library(auction_transport STATIC src/transport/TlsConfiguration.cpp)
target_include_directories(auction_transport PUBLIC ${AUCTION_INCLUDE_DIR})
target_link_libraries(auction_transport PUBLIC Qt6::Network)
target_compile_options(auction_transport PRIVATE ${AUCTION_CORE_COMPILE_OPTIONS})
file(GLOB SERVER_HEADERS CONFIGURE_DEPENDS "include/auction/server/*.h")
add_library(auction_server STATIC ${SERVER_SOURCES} ${SERVER_HEADERS})
target_include_directories(auction_server PUBLIC ${AUCTION_INCLUDE_DIR})
target_link_libraries(auction_server PUBLIC auction_media auction_transport auction_protocol auction_application auction_infrastructure auction_infrastructure_qt Qt6::Network Qt6::WebSockets)
target_compile_options(auction_server PRIVATE ${AUCTION_CORE_COMPILE_OPTIONS})
add_executable(AuctionServer app/server_main.cpp)
target_link_libraries(AuctionServer PRIVATE auction_server)
if(WIN32)
    add_custom_command(TARGET AuctionServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt6::Sql> $<TARGET_FILE_DIR:AuctionServer>
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:AuctionServer>/sqldrivers
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt6::QSQLiteDriverPlugin> $<TARGET_FILE_DIR:AuctionServer>/sqldrivers)
endif()
file(GLOB CLIENT_SOURCES CONFIGURE_DEPENDS "src/client/*.cpp")
add_library(auction_client STATIC ${CLIENT_SOURCES} include/auction/client/IAuctionGateway.h include/auction/client/RemoteGateway.h)
target_include_directories(auction_client PUBLIC ${AUCTION_INCLUDE_DIR})
target_link_libraries(auction_client PUBLIC auction_transport auction_protocol auction_domain Qt6::Network Qt6::WebSockets)
add_executable(AuctionTlsProbe app/lan_probe.cpp)
target_link_libraries(AuctionTlsProbe PRIVATE auction_transport auction_protocol Qt6::WebSockets)
target_compile_options(auction_client PRIVATE ${AUCTION_CORE_COMPILE_OPTIONS})
# 6. Presentation Layer (Qt 6 Widgets)
file(GLOB_RECURSE PRESENTATION_SOURCES CONFIGURE_DEPENDS "src/presentation/*/*.cpp")
file(GLOB_RECURSE PRESENTATION_HEADERS CONFIGURE_DEPENDS "include/auction/presentation/*/*.h")
add_library(auction_presentation STATIC ${PRESENTATION_SOURCES} ${PRESENTATION_HEADERS}
    src/presentation/Formatters.cpp src/presentation/QtEventBridge.cpp src/presentation/QtLogSink.cpp
    include/auction/presentation/QtEventBridge.h include/auction/presentation/QtLogSink.h
    resources/app.qrc)
target_include_directories(auction_presentation PUBLIC ${AUCTION_INCLUDE_DIR})
target_link_libraries(auction_presentation PUBLIC auction_media auction_client Qt6::Widgets Qt6::Svg)
if(WIN32)
    target_link_libraries(auction_presentation PUBLIC winmm)
endif()
if(NOT MSVC)
    target_compile_options(auction_presentation PRIVATE -Wall -Wextra)
endif()

# 7. Main Executable (AuctionApp)
add_executable(AuctionApp app/client_main.cpp)
target_link_libraries(AuctionApp PRIVATE
    auction_presentation
    auction_domain
    auction_common
    Qt6::Widgets
)
if(WIN32)
    foreach(TLS_EXECUTABLE AuctionServer AuctionApp)
        add_custom_command(TARGET ${TLS_EXECUTABLE} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${TLS_EXECUTABLE}>/tls
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt6::QSchannelBackendPlugin> $<TARGET_FILE_DIR:${TLS_EXECUTABLE}>/tls
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt6::QTlsBackendCertOnlyPlugin> $<TARGET_FILE_DIR:${TLS_EXECUTABLE}>/tls)
    endforeach()
endif()

if(AUCTION_BUILD_TESTS)
    enable_testing()

    # Client, server, and protocol tests
    add_executable(auction_client_tests tests/client/TestClient.cpp)
    target_include_directories(auction_client_tests PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(auction_client_tests PRIVATE auction_client auction_server Qt6::Test)
    add_executable(auction_server_tests tests/server/TestServer.cpp)
    target_include_directories(auction_server_tests PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(auction_server_tests PRIVATE auction_server Qt6::Test)
    add_dependencies(auction_server_tests AuctionApp AuctionServer)

    add_executable(auction_protocol_tests tests/protocol/TestProtocol.cpp)
    target_link_libraries(auction_protocol_tests PRIVATE auction_protocol Qt6::Test)

    # Unit tests (không sleep thật, dùng SimulatedClock)
    file(GLOB_RECURSE UNIT_TEST_SOURCES "tests/unit/*.cpp")
    file(GLOB_RECURSE SUPPORT_SOURCES "tests/support/*.cpp")
    add_executable(auction_unit_tests ${UNIT_TEST_SOURCES} ${SUPPORT_SOURCES})
    target_include_directories(auction_unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    target_link_libraries(auction_unit_tests PRIVATE
        auction_application
        auction_infrastructure
        auction_domain
        auction_common
        Qt6::Test
    )

    # GUI tests (Qt Test trên MainWindow thật)
    file(GLOB_RECURSE GUI_TEST_SOURCES "tests/gui/*.cpp")
    add_executable(auction_gui_tests ${GUI_TEST_SOURCES})
    target_include_directories(auction_gui_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    target_link_libraries(auction_gui_tests PRIVATE
        auction_server
        auction_presentation
        auction_infrastructure_qt
        auction_infrastructure
        auction_application
        auction_domain
        auction_common
        Qt6::Test
        Qt6::Widgets
    )

    add_executable(auction_image_tests tests/images/TestImages.cpp)
    target_include_directories(auction_image_tests PRIVATE ${CMAKE_SOURCE_DIR})
    target_compile_definitions(auction_image_tests PRIVATE AUCTION_TLS_FIXTURES="${CMAKE_SOURCE_DIR}/tests/tls/fixtures")
    target_link_libraries(auction_image_tests PRIVATE auction_server auction_presentation Qt6::Test Qt6::Widgets Qt6::Sql)
    add_test(NAME ImageTests COMMAND auction_image_tests -o "${CMAKE_BINARY_DIR}/image-tests.txt,txt")
    set_tests_properties(ImageTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    add_executable(auction_tls_tests tests/tls/TestTls.cpp)
    target_include_directories(auction_tls_tests PRIVATE ${CMAKE_SOURCE_DIR})
    target_compile_definitions(auction_tls_tests PRIVATE AUCTION_TLS_FIXTURES="${CMAKE_SOURCE_DIR}/tests/tls/fixtures")
    target_link_libraries(auction_tls_tests PRIVATE auction_server auction_presentation Qt6::Test Qt6::Widgets)
    add_test(NAME TlsTests COMMAND auction_tls_tests -o "${CMAKE_BINARY_DIR}/tls-tests.txt,txt")
    set_tests_properties(TlsTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    add_executable(auction_persistence_tests tests/persistence/TestPersistence.cpp tests/support/SimulatedClock.cpp)
    target_include_directories(auction_persistence_tests PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(auction_persistence_tests PRIVATE auction_server Qt6::Test Qt6::Sql)
    add_dependencies(auction_persistence_tests AuctionServer)
    add_test(NAME PersistenceTests COMMAND auction_persistence_tests -o "${CMAKE_BINARY_DIR}/persistence-tests.txt,txt")

    add_executable(auction_authentication_tests tests/accounts/TestAuthentication.cpp)
    target_include_directories(auction_authentication_tests PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(auction_authentication_tests PRIVATE auction_server Qt6::Test)
    add_test(NAME AuthenticationTests COMMAND auction_authentication_tests -o "${CMAKE_BINARY_DIR}/authentication-tests.txt,txt")

    add_executable(auction_crypto_adapter_tests tests/accounts/TestCryptoAdapters.cpp)
    target_include_directories(auction_crypto_adapter_tests PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(auction_crypto_adapter_tests PRIVATE auction_infrastructure_qt Qt6::Test)
    add_test(NAME CryptoAdapterTests COMMAND auction_crypto_adapter_tests -o "${CMAKE_BINARY_DIR}/crypto-adapter-tests.txt,txt")

    add_executable(auction_account_foundations_tests tests/accounts/TestAccountFoundations.cpp tests/support/SimulatedClock.cpp)
    target_include_directories(auction_account_foundations_tests PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(auction_account_foundations_tests PRIVATE auction_application auction_infrastructure Qt6::Test)
    add_test(NAME AccountFoundationsTests COMMAND auction_account_foundations_tests -o "${CMAKE_BINARY_DIR}/account-foundations-tests.txt,txt")

    set(LEGACY_BUSINESS_SOURCES ${UNIT_TEST_SOURCES})
    list(FILTER LEGACY_BUSINESS_SOURCES EXCLUDE REGEX "/TestMain.cpp$")
    # The unchanged legacy UnitTests still run separately and report the explicitly
    # retained six-character-password incompatibility. This runner checks every
    # other legacy business case with its existing assertions and inputs.
    add_executable(auction_business_regression_tests tests/accounts/LegacyBusinessMain.cpp ${LEGACY_BUSINESS_SOURCES} ${SUPPORT_SOURCES})
    target_include_directories(auction_business_regression_tests PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/tests)
    target_link_libraries(auction_business_regression_tests PRIVATE auction_application auction_infrastructure Qt6::Test)
    add_test(NAME BusinessRegressionTests COMMAND auction_business_regression_tests)

    add_test(NAME ProtocolTests COMMAND auction_protocol_tests)
    add_test(NAME ServerTests COMMAND auction_server_tests -o "${CMAKE_BINARY_DIR}/server-tests.txt,txt")
    add_test(NAME ClientTests COMMAND auction_client_tests -o "${CMAKE_BINARY_DIR}/client-tests.txt,txt")
    add_test(NAME check_client_layering COMMAND ${CMAKE_COMMAND} -DROOT=${CMAKE_SOURCE_DIR} -P ${CMAKE_SOURCE_DIR}/cmake/CheckClientLayering.cmake)
    add_test(NAME UnitTests COMMAND auction_unit_tests)
    add_test(NAME GuiTests COMMAND auction_gui_tests -o "${CMAKE_BINARY_DIR}/gui-tests.txt,txt")
    set_tests_properties(GuiTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()
