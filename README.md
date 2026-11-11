# AUCTION ENGINE
> **Hệ thống Đấu giá Trực tuyến Thời gian thực Chuẩn Kiến trúc Đa tầng (Clean Architecture)**  
> **Bài tập lớn môn Lập trình Hướng đối tượng (OOP) • Khoa Công nghệ Thông tin — UTC2**

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg?style=flat&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/20)
[![Qt 6](https://img.shields.io/badge/Qt-6.11%20Widgets-green.svg?style=flat&logo=qt)](https://www.qt.io/)
[![CMake](https://img.shields.io/badge/CMake-3.21%2B-064F8C.svg?style=flat&logo=cmake)](https://cmake.org/)
[![SQLite 3](https://img.shields.io/badge/SQLite-3%20(WAL)-003B57.svg?style=flat&logo=sqlite)](https://sqlite.org/)
[![WebSocket](https://img.shields.io/badge/Protocol-WebSocket%20JSON--RPC-orange.svg)](https://www.jsonrpc.org/specification)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

---

## 📌 1. Giới thiệu Tổng quan Đề tài

**Auction Engine** là giải pháp phần mềm đấu giá trực tuyến theo thời gian thực được thiết kế và phát triển phục vụ học phần **Lập trình Hướng đối tượng (OOP)** tại **Trường Đại học Giao thông Vận tải - Phân hiệu tại TP.HCM (UTC2)**.

Dự án áp dụng chặt chẽ các nguyên lý thiết kế hướng đối tượng nâng cao (**SOLID**, **Clean Architecture**, **Mẫu thiết kế GoF**), phân tách hoàn toàn giữa lõi tính toán nghiệp vụ (Domain & Application) và giao diện hiển thị (Presentation Qt6), giao tiếp full-duplex thông qua giao thức **WebSocket JSON-RPC 2.0** có bảo mật TLS/WSS và cơ chế lưu trữ bền vững **SQLite 3 WAL Mode**.

### 🌟 Tính năng nổi bật
* **Đấu giá Thời gian thực (Real-time Low Latency)**: Tự động phát sóng cập nhật bảng giá (Broadcast Price Updates) với độ trễ tính bằng mili-giây tới toàn bộ máy trạm tham gia.
* **Quy trình Đặt giá Nguyên tử & Rollback**: Bảo toàn số dư ví tiền điện tử; tự động giải tỏa ký quỹ (Escrow Hold) nếu có lỗi xảy ra ở bất kỳ khâu nào.
* **Quản lý Vòng đời Phiên bằng State Pattern**: Chuyển trạng thái an toàn tuyệt đối (*Chờ bắt đầu ➔ Đang đấu giá ➔ Tạm dừng ➔ Kết thúc*).
* **Đa dạng Mặt hàng Đấu giá**: Kế thừa đa hình hỗ trợ mở rộng không giới hạn các danh mục tài sản (*Đồ cổ, Tranh nghệ thuật, Bất động sản, Thiết bị công nghệ...*).
* **Bảo mật & Phiên làm việc**: Băm mật khẩu chuẩn PBKDF2/SHA-256, kiểm soát phiên đăng nhập phân tán và phân quyền tác nhân (Admin, Auctioneer, Bidder).

---

## 🏫 2. Thông tin Học viện & Đơn vị Thực hiện

* **Cơ sở đào tạo**: Trường Đại học Giao thông Vận tải - Phân hiệu tại TP. Hồ Chí Minh (UTC2)
* **Khoa**: Công nghệ Thông tin
* **Bộ môn**: Công nghệ Thông tin
* **Ngành**: Công nghệ Thông tin
* **Học phần**: Lập trình Hướng đối tượng (OOP) • **Mã lớp**: `CQ.66.CNTT`
* **Giảng viên hướng dẫn**: **ThS. Trần Thị Dung**
* **Năm học**: 2026 - 2027

### 👥 Danh sách Thành viên Nhóm thực hiện (Nhóm 4)

| STT | Họ và tên | MSSV | Vai trò | Trách nhiệm & Phạm vi đảm nhiệm |
| :-: | :--- | :-: | :-: | :--- |
| **1** | **Phạm Ngọc Thắng** | **6651071071** | **Trưởng nhóm** | Quản trị dự án Scrum/Jira; Thiết kế kiến trúc Clean Architecture đa tầng; Hiện thực State Pattern, Transaction Pipeline & Rollback bảo toàn tiền. |
| **2** | **Phạm Đan Trường** | **6651071086** | Thành viên | Hiện thực Observer Pattern (Đồng bộ thời gian thực); Thiết kế giao thức mạng WebSocket JSON-RPC; Xây dựng Network Manager & Đồng bộ bảng giá. |
| **3** | **Phan Minh Tân** | **6651071067** | Thành viên | Thiết kế CSDL SQLite 3 (WAL mode); Quản lý Domain Core (Tài khoản, Ví điện tử, Sản phẩm đấu giá); Biên tập & Sản xuất Video Demo TikTok. |
| **4** | **Trần Minh Khôi** | **6651071034** | Thành viên | Xây dựng giao diện Client Qt 6 Widgets; Hiện thực Strategy Pattern (Bộ quy tắc đặt giá IBidRule); Quản trị kho mã nguồn GitHub & Viết Báo cáo LaTeX. |

---

## 🔗 3. Hệ sinh thái Liên kết Bàn giao Đồ án

| Kênh tài nguyên | Phân loại | Đường dẫn truy cập (Click để mở) |
| :--- | :--- | :--- |
| 📦 **Mã nguồn Dự án** | GitHub Repo | [https://github.com/TranMinhKhoi2007/Auction-Engine-Project.git](https://github.com/TranMinhKhoi2007/Auction-Engine-Project.git) |
| 🎯 **Quản trị Tiến độ** | Jira Scrum Board | [https://phamngocthangcr7.atlassian.net/jira/software/projects/SCRUM/boards/1](https://phamngocthangcr7.atlassian.net/jira/software/projects/SCRUM/boards/1) |
| 📄 **Báo cáo Thuyết minh** | Overleaf LaTeX | [https://www.overleaf.com/read/jhrxcmkvtpfx#706fda](https://www.overleaf.com/read/jhrxcmkvtpfx#706fda) |
| 🎨 **Slide Thuyết trình** | Canva Presentation | [https://canva.link/qe62pngagcwjtmu](https://canva.link/qe62pngagcwjtmu) |
| 🎬 **Video Demo Trực quan** | TikTok Video | [https://www.tiktok.com/@hero_phan1503/video/7704060293193731336](https://www.tiktok.com/@hero_phan1503/video/7704060293193731336) |

---

## 🏗️ 4. Kiến trúc Hệ thống & Áp dụng Mẫu thiết kế (OOP Patterns)

Dự án được xây dựng tuân thủ mô hình **Clean Architecture 5 tầng**:
```text
┌─────────────────────────────────────────────────────────────┐
│                    Presentation Layer                       │
│    (Qt 6 Widgets: AppShell, Pages, Dialogs, Custom Views)   │
└──────────────────────────────▲──────────────────────────────┘
                               │ (Observer & Events)
┌──────────────────────────────┴──────────────────────────────┐
│                    Application Layer                        │
│   (AuctionEngine, AuctionService, Commands, DTOs, Policies) │
└──────────────────────────────▲──────────────────────────────┘
                               │ (Dependency Inversion)
┌──────────────────────────────┴──────────────────────────────┐
│                      Domain Core                            │
│  (NguoiDung, ViTien, SanPhamDauGia, PhienDauGia, LuotDatGia)│
└──────────────────────────────▲──────────────────────────────┘
                               │ (Repository Ports)
┌──────────────────────────────┴──────────────────────────────┐
│                  Infrastructure Layer                       │
│     (SqliteStore WAL, Pbkdf2Hasher, JsonCodec, WebSocket)   │
└─────────────────────────────────────────────────────────────┘
```

### 🧩 4 Mẫu thiết kế Hướng đối tượng Cốt lõi (GoF Patterns)
1. **State Pattern (`IPhienDauGiaState`)**:
   * Quản lý vòng đời trạng thái phiên đấu giá khép kín: `ChoDauGiaState` ➔ `DangDauGiaState` ➔ `TamDungState` ➔ `KetThucState`.
   * Mỗi trạng thái đóng gói các hành vi hợp lệ riêng biệt; tự động chặn đặt giá khi chưa mở hoặc đã kết thúc.
2. **Strategy Pattern (`IBidRule`)**:
   * Đóng gói linh hoạt các thuật toán kiểm tra tính hợp lệ của lượt đặt giá (`StepSizeRule`, `BalanceRule`, `TimeRule`). Dễ dàng thêm quy tắc mới mà không sửa code cũ (Open/Closed Principle).
3. **Observer Pattern (`ISubject` & `IObserver`)**:
   * Khi có người đặt giá thành công hoặc bước nhảy thời gian thay đổi, `AuctionEngine` thông báo ngay lập tức đến toàn bộ Client đã đăng ký lắng nghe mà không cần cơ chế Polling tốn tài nguyên.
4. **Singleton Pattern (`AuctionEngine::instance()`)**:
   * Triển khai mẫu **Meyers' Singleton** an toàn luồng tuyệt đối (Thread-safe) trong chuẩn C++20 để quản lý duy nhất phiên làm việc của động cơ lõi.

---

## 📁 5. Cấu trúc Thư mục Dự án

```text
Auction-Engine-Project/
├── CMakeLists.txt            # Tập tin cấu hình biên dịch CMake chính
├── README.md                 # Tài liệu giới thiệu & hướng dẫn dự án
├── .gitignore                # Danh mục tệp bỏ qua khi kiểm soát mã nguồn
├── cmake/                    # Các module kiểm tra phân tầng kiến trúc
│   └── CheckClientLayering.cmake
├── app/                      # Điểm khởi chạy chương trình (Main Executables)
│   ├── client_main.cpp       # Mã nguồn khởi chạy ứng dụng Client (Qt GUI)
│   ├── server_main.cpp       # Mã nguồn khởi chạy Server headless
│   └── lan_probe.cpp         # Công cụ dò tìm máy chủ qua mạng nội bộ
├── include/auction/          # Header Files (.h) phân tầng chuẩn mực
│   ├── application/          # Lớp Ứng dụng: Engine, Services, DTOs, Commands
│   ├── client/               # Lớp Máy trạm: Gateway, Clocks, Connection
│   ├── common/               # Kiểu dữ liệu dùng chung, StrongId, TimeTypes
│   ├── domain/               # Lớp Thực thể nghiệp vụ, Exceptions, Ports
│   ├── infrastructure/       # Bộ nhớ tạm, EventLoop, Logging, Hashing
│   ├── infrastructure_qt/    # SQLite 3 Store, Qt Adapters
│   ├── media/                # Quản lý hình ảnh và dữ liệu nhị phân
│   ├── presentation/         # Qt Widgets UI, ViewModels, Dialogs
│   ├── protocol/             # Giao thức mạng Wire Protocol (JSON-RPC 2.0)
│   └── server/               # Lõi Server: Dispatcher, Session, Broadcaster
├── src/                      # Implementation Files (.cpp) tương ứng
├── resources/                # Tài nguyên đồ họa, cơ sở dữ liệu và styles
│   ├── app.qrc               # Qt Resource System cho hình ảnh & stylesheet
│   ├── database.qrc          # File cơ sở dữ liệu mẫu
│   └── sql/                  # Kịch bản khởi tạo CSDL SQLite DDL/DML
└── tests/                    # Bộ kiểm thử tự động (Unit Tests & Integration)
```

---

## 🛠️ 6. Hướng dẫn Biên dịch & Chạy Ứng dụng

### Yêu cầu Tiền đề (Prerequisites)
* **Trình biên dịch**: GCC/MinGW 13.1+ (Hỗ trợ đầy đủ **C++20**) hoặc MSVC 2022.
* **CMake**: Phiên bản **>= 3.21**.
* **Framework Qt**: Qt **6.4+** (Khuyến nghị **Qt 6.11.1** với các module: `Core`, `Gui`, `Widgets`, `Network`, `WebSockets`, `Sql`).
* **Hệ thống Build**: **Ninja** (khuyến nghị) hoặc Make.

### 🚀 Các bước Biên dịch (Windows PowerShell)

```powershell
# 1. Clone repository
git clone https://github.com/TranMinhKhoi2007/Auction-Engine-Project.git
cd Auction-Engine-Project

# 2. Thiết lập đường dẫn môi trường Qt (điều chỉnh theo thư mục cài đặt Qt của bạn)
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;" + $env:PATH

# 3. Tạo thư mục build và cấu hình CMake
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/Qt/6.11.1/mingw_64"

# 4. Biên dịch toàn bộ dự án
cmake --build build --config Release
```

### 💻 Chạy Chương trình
1. **Khởi động Máy chủ (AuctionServer)**:
   ```powershell
   ./build/AuctionServer.exe
   ```
   *Máy chủ sẽ mở cổng WebSocket (mặc định: `ws://127.0.0.1:8080`) và sẵn sàng phục vụ.*

2. **Khởi động Máy trạm Đồ họa (AuctionApp)**:
   ```powershell
   ./build/AuctionApp.exe
   ```
   *Đăng nhập bằng tài khoản người dùng hoặc tạo tài khoản mới để tham gia đấu giá trực tiếp.*

---

## 📜 7. Giấy phép Bản quyền (License)

Dự án được phân phối dưới giấy phép **MIT License**. Toàn bộ mã nguồn phục vụ mục đích nghiên cứu, học tập và thẩm định kết quả học phần tại Trường Đại học Giao thông Vận tải - Phân hiệu tại TP.HCM.
